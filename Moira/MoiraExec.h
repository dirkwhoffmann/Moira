// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void execLineA(u16 opcode);
void execLineF(u16 opcode);
void execIllegal(u16 opcode);
void execException(u8 nr);


template<Instr I, Size S> void execRegShift(u16 opcode); // DEPRECATED
template<Instr I, Size S> void execImmShift(u16 opcode); // DEPRECATED
template<Instr I, Mode M> void execEaShift(u16 opcode);  // DEPRECATED

template<Instr I, Mode M, Size S> void execShift(u16 opcode);

template<Instr I, Mode M, Size S> void execAddXXRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAddRgXX(u16 opcode);

template<Mode M> void execLea(u16 opcode);
