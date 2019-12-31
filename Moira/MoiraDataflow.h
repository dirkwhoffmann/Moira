// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/* Moira utilizes a three layer design to model dataflow inside the processor.
 *
 * Layer 1: Main entry points. Most instruction execution handlers call a
 *          layer 1 function to read or write their operands.
 *
 * Layer 2: Data providers. This layer provides functions for moving data back
 *          and forth between various locations.
 *
 * Layer 3: Memory interface. The functions from this layer perform the actual
 *          memory access.
 *
 * The following picture depicts the interplay between the different layers:
 *
 *                                      |
 * - - - - - - - - - - - - - - - - - - -|- - - - - - - - - - - - - - - - - - - -
 * Layer 1:                             V
 *                                    readOp
 *                                   (writeOp)
 *                                      |
 *  Addressing Mode M = 0---1---2---3---4---5---6---7---8---9---A---B
 *                     /    |   |   |   |   |   |   |   |   |   |    \
 *                    /     |   ---------------------------------     \
 *                   /      |           |                              \
 *                  /       |           V                               \
 *                 /        |       computeEA -----------                \
 *                /         |           |               |                 \
 * - - - - - - - -|- - - - -|- - - - - -|- - - - - - - -|- - - - - - - - -|- - -
 * Layer 2:       |         |           |               V                 |
 *                |         |           |       readExtensionWord         |
 *                V         V           V                                 V
 *              readD     readA      readM -----> addressError          readI
 *             (writeD)  (writeA)   (writeM)
 *                                      |
 *                                  updateAn()
 *                                      |
 * - - - - - - - - - - - - - - - - - - -|- - - - - - - - - - - - - - - - - - - -
 * Layer 3:                             |
 *                 Size S = B-----------W-----------L
 *                          |           |           |
 *                          V           V           V
 *                        read8       read16    2 x read16
 *                       (write8)    (write8)  (2 x write16)
 *
 */

/* Reads an operand
 *
 * The source of the operand is determined by the addressing mode M. If the
 * operand is read from memory, the effective address is stored in variable ea.
 * If the source is a register or an immediate value, variable ea remains
 * untouched.
 */
template<Mode M, Size S> bool readOp(int n, u32 &ea, u32 &result);

/* Writes an operand
 *
 * If parameter ea is omitted, the destination of the operand is determined
 * by the addressing mode M. Parameter 'last' indicates if this function is
 * initiates the last memory bus cycle of an instruction.
 */
template<Mode M, Size S, bool last = false> bool writeOp(int n, u32 val);
template<Mode M, Size S, bool last = false> void writeOp(int n, u32 ea, u32 val);
