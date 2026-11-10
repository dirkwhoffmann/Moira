// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// Connects MAME's microcoded 68000 to the random test runner (see Tests/Random)

#include "Shared.h"
#include "MameCpu.h"
#include <memory>

static std::unique_ptr<mame::Cpu> mamecpu;

// The 68000 has no such registers. Moira keeps the values it has been given.
static u32 mameVBR, mameSFC, mameDFC, mameCACR, mameCAAR;

// The state loaded by the last reset
static mame::State mameState;

// Counts the write accesses of an instruction. The write with the number mameSuppress is not performed.
static int mameWrites = 0;
static int mameSuppress = -1;

void setupMAME(moira::CPUModel model)
{
    // MAME's core is a 68000
    assert(model == moira::CPUModel::M68000);

    if (mamecpu) return;

    mamecpu = std::make_unique<mame::Cpu>();

    mamecpu->read = [](uint32_t addr, uint16_t mask) -> uint16_t {
        return get16(refMem, addr);
    };
    mamecpu->write = [](uint32_t addr, uint16_t data, uint16_t mask) {
        if (mameWrites++ == mameSuppress) return;
        if (mask == 0xFFFF) set16(refMem, addr, data);
        else if (mask == 0xFF00) set8(refMem, addr, data >> 8);
        else set8(refMem, addr + 1, data & 0xFF);
    };
}

void resetMAME(const runner::Setup &s)
{
    mameVBR = s.vbr; mameSFC = s.sfc & 7; mameDFC = s.dfc & 7; mameCACR = s.cacr; mameCAAR = s.caar;

    mame::State st;

    for (int i = 0; i < 8; i++) st.d[i] = s.d[i];
    for (int i = 0; i < 7; i++) st.a[i] = s.a[i];
    st.usp = 0;
    st.ssp = 0x2000;
    st.sr = (s.supervisor ? 0x2000 : 0) | 0x0700 | s.ccr;
    st.pc = s.pc;
    st.ird = get16(refMem, s.pc);
    st.irc = get16(refMem, s.pc + 2);

    mameState = st;
    mamecpu->load(st);
}

void executeMAME(runner::Result &r)
{
    static u8 snapshot[0x10000];
    memcpy(snapshot, refMem, sizeof(snapshot));

    mameWrites = 0;
    mameSuppress = -1;

    clock_t elapsed = clock();
    r.cycles = mamecpu->execute();
    r.elapsed[0] = clock() - elapsed;

    auto st = mamecpu->state();

    // MAME logs the aborted access of an address error, which never reaches a real bus. Drop it.
    // It is the access that precedes the exception stack frame (the last 7 writes).
    auto log = mamecpu->log();
    bool addressError = false;
    for (auto &b : log) if (!b.write && b.fc == 5 && (b.addr == 0x0C || b.addr == 0x0E)) addressError = true;

    if (addressError) {

        std::vector<size_t> writes;
        for (size_t i = 0; i < log.size(); i++) if (log[i].write) writes.push_back(i);

        // Find the first exception stack frame. Its writes go to SP-2, SP-6, SP-4, SP-8, SP-10, SP-14, SP-12
        static const int offsets[7] = { 0, -4, -2, -6, -8, -12, -10 };
        int frame = -1;
        for (size_t i = 1; i + 6 < writes.size() && frame < 0; i++) {
            bool match = true;
            for (int k = 0; k < 7; k++) {
                auto &e = log[writes[i + k]];
                if (e.fc != 5 || e.addr != log[writes[i]].addr + offsets[k]) { match = false; break; }
            }
            if (match && writes[i - 1] + 1 == writes[i]) frame = int(i);
        }

        if (frame > 0) {

            // MAME has performed the aborted write. Run the instruction again without it, because
            // the write could modify the data the exception processing depends on (e.g., a vector).
            int aborted = frame - 1;
            memcpy(refMem, snapshot, sizeof(snapshot));
            mamecpu->load(mameState);
            mameWrites = 0;
            mameSuppress = aborted;
            r.cycles = mamecpu->execute();
            st = mamecpu->state();
            log = mamecpu->log();
            log.erase(log.begin() + writes[aborted]);
        }
    }

    // Report the write accesses to the sandbox. Moira performs a write after the first two
    // cycles of the bus cycle have elapsed.
    if (CHECK_MEM_WRITES) {
        for (auto &b : log) {
            if (!b.write) continue;
            if (sandbox.records() >= 250) break;
            u64 cycle = b.cycle + 2;
            if (b.mask == 0xFFFF) sandbox.record(POKE16, b.addr, cycle, b.fc, b.data);
            else if (b.mask == 0xFF00) sandbox.record(POKE8, b.addr, cycle, b.fc, b.data >> 8);
            else sandbox.record(POKE8, b.addr + 1, cycle, b.fc, b.data & 0xFF);
        }
    }


    r.pc = st.pc & 0xFFFFFF;
    r.sr = st.sr;
    r.usp = st.usp;
    r.isp = st.ssp;
    r.msp = 0;
    r.fc = 0;
    r.vbr = mameVBR;
    r.sfc = mameSFC;
    r.dfc = mameDFC;
    r.cacr = mameCACR;
    r.caar = mameCAAR;
    for (int i = 0; i < 8; i++) r.d[i] = st.d[i];
    for (int i = 0; i < 7; i++) r.a[i] = st.a[i];
    r.a[7] = (st.sr & 0x2000) ? st.ssp : st.usp;
}
