// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <stddef.h>

/* UAE's CPU core (GPLv3) wrapped as a reference for the test runner.
 *
 * The core is compiled from the UAE source tree (see UAE_DIR in
 * CMakeLists.txt). Its own headers are only visible inside UAECpu.cpp,
 * so this interface consists of plain types. The CPU runs on a flat 64 KB
 * memory area provided by the caller. Write accesses are reported through a
 * callback.
 */
namespace uaecpu {

struct State {

    uint32_t d[8] = {};
    uint32_t a[7] = {};         // A0 - A6
    uint32_t usp = 0, isp = 0, msp = 0;
    uint32_t pc = 0;
    uint16_t sr = 0;
    uint32_t vbr = 0, sfc = 0, dfc = 0, cacr = 0, caar = 0;

    // Floating-point registers (sign/exponent and mantissa)
    uint16_t fpse[8] = {};
    uint64_t fpman[8] = {};
    uint32_t fpcr = 0, fpsr = 0, fpiar = 0;

    uint32_t a7 = 0;            // Active stack pointer (output only)
};

enum class Cpu { M68000, M68010, M68020, M68EC020, M68030, M68EC030, M68040, M68LC040, M68EC040 };

// Write observer: size is 1 or 2 bytes, fc is the function code of the access
typedef void (*WriteHook)(int size, uint32_t addr, uint32_t value, uint32_t fc, uint64_t cycle);

// Selects the CPU model and the memory area (65536 bytes)
void init(Cpu model, uint8_t *memory, WriteHook hook);

// Loads and reads back the processor state
void setState(const State &s);
State getState();

// Executes a single instruction (or exception). Returns false if the CPU halted.
bool step();

// Disassembles the instruction at addr in the memory area (UAE's disassembler).
// The text has no address and no instruction words (lowercase, '$' for hex).
// Returns the instruction length in bytes.
int disassemble(uint32_t addr, char *out, size_t size);

// Vector number of the last exception taken by the last step (0 if none)
int lastException();

// Number of CPU cycles consumed by the last step (in UAE's cycle units)
uint64_t cycles();

}
