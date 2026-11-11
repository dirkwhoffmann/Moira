// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "Moira.h"
#include "SSTFile.h"
#include <vector>

namespace sstrunner {

using namespace moira;

constexpr u32 ADDR_MASK = 0xFFFFFF;
constexpr u32 HANDLER = 0x600000;       // Bus error handler

// A flat 16 MB memory that can be reset quickly
struct Memory {

    std::vector<u8> ram = std::vector<u8>(1 << 24);
    std::vector<u32> touched;
    bool installVectors = true;            // Install the vectors of the bus error and address error handler?
    bool prefetchInRam = false;            // Store the prefetch queue in memory? (needed by cores without a queue)

    u8 peek(u32 a) const { return ram[a & ADDR_MASK]; }
    u16 peek16(u32 a) const { return u16(peek(a) << 8 | peek(a + 1)); }
    void poke(u32 a, u8 v) { ram[a & ADDR_MASK] = v; touched.push_back(a & ADDR_MASK); }
    void clear() { for (auto a : touched) ram[a] = 0; touched.clear(); }

    void load(const sst::State &s)
    {
        clear();
        for (auto &[addr, val] : s.ram) poke(addr, val);

        // The queue holds the opcode and the next extension word (the PC points behind them)
        if (prefetchInRam) {
            u32 pc = s.pc - 4;
            poke(pc, u8(s.prefetch[0] >> 8)); poke(pc + 1, u8(s.prefetch[0]));
            poke(pc + 2, u8(s.prefetch[1] >> 8)); poke(pc + 3, u8(s.prefetch[1]));
        }

        if (!installVectors) return;

        // Install the bus error vector
        for (int i = 0; i < 4; i++) poke(8 + i, u8(HANDLER >> (24 - 8 * i)));

        // Install the address error vector
        for (int i = 0; i < 4; i++) poke(12 + i, u8((HANDLER + 0x100) >> (24 - 8 * i)));
    }
};

// A bus access as seen by the memory
struct Access {

    i64 cycle;
    bool write;
    u32 addr;           // Word address
    u16 mask;           // Selected byte lanes
    u16 data;
    int fc;
};

// Decides whether a bus access fails
struct Fault {

    int index = -1;     // Index of the failing access (-1 = none)
    int count = 0;      // Number of accesses so far

    bool hit() { return count++ == index; }
};

}
