// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

// The CPU under test
static constexpr int CPUTYPE = 68020;

// Set to true for verbose output
static constexpr bool VERBOSE = false;

// Set to true to verify all memory write operations
static constexpr bool CHECK_MEM_WRITES = true;

// Change to limit the range of executed instructions
#define doExec(opcode) (opcode > 0xFFFFFFFF)

// Change to limit the range of disassembled instructions
#define doDasm(opcode) (opcode >= 0x0000 && opcode <= 0xEFFF)

// Number of instructions that are executed in a row
static constexpr int RUNS = 1;

// Uncomment to enable assertion checking
#define NDEBUG true

#include <assert.h>
