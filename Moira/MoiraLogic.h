// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/*
template<Size> uint32_t clip(uint32_t data);
template<Size> uint32_t mask();
template<Size> bool msbit(uint32_t data);
template<Size> uint32_t sign(uint32_t data);
template<Size> uint8_t bytes();
*/

template<Size size, Instr instr> uint32_t shift(uint32_t data, uint8_t shift);
