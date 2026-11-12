// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/* Reference core: MAME's microcoded 68000 (see Sequence.cpp). MAME has no 68010. */

#include "Sequence.h"
#include "MameCpu.h"

using namespace sequence;

namespace {

class MameReference : public Reference {

    mame::Cpu cpu;
    std::vector<u8> mem;
    std::vector<Write> log;
    bool irq = false;
    bool spurious = false;
    int stopMask = -1;

    u16 peek(u32 addr) const { addr &= MEM_MASK; return u16(mem[addr] << 8 | mem[(addr + 1) & MEM_MASK]); }

public:

    MameReference()
    {
        cpu.read = [this](u32 addr, u16) { return peek(addr); };
        cpu.write = [this](u32 addr, u16 val, u16 mask) {
            if (mask & 0xFF00) { mem[addr & MEM_MASK] = u8(val >> 8); log.push_back({ addr & MEM_MASK, u8(val >> 8) }); }
            if (mask & 0x00FF) { mem[(addr + 1) & MEM_MASK] = u8(val); log.push_back({ (addr + 1) & MEM_MASK, u8(val) }); }
        };
        cpu.vector = [this](int level) {
            irq = true;
            if (level == 0) spurious = true;
            if (stopMask >= 0 && level != 7 && level <= stopMask) spurious = true;
            return u8(24 + level);
        };
    }

    const char *name() override { return "MAME"; }

    bool init(const std::string &cpuName) override { return cpuName == "68000"; }

    void load(const std::vector<u8> &image, const Program &prog) override
    {
        mem = image;

        mame::State s;
        for (int i = 0; i < 8; i++) s.d[i] = prog.d[i];
        for (int i = 0; i < 7; i++) s.a[i] = prog.a[i];
        s.usp = USP; s.ssp = SSP;
        s.sr = u16(prog.sr);
        s.pc = PROG;
        s.ird = peek(PROG);
        s.irc = peek(PROG + 2);
        cpu.load(s);
        cpu.setIPL(0);
    }

    void setIPL(int level) override { cpu.setIPL(level); }

    void step() override
    {
        log.clear();
        irq = false;
        spurious = false;

        // Remember the interrupt mask a STOP instruction is about to load
        u32 pc = cpu.state().pc;
        stopMask = peek(pc) == 0x4E72 ? (peek(pc + 2) >> 8) & 7 : -1;

        cpu.executeSequence();
    }

    RefState state() override
    {
        mame::State m = cpu.state();
        RefState r;
        for (int i = 0; i < 8; i++) r.d[i] = m.d[i];
        for (int i = 0; i < 7; i++) r.a[i] = m.a[i];
        r.usp = m.usp; r.isp = m.ssp; r.sr = m.sr; r.pc = m.pc;
        r.sp = (m.sr & 0x2000) ? m.ssp : m.usp;
        return r;
    }

    bool stopped() override { return cpu.stopped(); }
    bool interrupted() override { return irq; }

    /* If a trace exception and an interrupt are due at the same time, MAME's microcode loses the
     * interrupt level while it processes the trace exception (it keeps the pending state in a
     * single variable). The interrupt that follows is acknowledged with level 0. A STOP
     * instruction also takes an interrupt that its new interrupt mask would block, because the
     * microcode decides about it before the mask has been loaded. The programs end when this
     * happens.
     */
    bool unreliable() override { return spurious; }
    bool addressError() override { return cpu.state().pc == H_LOOP; }
    const std::vector<Write> &writes() override { return log; }
};

}

int main(int argc, char **argv)
{
    MameReference ref;
    return sequence::run(argc, argv, ref);
}
