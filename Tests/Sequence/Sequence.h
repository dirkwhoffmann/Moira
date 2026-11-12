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
#include <string>
#include <vector>

namespace sequence {

using moira::u8; using moira::u16; using moira::u32;

constexpr u32 MEM_MASK  = 0x1FFFFF;
constexpr u32 PROG      = 0x1000;       // Start of the test program
constexpr u32 H_SKIP    = 0x2000;       // Handler that skips the faulting instruction
constexpr u32 H_RTE     = 0x2010;       // Handler that returns
constexpr u32 H_LOOP    = 0x2020;       // Handler for bus and address errors (never expected)
constexpr u32 DATA      = 0x8000;       // Memory the program writes to
constexpr u32 SSP       = 0x7000;
constexpr u32 USP       = 0x6000;

struct Write {
    u32 addr; u8 value;
    bool operator<(const Write &o) const { return addr != o.addr ? addr < o.addr : value < o.value; }
    bool operator==(const Write &o) const { return addr == o.addr && value == o.value; }
};

struct Program {

    std::vector<u16> words;
    u32 sr = 0;
    u32 d[8] = {};
    u32 a[7] = {};
    std::vector<std::pair<int, int>> events;    // (step, level)
    bool ack = false;                           // Drop the IPL pins when an interrupt is taken
};

// Register state of a CPU at an instruction boundary
struct RefState {

    u32 d[8] = {}, a[7] = {};
    u32 usp = 0, isp = 0, sp = 0;
    u32 sr = 0, pc = 0;
};

/* The CPU core Moira is compared with. A step is an instruction plus the
 * exceptions that are due after it (trace, interrupt, ...). A stopped CPU
 * executes a step by waiting.
 */
class Reference {

public:

    virtual ~Reference() = default;

    // Name of the core (used in the headline)
    virtual const char *name() = 0;

    // Selects the CPU model ("68000", "68010"). Returns false if it is not supported.
    virtual bool init(const std::string &cpu) = 0;

    // Loads the memory image and the initial state (IPL pins are 0)
    virtual void load(const std::vector<u8> &mem, const Program &prog) = 0;

    // Sets the interrupt priority level on the IPL pins
    virtual void setIPL(int level) = 0;

    // Executes a step
    virtual void step() = 0;

    // Inspects the result of the last step
    virtual RefState state() = 0;
    virtual bool stopped() = 0;
    virtual bool interrupted() = 0;             // Was an interrupt acknowledged?
    virtual bool addressError() = 0;            // Was an address or bus error taken?
    virtual bool unreliable() { return false; } // Did the reference core run into a known defect? (the program ends)
    virtual const std::vector<Write> &writes() = 0;
    virtual std::string disassemble(u32 addr) { (void)addr; return ""; }  // Empty: Moira's disassembler is used
};

// Runs the sequence test with the given reference core
int run(int argc, char **argv, Reference &ref);

}
