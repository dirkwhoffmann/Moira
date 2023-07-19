// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

// Number of test rounds
static const long ROUNDS = 1;

// Set to true to verify all memory write operations
static const bool CHECK_MEM_WRITES = true;

// Set to true to execute CPU tests
static const bool CHECK_CPU = true;

// Set to true to execute MMU tests for capable CPUs
static const bool CHECK_MMU = true;

// Set to true to execute FPU tests for capable CPUs
static const bool CHECK_FPU = true;

// Set to true to skip the execution of illegal instructions
static const bool SKIP_ILLEGAL = true;

// Set to true for verbose output
static const bool VERBOSE = false;

// Set to true to measure the CPU speed
static const bool PROFILE_CPU = true;

// Set to true to measure the disassembler speed
static const bool PROFILE_DASM = false;

// Change to limit the range of executed instructions
#define doExec(opcode) (opcode >= 0x0000 && opcode <= 0xEFFF)

// Change to limit the range of disassembled instructions
#define doDasm(opcode) (opcode >= 0x0000 && opcode <= 0xFFFF)

// Uncomment to check against M68K in Motorola syntax
#define MOTOROLA

// Uncomment to create M68881 compatible disassembler output
// #define ENABLE_M68881

// Uncomment to disable assertion checking
// #define NDEBUG

#include <assert.h>
