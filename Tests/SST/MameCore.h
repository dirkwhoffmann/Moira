// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "RefCore.h"

namespace sstrunner {

// MAME's microcoded 68000
struct MameCore : RefCore {

    mame::Cpu cpu;

    MameCore(Memory &mem, Fault &fault)
    {
        cpu.read = [&mem](u32 addr, u16) { return mem.peek16(addr); };
        cpu.write = [&mem](u32 addr, u16 val, u16 mask) {
            if (mask & 0xFF00) mem.poke(addr, u8(val >> 8));
            if (mask & 0x00FF) mem.poke(addr + 1, u8(val));
        };
        cpu.busError = [&fault](u32, bool, int) { return fault.hit(); };
    }

    void load(const sst::State &s) override
    {
        mame::State m;
        for (int i = 0; i < 8; i++) m.d[i] = s.d[i];
        for (int i = 0; i < 7; i++) m.a[i] = s.a[i];
        m.usp = s.usp;
        m.ssp = s.ssp;
        m.sr = u16(s.sr);
        m.pc = (s.pc - 4) & ADDR_MASK;
        m.ird = u16(s.prefetch[0]);
        m.irc = u16(s.prefetch[1]);
        cpu.load(m);
    }

    int execute() override { return cpu.execute(); }
    mame::State state() const override { return cpu.state(); }
    const std::vector<mame::BusCycle> &log() const override { return cpu.log(); }
    bool halted() const override { return cpu.halted(); }

    const char *name() const override { return "MAME's microcoded 68000"; }
    const char *shortName() const override { return "MAME"; }
};

}
