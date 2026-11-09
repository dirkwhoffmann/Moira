// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

/* A wrapper around MAME's microcode-based 68000 core.
 *
 * The core is compiled from MAME's unmodified sources (see ../MAME) on top of
 * a minimal device framework (see shim/emu.h). The core runs the original
 * microcode, i.e., it is cycle exact and it implements the real bus error and
 * address error sequences. It serves as the reference for bus error tests.
 *
 * This header does not expose any MAME type.
 */

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace mame {

/// Register state at an instruction boundary
struct State {

    uint32_t d[8] = { };        ///< Data registers
    uint32_t a[7] = { };        ///< Address registers A0 - A6
    uint32_t usp = 0;           ///< User stack pointer
    uint32_t ssp = 0;           ///< Supervisor stack pointer
    uint16_t sr = 0x2700;       ///< Status register
    uint32_t pc = 0;            ///< Address of the instruction to be executed (the word in IRD)
    uint16_t ird = 0;           ///< Prefetch queue: instruction word
    uint16_t irc = 0;           ///< Prefetch queue: word following the instruction word
};

/// A bus cycle as seen by the memory
struct BusCycle {

    int cycle;                  ///< Clock cycle in which the access starts
    bool write;                 ///< Write access?
    uint32_t addr;              ///< Word address (24 bits)
    uint16_t data;              ///< Data
    uint16_t mask;              ///< Byte lanes (0xFFFF, 0xFF00, or 0x00FF)
    int fc;                     ///< Function code
};

class Cpu {

public:

    Cpu();
    ~Cpu();

    //
    // Connecting the CPU to the outside world
    //

    /// Reads a word (or a byte in the lane selected by mask)
    std::function<uint16_t(uint32_t addr, uint16_t mask)> read;

    /// Writes a word (or a byte in the lane selected by mask)
    std::function<void(uint32_t addr, uint16_t data, uint16_t mask)> write;

    /// Is queried for each bus cycle. If it returns true, a bus error occurs.
    std::function<bool(uint32_t addr, bool write, int fc)> busError;

    /// Supplies the interrupt vector (if unset, the autovector is used)
    std::function<uint8_t(int level)> vector;

    //
    // Running the CPU
    //

    /// Sets the register state and fills the prefetch queue
    void load(const State &state);

    /// Executes a single instruction (or exception sequence). Returns the elapsed cycles.
    int execute();

    /// Returns the register state at the end of the last instruction
    State state() const;

    /// Returns the bus cycles of the last instruction
    const std::vector<BusCycle> &log() const;

    /// Indicates whether the CPU stopped because of a double bus fault
    bool halted() const;

    /// Sets the level of the interrupt lines
    void setIPL(int level);

private:

    struct Impl;
    std::unique_ptr<Impl> impl;
};

}
