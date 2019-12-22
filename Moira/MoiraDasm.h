// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

//
// Assembling operands
//

template <Size S> u32 dasmRead(u32 &addr);
int baseDispWords(u16 ext);
int outerDispWords(u16 ext);
template <Mode M, Size S> Ea<M,S> Op(u16 reg, u32 &pc);

void dasmLineA(StrWriter &str, u32 &addr, u16 op);
void dasmLineF(StrWriter &str, u32 &addr, u16 op);
void dasmIllegal(StrWriter &str, u32 &addr, u16 op);

//
// Printing instructions
//

template<Instr I, Mode M, Size S> void dasmShiftRg(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmShiftIm(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmShiftEa(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmAbcd(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAddEaRg(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAddRgEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAdda(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAddiRg(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAddiEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAddqDn(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAddqAn(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAddqEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAddxRg(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAddxEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAndEaRg(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAndRgEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAndi(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAndiccr(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmAndisr(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmBcc(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmBitDxEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmBitImEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmBsr(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmChk(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmClr(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmCmp(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmCmpa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmCmpi(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmCmpm(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmDbcc(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmExgDxDy(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmExgAxDy(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmExgAxAy(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmExt(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmJmp(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmJsr(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmLea(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmLink(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M1, Mode M2, Size S> void dasmMove(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMove0(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMove2(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMove3(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMove4(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMove5(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMove6(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMove7(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMove8(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMovea(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMovemEaRg(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMovemRgEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMovepDxEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMovepEaDx(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMoveq(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMoveToCcr(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMoveFromSr(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMoveToSr(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMoveUspAn(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMoveAnUsp(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmMul(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmDiv(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmNbcd(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmNegRg(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmNegEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmNop(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmPea(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmReset(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmRte(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmRtr(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmRts(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmSccRg(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmSccEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmStop(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmSwap(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmTasRg(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmTasEa(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmTrap(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmTrapv(StrWriter &str, u32 &addr, u16 op);
template<Instr I, Mode M, Size S> void dasmTst(StrWriter &str, u32 &addr, u16 op);

template<Instr I, Mode M, Size S> void dasmUnlk(StrWriter &str, u32 &addr, u16 op);

