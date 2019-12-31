// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// Saves information to stack for group 0 exceptions
void saveToStackDetailed(u16 sr, u32 addr, u16 code);

// Saves information to stack for group 1 and group 2 exceptions
void saveToStackBrief(u16 sr);

// Emulates an address error
void execAddressError(u32 addr);

// Emulates the execution of unimplemented and illegal instructions
void execUnimplemented(Exception e);
void execLineA(u16 opcode) { execUnimplemented(LINE_A_INSTRUCTION); }
void execLineF(u16 opcode) { execUnimplemented(LINE_F_INSTRUCTION); }
void execIllegal(u16 opcode);

// Emulates a trap or priviledge exception
void execTrapException(Exception e);
void execPrivilegeException();

template<Instr I, Mode M, Size S> void execShiftRg(u16 opcode);
template<Instr I, Mode M, Size S> void execShiftIm(u16 opcode);
template<Instr I, Mode M, Size S> void execShiftEa(u16 opcode);

template<Instr I, Mode M, Size S> void execAbcd(u16 opcode);

template<Instr I, Mode M, Size S> void execAddEaRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAddRgEa(u16 opcode);

template<Instr I, Mode M, Size S> void execAdda(u16 opcode);
template<Instr I, Mode M, Size S> void execAddiRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAddiEa(u16 opcode);
template<Instr I, Mode M, Size S> void execAddqDn(u16 opcode);
template<Instr I, Mode M, Size S> void execAddqAn(u16 opcode);
template<Instr I, Mode M, Size S> void execAddqEa(u16 opcode);
template<Instr I, Mode M, Size S> void execAddxRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAddxEa(u16 opcode);

template<Instr I, Mode M, Size S> void execAndEaRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAndRgEa(u16 opcode);
template<Instr I, Mode M, Size S> void execAndiRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAndiEa(u16 opcode);
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
template<Instr I, Mode M, Size S> void execCmpiRg(u16 opcode);
template<Instr I, Mode M, Size S> void execCmpiEa(u16 opcode);
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
template<Instr I, Mode M, Size S> void execMove0(u16 opcode);
template<Instr I, Mode M, Size S> void execMove2(u16 opcode);
template<Instr I, Mode M, Size S> void execMove3(u16 opcode);
template<Instr I, Mode M, Size S> void execMove4(u16 opcode);
template<Instr I, Mode M, Size S> void execMove5(u16 opcode);
template<Instr I, Mode M, Size S> void execMove6(u16 opcode);
template<Instr I, Mode M, Size S> void execMove7(u16 opcode);
template<Instr I, Mode M, Size S> void execMove8(u16 opcode);
template<Instr I, Mode M, Size S> void execMovea(u16 opcode);
template<Instr I, Mode M, Size S> void execMovemEaRg(u16 opcode);
template<Instr I, Mode M, Size S> void execMovemRgEa(u16 opcode);
template<Instr I, Mode M, Size S> void execMovepDxEa(u16 opcode);
template<Instr I, Mode M, Size S> void execMovepEaDx(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveq(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveToCcr(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveFromSrRg(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveFromSrEa(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveToSr(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveUspAn(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveAnUsp(u16 opcode);
template<Instr I, Mode M, Size S> void execMul(u16 opcode);
template<Instr I, Mode M, Size S> void execDiv(u16 opcode);

template<Instr I, Mode M, Size S> void execNbcd(u16 opcode);
template<Instr I, Mode M, Size S> void execNegRg(u16 opcode);
template<Instr I, Mode M, Size S> void execNegEa(u16 opcode);
template<Instr I, Mode M, Size S> void execNop(u16 opcode);

template<Instr I, Mode M, Size S> void execPea(u16 opcode);

template<Instr I, Mode M, Size S> void execReset(u16 opcode);
template<Instr I, Mode M, Size S> void execRte(u16 opcode);
template<Instr I, Mode M, Size S> void execRtr(u16 opcode);
template<Instr I, Mode M, Size S> void execRts(u16 opcode);

template<Instr I, Mode M, Size S> void execSccRg(u16 opcode);
template<Instr I, Mode M, Size S> void execSccEa(u16 opcode);
template<Instr I, Mode M, Size S> void execStop(u16 opcode);
template<Instr I, Mode M, Size S> void execSwap(u16 opcode);

template<Instr I, Mode M, Size S> void execTasRg(u16 opcode);
template<Instr I, Mode M, Size S> void execTasEa(u16 opcode);
template<Instr I, Mode M, Size S> void execTrap(u16 opcode);
template<Instr I, Mode M, Size S> void execTrapv(u16 opcode);
template<Instr I, Mode M, Size S> void execTst(u16 opcode);

template<Instr I, Mode M, Size S> void execUnlk(u16 opcode);


