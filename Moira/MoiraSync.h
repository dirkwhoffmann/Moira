// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

int syncIllegal(uint16_t opcode, int i);

template<Instr I, Size S> int syncRegShift(uint16_t opcode, int i);
template<Instr I, Size S> int syncImmShift(uint16_t opcode, int i);
template<Instr I, Mode M> int syncEaShift(uint16_t opcode, int i);
