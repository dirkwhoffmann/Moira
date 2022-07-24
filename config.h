// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

// CPU under test
static constexpr int CPUTYPE = 68000;

// Set to true for verbose output
static constexpr bool VERBOSE = false;

// Set to true to verify the disassembler output only
static constexpr bool DASM_ONLY = true;

// Set to true to verify all memory write operations
static constexpr bool CHECK_MEM_WRITES = true;

// Number of instructions that are executed in a row
static constexpr int RUNS = 1;

// Uncomment to enable assertion checking
#define NDEBUG true

#include <assert.h>
