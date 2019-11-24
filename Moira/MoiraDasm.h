// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void
dasmIllegal(StrWriter &str, u16 op, u16 ext1, u16 ext2);
template<Instr I, Size S>
void dasmRegShift(StrWriter &str, u16 op, u16 ext1, u16 ext2);
template<Instr I, Size S>
void dasmImmShift(StrWriter &str, u16 op, u16 ext1, u16 ext2);
template<Instr I, Mode M>
void dasmEaShift(StrWriter &str, u16 op, u16 ext1, u16 ext2);
template<Mode M>
void dasmLea(StrWriter &str, u16 op, u16 ext1, u16 ext2);
