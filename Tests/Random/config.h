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

// Set to true to verify all memory write operations
static const bool CHECK_MEM_WRITES = true;

// The runner is built in two flavors: randomized execution tests (RUNNER_STAGES_RANDOM)
// and disassembler tests (RUNNER_STAGES_DASM)
#if defined(RUNNER_STAGES_DASM)
static const bool EXEC_STAGES = false;
static const bool DASM_STAGES = true;
#else
static const bool EXEC_STAGES = true;
static const bool DASM_STAGES = false;
#endif

// Set to true to execute CPU tests
static const bool CHECK_CPU = EXEC_STAGES;

// Set to true to execute MMU tests for capable CPUs
static const bool CHECK_MMU = EXEC_STAGES;

// Set to true to execute FPU tests for capable CPUs
static const bool CHECK_FPU = EXEC_STAGES;

// Set to true to run the disassembler tests
static const bool CHECK_DASM = DASM_STAGES;

// Set to true to skip the execution of illegal instructions
static const bool SKIP_ILLEGAL = true;

// Set to true for verbose output
static const bool VERBOSE = false;

// Change to limit the range of executed instructions
#define doExec(opcode) ((opcode) <= 0xEFFF || isFpuOpcode(opcode) || isMmuOpcode(opcode))

// Change to limit the range of disassembled instructions
#define doDasm(opcode) (opcode >= 0x0000 && opcode <= 0xFFFF)

// Uncomment to check against M68K in Motorola syntax
#define MOTOROLA

// Uncomment to create M68881 compatible disassembler output
// #define ENABLE_M68881

#include <assert.h>
