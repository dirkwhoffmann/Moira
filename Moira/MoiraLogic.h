// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------


// Reads a value from memory
template<Size S> u32 read(u32 addr);

// Writes a value to memory
template<Size S> void write(u32 addr, u32 value);

// Computea an effective address
template<Mode M, Size S> u32 computeEA(u32 n, u32 dis = 0, u32 idx = 0);

// Reads a value from an effective address
template<Mode M, Size S> u32 readEA(u32 n, u32 dis = 0, u32 idx = 0);


// Read immediate value
template<Size S> u32 readImm();

template<Instr I, Size S> u32 shift(int cnt, u64 data);
template<Instr I, Size S> u32 arith(u32 op1, u32 op2);
