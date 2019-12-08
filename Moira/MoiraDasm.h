// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void dasmLineA(StrWriter &str, u16 op, u16 ext1, u16 ext2);
void dasmLineF(StrWriter &str, u16 op, u16 ext1, u16 ext2);
void dasmIllegal(StrWriter &str, u16 op, u16 ext1, u16 ext2);

template<Instr I, Mode M, Size S> void dasmShiftRg(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmShiftIm(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmShift(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmAbcd(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmAddEaRg(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmAddRgEa(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmAdda(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmAndEaRg(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmAndRgEa(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmBitDxEa(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmBitImEa(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmClr(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmCmp(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmCmpa(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmDbcc(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmExt(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmLea(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmMovea(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmMoveq(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmMulDiv(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmNbcd(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmNegNot(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmNop(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmSccDn(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmSccEa(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmTas(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmTst(StrWriter &str, u16 op, u16 e1, u16 e2);
