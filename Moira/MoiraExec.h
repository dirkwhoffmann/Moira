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
void execTrapException(u8 nr);
void privilegeException() { pc -= 2; execGroup1Exception(8); }

template<Instr I, Mode M, Size S> void execShiftRg(u16 opcode);
template<Instr I, Mode M, Size S> void execShiftIm(u16 opcode);
template<Instr I, Mode M, Size S> void execShift(u16 opcode);

template<Instr I, Mode M, Size S> void execAbcd(u16 opcode);

template<Instr I, Mode M, Size S> void execAddEaRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAddRgEa(u16 opcode);

template<Instr I, Mode M, Size S> void execAdda(u16 opcode);
template<Instr I, Mode M, Size S> void execAddi(u16 opcode);
template<Instr I, Mode M, Size S> void execAddq(u16 opcode);
template<Instr I, Mode M, Size S> void execAddqAn(u16 opcode);
template<Instr I, Mode M, Size S> void execAddxRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAddxEa(u16 opcode);

template<Instr I, Mode M, Size S> void execAndEaRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAndRgEa(u16 opcode);
template<Instr I, Mode M, Size S> void execAndi(u16 opcode);
template<Instr I, Mode M, Size S> void execAndiccr(u16 opcode);
template<Instr I, Mode M, Size S> void execAndisr(u16 opcode);

template<Instr I, Mode M, Size S> void execBcc(u16 opcode);
template<Instr I, Mode M, Size S> void execBitDxEa(u16 opcode);
template<Instr I, Mode M, Size S> void execBitImEa(u16 opcode);
template<Instr I, Mode M, Size S> void execBsr(u16 opcode);

template<Instr I, Mode M, Size S> void execChk(u16 opcode);
template<Instr I, Mode M, Size S> void execClr(u16 opcode);
template<Instr I, Mode M, Size S> void execCmp(u16 opcode);
template<Instr I, Mode M, Size S> void execCmpa(u16 opcode);
template<Instr I, Mode M, Size S> void execCmpi(u16 opcode);
template<Instr I, Mode M, Size S> void execCmpm(u16 opcode);

template<Instr I, Mode M, Size S> void execDbcc(u16 opcode);

template<Instr I, Mode M, Size S> void execExgDxDy(u16 opcode);
template<Instr I, Mode M, Size S> void execExgAxDy(u16 opcode);
template<Instr I, Mode M, Size S> void execExgAxAy(u16 opcode);
template<Instr I, Mode M, Size S> void execExt(u16 opcode);

template<Instr I, Mode M, Size S> void execLink(u16 opcode);

template<Instr I, Mode M, Size S> void execJmp(u16 opcode);
template<Instr I, Mode M, Size S> void execJsr(u16 opcode);

template<Instr I, Mode M, Size S> void execLea(u16 opcode);
template<Instr I, Mode M1, Mode M2, Size S> void execMove(u16 opcode);
template<Instr I, Mode M, Size S> void execMovea(u16 opcode);
template<Instr I, Mode M, Size S> void execMovemEaRg(u16 opcode);
template<Instr I, Mode M, Size S> void execMovemRgEa(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveq(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveToCcr(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveFromSr(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveToSr(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveUsp(u16 opcode);
template<Instr I, Mode M, Size S> void execMulDiv(u16 opcode);

template<Instr I, Mode M, Size S> void execNbcd(u16 opcode);
template<Instr I, Mode M, Size S> void execNegNot(u16 opcode);
template<Instr I, Mode M, Size S> void execNop(u16 opcode);

template<Instr I, Mode M, Size S> void execPea(u16 opcode);

template<Instr I, Mode M, Size S> void execScc(u16 opcode);
template<Instr I, Mode M, Size S> void execSwap(u16 opcode);

template<Instr I, Mode M, Size S> void execTas(u16 opcode);
template<Instr I, Mode M, Size S> void execTst(u16 opcode);

template<Instr I, Mode M, Size S> void execUnlk(u16 opcode);

