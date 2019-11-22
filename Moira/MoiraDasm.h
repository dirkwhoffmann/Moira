// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template <class T> void _dasmIllegal(uint16_t opcode, char *str);
void dasmIllegal(uint16_t opcode, char *str, bool hex);

template <class T, Instr I, Size S> void _dasmRegShift(uint16_t opcode, char *str);
template<Instr I, Size S> void dasmRegShift(uint16_t opcode, char *str, bool hex);

template <class T, Instr I, Size S> void _dasmImmShift(uint16_t opcode, char *str);
template<Instr I, Size S> void dasmImmShift(uint16_t opcode, char *str, bool hex);

template <class T, Instr I, Mode M> void _dasmEaShift(uint16_t opcode, char *str);
template<Instr I, Mode M> void dasmEaShift(uint16_t opcode, char *str, bool hex);
