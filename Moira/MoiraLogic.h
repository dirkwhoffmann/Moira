// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------


template <Instr I, Size S> u32  shift(int cnt, u64 data);
template <Instr I, Size S> u32  arith(u32 op1, u32 op2);
template <Instr I, Size S> u32  logic(u32 op1);
template <Instr I, Size S> u32  logic(u32 op1, u32 op2);
template <Instr I>         u32  bitop(u32 op, u8 bit);
template <Size S>          void cmp(u32 op1, u32 op2);
template <Instr I>         bool bcond();
