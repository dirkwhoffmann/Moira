// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "RefCore.h"

namespace sstrunner {

// Musashi. The core has no bus log and doesn't emulate address errors. It reads the
// instruction from memory (no prefetch queue), so the queue is poked in behind the PC.
struct MusashiCore : RefCore {

    Memory &mem;
    std::vector<mame::BusCycle> cycles;

    explicit MusashiCore(Memory &m);

    void load(const sst::State &s) override;
    int execute() override;
    mame::State state() const override;

    const std::vector<mame::BusCycle> &log() const override { return cycles; }
    bool halted() const override { return false; }
    bool hasBusLog() const override { return false; }

    // Musashi's 68030 and 68040 support is too limited to serve as a reference
    bool supports(CPUModel m) const override
    {
        return m == CPUModel::M68000 || m == CPUModel::M68010 || m == CPUModel::M68EC020 || m == CPUModel::M68020;
    }
    void setModel(CPUModel m) override;

    const char *name() const override { return "Musashi"; }
    const char *shortName() const override { return "Musashi"; }

    // Known functional differences:
    // - Musashi computes the flags of DIVU and DIVS differently. It sets only V on an overflow and leaves
    //   the flags alone on a division by zero (the stacked SR shows up as the first frame word).
    // - LINK A7 pushes the decremented stack pointer in Musashi (the supported models push the old value).
    bool knownDifference(const char *kind, u16 opcode, const std::string &field) const override
    {
        bool div = (opcode & 0xF1C0) == 0x80C0 || (opcode & 0xF1C0) == 0x81C0;
        if (div && (field == "SR" || field == "frame.code" || field == "frame.sr")) return true;

        bool linkA7 = opcode == 0x4E57 || opcode == 0x480F;
        return linkA7 && field == "mem";
    }

    // Musashi executes MOVE16 (F620-F627) on the 68000, which has no such instruction
    bool unsupported(const sst::Test &t) const override { return (t.initial.prefetch[0] & 0xFFF8) == 0xF620; }
    const char *unsupportedNote() const override { return "(Musashi executes MOVE16 on the 68000)"; }
};

}
