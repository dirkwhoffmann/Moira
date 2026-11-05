// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Moira.h"

namespace runner {

class TestCPU : public MOIRA_NAMESPACE::Moira {

public:

    // Grant the test runner access to the FPU state
    using MOIRA_NAMESPACE::Moira::fpu;

private:

    void sync(int cycles) override;
    u8 read8(u32 addr) const override;
    u16 read16(u32 addr) const override;
    u32 read32(u32 addr) const override;
    u16 read16OnReset(u32 addr) const override;
    u16 read16Dasm(u32 addr) const override;
    void write8 (u32 addr, u8  val) const override;
    void write16 (u32 addr, u16 val) const override;
    void write32 (u32 addr, u32 val) const override;
    u16 readIrqUserVector(u8 level) const override;
    void didReachBreakpoint(u32 addr) override;
    void didReachWatchpoint(u32 addr) override;
};

}
