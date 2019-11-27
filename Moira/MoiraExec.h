// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void execIllegal(u16 opcode);

template<Instr I, Size S> void execRegShift(u16 opcode);
template<Instr I, Size S> void execImmShift(u16 opcode);
template<Instr I, Mode M> void execEaShift(u16 opcode);

template<Instr I, Mode M, Size S> void execAdd(u16 opcode); // DEPRECATED
template<Instr I, Mode M, Size S> void execAddRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAddEa(u16 opcode);
template<Instr I, Mode M, Size S> void execAddIm(u16 opcode);
template<Mode M> void execLea(u16 opcode);
