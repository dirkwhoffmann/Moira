// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// Checks for an address error
template<Mode M, Size S> bool addressError(u32 addr);

// Reads a value from memory
template<Size S> u32 read(u32 addr);

// Writes a value to memory
template<Size S> void write(u32 addr, u32 value);

// Writes a value to the stack
void writeToStack(u32 value);

// Computes an effective address
template<Mode M, Size S, u8 flags = 0> u32 computeEA(u32 n);

// Emulates the address register modification for modes (An)+, (An)-
template<Mode M, Size S> void postIncPreDec(int n);

// Reads an operand
template<Mode M, Size S> bool readOperand(int n, u32 &ea, u32 &result);

// Writes an operand
template<Mode M, Size S> bool writeOperand(int n, u32 value);

// Writes an operand to an already computed effective address
template<Mode M, Size S> void writeOperand(int n, u32 ea, u32 value);

// Reads an immediate value
template<Size S> u32 readImm();

template <Instr I, Size S> u32  shift(int cnt, u64 data);
template <Instr I, Size S> u32  arith(u32 op1, u32 op2);
template <Instr I, Size S> u32  logic(u32 op1);
template <Instr I, Size S> u32  logic(u32 op1, u32 op2);
template <Instr I>         u32  bitop(u32 op, u8 bit);
template <Size S>          void cmp(u32 op1, u32 op2);
template <Instr I>         bool bcond();
