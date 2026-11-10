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

static bool mameAddressError = false;

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

    clock_t elapsed = clock();
    r.cycles = mamecpu->execute();
    r.elapsed[0] = clock() - elapsed;

    auto st = mamecpu->state();

    // Address errors are not part of this test (see the exception tests). Tell the runner to skip
    // the case if MAME raised one, i.e., if it fetched the address error vector.
    auto log = mamecpu->log();
    mameAddressError = false;
    for (auto &b : log) if (!b.write && b.fc == 5 && (b.addr == 0x0C || b.addr == 0x0E)) mameAddressError = true;

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

bool addressErrorMAME() { return mameAddressError; }
