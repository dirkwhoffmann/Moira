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

template<Instr I, Mode M, Size S> void dasmShift(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M> void dasmAbcd(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmAddXXRg(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmAddRgXX(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmAndXXRg(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmAndRgXX(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M> void dasmBitDxDy(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M> void dasmBitDxEa(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M> void dasmBitImDy(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M> void dasmBitImEa(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmClr(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Cond C> void dasmDbcc(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Size S> void dasmExt(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Mode M> void dasmLea(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Mode M, Size S> void dasmMovea(StrWriter &str, u16 op, u16 e1, u16 e2);
void dasmMoveq(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M> void dasmMulDiv(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Mode M> void dasmNbcd(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmNegNotDx(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmNegNotEa(StrWriter &str, u16 op, u16 e1, u16 e2);
void dasmNop(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Cond C, Mode M> void dasmSccDn(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Cond C, Mode M> void dasmSccEa(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Mode M, Size S> void dasmTst(StrWriter &str, u16 op, u16 e1, u16 e2);
