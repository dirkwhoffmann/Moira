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
void privilegeException() { execGroup1Exception(8); }

template<Instr I, Mode M, Size S> void execShiftRg(u16 opcode);
template<Instr I, Mode M, Size S> void execShiftIm(u16 opcode);
template<Instr I, Mode M, Size S> void execShift(u16 opcode);

template<Instr I, Mode M, Size S> void execAbcd(u16 opcode);

template<Instr I, Mode M, Size S> void execAddEaRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAddRgEa(u16 opcode);

template<Instr I, Mode M, Size S> void execAdda(u16 opcode);

template<Instr I, Mode M, Size S> void execAndEaRg(u16 opcode);
template<Instr I, Mode M, Size S> void execAndRgEa(u16 opcode);

template<Instr I, Mode M, Size S> void execBitDxEa(u16 opcode);
template<Instr I, Mode M, Size S> void execBitImEa(u16 opcode);

template<Instr I, Mode M, Size S> void execClr(u16 opcode);
template<Instr I, Mode M, Size S> void execCmp(u16 opcode);
template<Instr I, Mode M, Size S> void execCmpa(u16 opcode);

template<Instr I, Mode M, Size S> void execDbcc(u16 opcode);
template<Instr I, Mode M, Size S> void execExt(u16 opcode);

template<Instr I, Mode M, Size S> void execLea(u16 opcode);
template<Instr I, Mode M1, Mode M2, Size S> void execMove(u16 opcode);
template<Instr I, Mode M, Size S> void execMovea(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveq(u16 opcode);
template<Instr I, Mode M, Size S> void execMoveUsp(u16 opcode);
template<Instr I, Mode M, Size S> void execMulDiv(u16 opcode);

template<Instr I, Mode M, Size S> void execNbcd(u16 opcode);
template<Instr I, Mode M, Size S> void execNegNot(u16 opcode);
template<Instr I, Mode M, Size S> void execNop(u16 opcode);

template<Instr I, Mode M, Size S> void execScc(u16 opcode);

template<Instr I, Mode M, Size S> void execTas(u16 opcode);
template<Instr I, Mode M, Size S> void execTst(u16 opcode);

