// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void dasmIllegal(StrWriter &str, uint32_t addr, uint16_t opcode);
template<Instr I, Size S> void dasmRegShift(StrWriter &str, uint32_t addr, uint16_t opcode);
template<Instr I, Size S> void dasmImmShift(StrWriter &str, uint32_t addr, uint16_t opcode);
template<Instr I, Mode M> void dasmEaShift(StrWriter &str, uint32_t addr, uint16_t opcode);
template<Mode M> void dasmLea(StrWriter &str, uint32_t addr, uint16_t opcode);
