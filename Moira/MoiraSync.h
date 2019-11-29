// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

int syncLineA(u16 opcode, int i);
int syncLineF(u16 opcode, int i);
int syncIllegal(u16 opcode, int i);

template<Instr I, Size S> int syncRegShift(u16 opcode, int i);
template<Instr I, Size S> int syncImmShift(u16 opcode, int i);
template<Instr I, Mode M> int syncEaShift(u16 opcode, int i);

template<Mode M> int syncLea(u16 opcode, int i);
