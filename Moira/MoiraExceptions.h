// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/* Creates a stack frame of a certain format
 *
 * Format  Description                  68000  68010  68020
 * ----------------------------------------------------------------------------
 *  AEBE   Address error / Bus error      X
 *  0000   Short format                   X      X      X
 *  0001   Throwaway                                    X
 *  0010   Instruction exception                        X
 *  1000   Bus fault                             X
 *  1001   Coprocessor mid-instruction                  X
 *  1010   Short bus fault                              X
 *  1011   Long bus fault                               X
 */
template <Core C> void writeStackFrameAEBE(StackFrame &frame);
template <Core C> void writeStackFrame0000(u16 sr, u32 pc, u16 nr);
template <Core C> void writeStackFrame0001(u16 sr, u32 pc, u16 nr);
template <Core C> void writeStackFrame0010(u16 sr, u32 pc, u32 ia, u16 nr);
template <Core C> void writeStackFrame1000(u16 sr, u32 pc, u16 nr);
template <Core C> void writeStackFrame1001(u16 sr, u32 pc, u32 ia, u16 nr);
template <Core C> void writeStackFrame1010(u16 sr, u32 pc, u16 nr);
template <Core C> void writeStackFrame1011(u16 sr, u32 pc, u16 nr);

// Emulates an exception
void execException(ExceptionType exc, int nr = 0);
template <Core C> void execException(ExceptionType exc, int nr = 0);

// Emulates an address error exception
void execAddressError(StackFrame frame, int delay = 0);
template <Core C> void execAddressError(StackFrame frame, int delay = 0);

// Emulates the execution of unimplemented and illegal instructions (DEPRECATED)
void execUnimplemented(int nr);
template <Core C> void execUnimplemented(int nr);

// Emulates an interrupt exception
void execIrqException(u8 level);
template <Core C> void execIrqException(u8 level);
