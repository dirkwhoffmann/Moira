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

template <class T, int size, int mode> void _dasmRegShift(uint16_t opcode, char *str);
template<int size, int mode> void dasmRegShift(uint16_t opcode, char *str, bool hex);

template <class T, int size, int mode> void _dasmImmShift(uint16_t opcode, char *str);
template<int size, int mode> void dasmImmShift(uint16_t opcode, char *str, bool hex);
