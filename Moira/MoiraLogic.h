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
u32 computeEA(u32 n, u32 dis = 0, u32 idx = 0);

// Read immediate value
template<Size S> u32 readImm();

template<Size S, Instr I> u32 shift(u32 cnt, u32 data);
template<Size S> u32 add(u32 op1, u32 op2);
