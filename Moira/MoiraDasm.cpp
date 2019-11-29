// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "StringWriter.cpp"


void
CPU::dasmIllegal(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    str << "ILLEGAL";
}

void
CPU::dasmLineA(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    str << "LINE A";
}

void
CPU::dasmLineF(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    str << "LINE F";
}

template<Instr I, Size S> void 
CPU::dasmRegShift(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    str
    << (const char *)"REG_SHIFT "
    << "size = " << (u8)42
    << "mode = " << (u16)13;
}

template <Instr I, Size S> void
CPU::dasmImmShift(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    str
    << (const char *)"IMM_SHIFT "
    << "size = " << (u8)42
    << "mode = " << (u16)13;
}

template <Instr I, Mode M> void
CPU::dasmEaShift(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    str
    << (const char *)"EA_SHIFT "
    << "size = " << (u8)42
    << "mode = " << (u16)13;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAdd(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    assert(0);
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddXXRg(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> src { _____________xxx(op), e1, e2 };
    Dn      dst { ____xxx_________(op) };

    str << "add" << Sz<S>{} << " " << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddRgXX(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn      src { ____xxx_________(op) };
    Ea<M,S> dst { _____________xxx(op), e1, e2 };

    str << "add" << Sz<S>{} << " " << src << "," << dst;
}

template <Mode M> void
CPU::dasmLea(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    Ea<M,Long> src { _____________xxx(op), ext1, ext2 };
    An         dst { ____xxx_________(op) };

    str << "lea     " << src << ", " << dst;
}
