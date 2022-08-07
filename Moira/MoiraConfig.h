// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

/* Set to true to enable precise timing mode (68000 and 68010 only)
 *
 * If disabled, Moira calls function 'sync' at the end of each instruction
 * with the number of elapsed cycles as a parameter. In precise timing mode,
 * 'sync' is called prior to each memory access. This enables to client to
 * emulate the surrounding hardware up the point where the memory access
 * actually happens.
 *
 * Enable to improve emulation compatibility, disable to gain speed.
 */
#define PRECISE_TIMING false

/* Set to true to enable address error checking.
 *
 * The Motorola 68k signals an address error violation if a odd memory location
 * is addressed in combination with word or long word addressing.
 *
 * Enable to improve emulation compatibility, disable to gain speed.
 */
#define EMULATE_ADDRESS_ERROR false

/* Set to true to emulate the function code pins FC0 - FC2.
 *
 * Whenever memory is accessed, the function code pins enable external hardware
 * to inspect the access type. If used, these pins are usually connected to an
 * external memory management unit (MMU).
 *
 * Enable to improve emulation compatibility, disable to gain speed.
 */
#define EMULATE_FC true

/* Set to true to enable the disassembler.
 *
 * The disassembler requires a jump table which consumes about 1MB of memory.
 * By disabling the disassembler, you can save this amount of memory.
 */
#define ENABLE_DASM true

/* Set to true to build the InstrInfo lookup table.
 *
 * The info table stores information about the instruction (Instr I), the
 * addressing mode (Mode M), and the size attribute (Size S) for all 65536
 * instruction words. The table is meant to provide data for, e.g., external
 * debuggers. It is not needed by Moira itself and therefore disabled by
 * default.
 */
#define BUILD_INSTR_INFO_TABLE true

/* Set to true to run Moira in a special Musashi compatibility mode.
 *
 * The compatibility mode is used by the test runner application to compare
 * the results computed by Moira and Musashi, respectively.
 *
 * Disable to improve emulation compatibility.
 */
#define MIMIC_MUSASHI true

/* Execution debugging
 *
 * This macro is evaluated at the beginning of the execution handlers of all
 * instructions. It is intended to invoke debug code. Make sure to define it
 * as an empty macro in release builds.
 */
#define EXEC_DEBUG { printf("%s(%d,%d,%d,%d)\n", __func__, C, I, M, S); }

// #define NDEBUG true
