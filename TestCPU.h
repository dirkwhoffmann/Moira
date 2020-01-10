// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef TESTCPU_H
#define TESTCPU_H

#include "Moira.h"

using namespace moira;

class TestCPU : public Moira {

    void sync(int cycles) override;
    u8 read8(u32 addr) override;
    u16 read16(u32 addr) override;
    u16 read16OnReset(u32 addr) override;
    u16 read16Dasm(u32 addr) override;
    void write8 (u32 addr, u8  val) override;
    void write16 (u32 addr, u16 val) override;
    void willPollIrq() override;
    int readIrqUserVector(u8 level) override;
    void breakpointReached(u32 addr) override;
    void watchpointReached(u32 addr) override;
};

#endif
