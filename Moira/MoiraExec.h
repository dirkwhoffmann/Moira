// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void execAddressError(u32 addr);
void execLineA(u16 opcode);
void execLineF(u16 opcode);
void execIllegal(u16 opcode);
void execGroup0Exception(u32 addr, u8 nr);
void execGroup1Exception(u8 nr);

template<Instr I, Mode M, Size S> void execShift(u16 opcode);

template<Instr I, Mode M> void execAbcd(u16 opcode);

template<Instr I, Mode M, Size S> void execAddXXRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAddRgXX(u16 opcode);

template<Instr I, Mode M, Size S> void execAndXXRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAndRgXX(u16 opcode);

template<Instr I, Mode M> void execBitDxDy(u16 opcode);
template<Instr I, Mode M> void execBitDxEa(u16 opcode);
template<Instr I, Mode M> void execBitImDy(u16 opcode);
template<Instr I, Mode M> void execBitImEa(u16 opcode);

template<Instr I, Mode M, Size S> void execClr(u16 opcode);
template<Cond C> void execDbcc(u16 opcode);
template<Size S> void execExt(u16 opcode);

template<Mode M> void execLea(u16 opcode);
template<Mode M, Size S> void execMovea(u16 opcode);
void execMoveq(u16 opcode);
template<Instr I, Mode M> void execMulDiv(u16 opcode);
template<Mode M> bool mulDivOp(u16 src, u16& result);
template<Mode M> void execNbcd(u16 opcode);
void execNop(u16 opcode);
template<Mode M, Size S> void execTst(u16 opcode);

