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

template<Instr I, Mode M, Size S> void dasmAddXXRg(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmAddRgXX(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmAndXXRg(StrWriter &str, u16 op, u16 e1, u16 e2);
template<Instr I, Mode M, Size S> void dasmAndRgXX(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Instr I, Mode M, Size S> void dasmClr(StrWriter &str, u16 op, u16 e1, u16 e2);

template<Mode M> void dasmLea(StrWriter &str, u16 op, u16 e1, u16 e2);
