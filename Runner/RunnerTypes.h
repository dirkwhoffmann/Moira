// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

// Types shared between the test runner and the Moira adapters.
//
// This header must not include any Moira header. The runner is compiled once
// and talks to every Moira configuration (UAE, Musashi, ...) through the
// MoiraAdapter interface below. Each configuration implements the adapter in a
// translation unit of its own (see MoiraAdapter.cpp).

#include "Shared.h"
#include <ctime>
#include <memory>
#include <string>

namespace runner {

// A floating-point register in its raw 80-bit representation
struct FpReg {

    u16 se;     // Sign and exponent
    u64 man;    // Mantissa

    bool operator==(const FpReg &rhs) const { return se == rhs.se && man == rhs.man; }
};

// A test setup
struct Setup {

    long    round;
    u8      ccr;
    bool    supervisor;
    u32     pc;
    u16     opcode;
    u16     ext1;
    u16     ext2;
    u16     ext3;
    u32     d[8];
    u32     a[8];
    u16     vbr;
    u16     sfc;
    u16     dfc;
    u32     cacr;
    u32     caar;
    FpReg   fpr[8];
    u32     fpcr;
    u32     fpsr;
    u32     fpiar;
    u8      mem[0x10000];
};

// A test result
struct Result {

    u16     dasmResult;
    u32     oldpc;
    u16     opcode;
    u32     pc;
    u32     usp;
    u32     isp;
    u32     msp;
    u32     d[8];
    u32     a[8];
    u16     sr;
    u32     fc;
    u32     vbr;
    u32     sfc;
    u32     dfc;
    u32     cacr;
    u32     caar;
    FpReg   fpr[8];
    u32     fpcr;
    u32     fpsr;
    u32     fpiar;
    char    dasmMoira[128];
    int     dasmCntMoira;
    char    dasmBinutils[128];
    int     dasmCntBinutils;
    char    dasmMusashi[128];
    int     dasmCntMusashi;
    char    dasmUAE[128];
    int     dasmCntUAE;
    char    dasmMoiraUAE[128];      // Moira's output used for the comparison with UAE
    int     dasmCntMoiraUAE;
    int     cycles;
    clock_t elapsed[2];
};

// Location of the tested instruction in memory
const u32 pc = 0x1000;

// Static information about an opcode (the values of Moira's Instr, Size and Mode)
struct InstrInfo { int I; int S; int M; };

// The part of a Moira core that the test runner needs. Enumerations are passed
// as their integer values, because every configuration has its own set of types.
class MoiraAdapter {

public:

    virtual ~MoiraAdapter() = default;

    // Describes the tested core
    virtual std::string build() const = 0;
    virtual int compat() const = 0;                 // Value of Moira's Compatibility enum

    // Selects the CPU model (value of Moira's Model enum)
    virtual void setModel(int model) = 0;
    virtual u32 cacrMask() const = 0;
    virtual bool hasMMU() const = 0;
    virtual bool hasFPU() const = 0;
    virtual InstrInfo getInstrInfo(u16 op) const = 0;

    // Brings the CPU into the state described by a test setup
    virtual void reset(const Setup &s) = 0;
    virtual void resetFpu(const Setup &s) = 0;

    // Disassembles and executes the instruction at the current PC
    virtual void run(Result &r) = 0;
    virtual void recordFpu(Result &r) = 0;
};

}
