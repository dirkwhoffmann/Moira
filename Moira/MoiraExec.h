// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void execIllegal(uint16_t opcode);

template<Instr I, Size S> void execRegShift(uint16_t opcode);
template<Instr I, Size S> void execImmShift(uint16_t opcode);
template<Instr I, Mode M> void execEaShift(uint16_t opcode);

template<Mode M> void execLea(uint16_t opcode);
