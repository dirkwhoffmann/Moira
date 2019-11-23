// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------


// Compute effective address
template<Size S, Mode M>
uint32_t computeEA(uint32_t n, uint32_t dis = 0, uint32_t idx = 0);

template<Size size, Instr instr>
uint32_t shift(uint32_t cnt, uint32_t data);
