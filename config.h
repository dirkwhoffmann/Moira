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
// #define CPUTYPE M68K_CPU_TYPE_68020
static constexpr int CPUTYPE = M68K_CPU_TYPE_68000;

// Set to true for verbose output
// #define VERBOSE false
static constexpr bool VERBOSE = false;

// Set to true to verify all memory write operations
// #define CHECK_MEM_WRITES true
static constexpr bool CHECK_MEM_WRITES = true;

// Number of instructions that are executed in a row
// #define RUNS 1
static constexpr int RUNS = 1;
