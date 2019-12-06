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

template<Instr I, Mode M, Size S> void
CPU::dasmShift(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> src { ____xxx_________(op), e1, e2 };
    Dn      dst { _____________xxx(op) };

    str << Ins<I>{} << Sz<S>{} << " " << src << "," << dst;
}

template<Instr I, Mode M> void
CPU::dasmAbcd(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    switch (M) {

        case 0:
        {
            Dn src { _____________xxx(op) };
            Dn dst { ____xxx_________(op) };

            str << Ins<I>{} << " " << src << ", " << dst;
            break;
        }
        case 4:
        {
            An src { _____________xxx(op) };
            An dst { ____xxx_________(op) };

            str << Ins<I>{} << " -(" << src << "), -(" << dst << ")";
            break;
        }
        default:
        {
            assert(false);
        }
    }
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddXXRg(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> src { _____________xxx(op), e1, e2 };
    Dn      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << " " << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddRgXX(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn      src { ____xxx_________(op) };
    Ea<M,S> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << Sz<S>{} << " " << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAndXXRg(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> src { _____________xxx(op), e1, e2 };
    Dn      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << " " << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAndRgXX(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn      src { ____xxx_________(op) };
    Ea<M,S> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << Sz<S>{} << " " << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmClr(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Long> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << Sz<S>{} << " " << dst;
}

template<Cond C> void
CPU::dasmDbcc(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn src { _____________xxx(op) };

    str << "db" << Cnd<C>{} << " " << Dn{src} << ", " << irc;
}

template<Size S> void
CPU::dasmExt(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn src { _____________xxx(op) };

    str << Ins<EXT>{} << Sz<S>{} << " " << Dn{src};
}

template <Mode M> void
CPU::dasmLea(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Long> src { _____________xxx(op), e1, e2 };
    An         dst { ____xxx_________(op) };

    str << Ins<LEA>{} << src << ", " << dst;
}

template <Mode M> void
CPU::dasmNbcd(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Byte> dst { _____________xxx(op), e1, e2 };

    str << Ins<NBCD>{} << " " << dst;
}

void
CPU::dasmNop(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    str << Ins<NOP>{};
}

template<Mode M, Size S> void
CPU::dasmTst(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> ea { _____________xxx(op), e1, e2 };

    str << Ins<TST>{} << " " << ea;
}

