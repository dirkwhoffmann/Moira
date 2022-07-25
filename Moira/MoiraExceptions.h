// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// Emulates a group 0 exception
void saveToStack(AEStackFrame &frame);
template <Core C> void saveToStack(AEStackFrame &frame);

// Emulates a group 1 or group 2 exception
void saveToStackBrief(u16 nr, u16 sr, u32 pc);
template <Core C> void saveToStackBrief(u16 nr, u16 sr, u32 pc);

// Emulates an address error
void execAddressError(AEStackFrame frame, int delay = 0);
template <Core C> void execAddressError(AEStackFrame frame, int delay = 0);

// Emulates a format error (68010+)
void execFormatError();
template <Core C> void execFormatError();

// Emulates the execution of unimplemented and illegal instructions
void execUnimplemented(int nr);
template <Core C> void execUnimplemented(int nr);

// Emulates a trace exception
void execTraceException();
template <Core C> void execTraceException();

// Emulates a trap exception
void execTrapException(int nr);
template <Core C> void execTrapException(int nr);

// Emulates a priviledge exception
void execPrivilegeException();
template <Core C> void execPrivilegeException();

// Emulates an interrupt exception
void execIrqException(u8 level);
template <Core C> void execIrqException(u8 level);
