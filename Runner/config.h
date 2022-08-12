// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

// Set to true for verbose output
static constexpr bool VERBOSE = false;

// Set to true to verify all memory write operations
static constexpr bool CHECK_MEM_WRITES = true;

// Set to true to skip the execution of illegal instructions
static constexpr bool SKIP_ILLEGAL = true;

// Set to true to measure the CPU speed
static constexpr bool PROFILE_CPU = true;

// Set to true to measure the disassembler speed
static constexpr bool PROFILE_DASM = false;

// Change to limit the range of executed instructions
#define doExec(opcode) (opcode >= 0x0000 && opcode <= 0xEFFF)

// Change to limit the range of disassembled instructions
#define doDasm(opcode) (opcode >= 0x0000 && opcode <= 0xEFFF)

// Uncomment to disable assertion checking
// #define NDEBUG

#include <assert.h>
