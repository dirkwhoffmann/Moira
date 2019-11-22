// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void dasmIllegal(uint16_t opcode);
template<int size, int mode> void dasmRegShift(uint16_t opcode);
template<int size, int mode> void dasmImmShift(uint16_t opcode);
