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

    uint64_t crp = 0, srp = 0;  // 68030 MMU registers
    uint32_t tc = 0, tt0 = 0, tt1 = 0, mmusr = 0;

    uint32_t a7 = 0;            // Active stack pointer (output only)
};

enum class Cpu { M68000, M68010, M68020, M68EC020, M68030, M68EC030, M68040, M68LC040, M68EC040 };
enum class Fpu { None, M68881, M68882, Internal };

// Write observer: size is 1 or 2 bytes, fc is the function code of the access
typedef void (*WriteHook)(int size, uint32_t addr, uint32_t value, uint32_t fc, uint64_t cycle);

// A bus access of the CPU
struct BusAccess {

    bool write;                 // Write access?
    bool fetch;                 // Program fetch?
    uint32_t addr;              // Address (24 bits)
    int size;                   // 1 (byte) or 2 (word)
    uint32_t value;             // Data (valid for writes)
    uint32_t fc;                // Function code
    uint64_t cycle;             // CPU cycle (in UAE's cycle units)
};

// Is called before each CPU bus access. If it returns true, the access fails with a bus error.
// This is supported by the cycle-exact 68000 core only.
typedef bool (*AccessHook)(const BusAccess &access);
void setAccessHook(AccessHook hook);

// Selects the CPU and FPU model and the memory area (65536 bytes)
void init(Cpu model, Fpu fpu, uint8_t *memory, WriteHook hook);

// If disabled, a pending trace exception is not taken at the end of step() (default: enabled)
void setTraceInStep(bool enable);

// Changes the size of the memory area to mask + 1 bytes (e.g. 0xFFFFFF for 16 MB)
void setMemoryMask(uint32_t mask);

// Overwrites the prefetch queue (instruction word and the word following it)
void setPrefetch(uint16_t ir, uint16_t irc);

// Indicates whether the CPU has halted (double fault)
bool halted();

// Loads and reads back the processor state
void setState(const State &s);
State getState();

// Executes a single instruction (or exception). Returns false if the CPU halted.
bool step();

// Disassembles the instruction at addr in the memory area (UAE's disassembler).
// The text has no address and no instruction words (lowercase, '$' for hex).
// Returns the instruction length in bytes.
int disassemble(uint32_t addr, char *out, size_t size);

// Address translation of the 68030 MMU (UAE's table walk, no instruction involved)
void setMMU(uint64_t crp, uint64_t srp, uint32_t tc, uint32_t tt0, uint32_t tt1);
// Enables the 68030 MMU for instruction execution (PMOVE, PTEST, ...)
void enableMMU(bool enable);
void flushATC();
bool translate(uint32_t addr, bool super, bool data, bool write, uint32_t *physical);
bool ptest(uint32_t addr, uint8_t fc, bool write, uint32_t level, uint32_t *mmusr, uint32_t *descr);

// Address translation of the 68040 MMU
void setMMU040(uint32_t urp, uint32_t srp, uint32_t tc, uint32_t itt0, uint32_t itt1, uint32_t dtt0, uint32_t dtt1);
void endMMU040();
void flushATC040();
bool translate040(uint32_t addr, bool super, bool data, bool write, uint32_t *physical);
uint32_t ptest040(uint32_t addr, uint8_t dfc, bool write);

// Vector number of the last exception taken by the last step (0 if none)
int lastException();

// Number of CPU cycles consumed by the last step (in UAE's cycle units)
uint64_t cycles();

}
