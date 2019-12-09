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
CPU::dasmShiftRg(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn dst { _____________xxx(op) };
    u8 src = ____xxx_________(op);
    if (src == 0) src = 8;
    
    str << Ins<I>{} << Sz<S>{} << tab << "#" << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmShiftIm(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn dst { _____________xxx(op) };
    u8 src = ____xxx_________(op);
    if (src == 0) src = 8;

    str << Ins<I>{} << Sz<S>{} << tab << "#" << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmShift(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn dst { _____________xxx(op) };

    switch (M) {
        case 11: // Imm
        {
            u8 src = ____xxx_________(op);
            if (src == 0) src = 8;

            str << Ins<I>{} << Sz<S>{} << tab << "#" << src << "," << dst;
            break;
        }
        default:
        {
            Ea<M,S> src { ____xxx_________(op), e1, e2 };
            str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
        }
    }
}

template<Instr I, Mode M, Size S> void
CPU::dasmAbcd(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    assert(M == 0 || M == 4);

    Ea<M,Long> src { _____________xxx(op) };
    Ea<M,Long> dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddEaRg(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> src { _____________xxx(op), e1, e2 };
    Dn      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddRgEa(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn      src { ____xxx_________(op) };
    Ea<M,S> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAndEaRg(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> src { _____________xxx(op), e1, e2 };
    Dn      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAdda(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> src { _____________xxx(op), e1, e2 };
    An      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAndRgEa(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn      src { ____xxx_________(op) };
    Ea<M,S> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmClr(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Long> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmCmp(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmCmpa(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmBitDxEa(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn         src { ____xxx_________(op) };
    Ea<M,Byte> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmBitImEa(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    u8 src = e1 & 0b11111;
    Ea<M,Byte> dst { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << tab << "#" << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmDbcc(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn src { _____________xxx(op) };

    str << Ins<I>{} << tab << Dn{src} << ", " << irc;
}

template<Instr I, Mode M, Size S> void
CPU::dasmExt(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn src { _____________xxx(op) };

    str << Ins<EXT>{} << Sz<S>{} << tab << Dn{src};
}

template <Instr I, Mode M, Size S> void
CPU::dasmLea(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Long> src { _____________xxx(op), e1, e2 };
    An         dst { ____xxx_________(op) };

    str << Ins<LEA>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmMovea(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Long> src { _____________xxx(op), e1, e2 };
    An         dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmMoveq(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Dn dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << "#" << (u8)(op & 0xFF) << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmMulDiv(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Word> src { _____________xxx(op), e1, e2 };
    Dn         dst { ____xxx_________(op) };

    str << Ins<I>{} << src << "," << dst;
}

template <Instr I, Mode M, Size S> void
CPU::dasmNbcd(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Byte> dst { _____________xxx(op), e1, e2 };

    str << Ins<NBCD>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmNop(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    str << Ins<NOP>{};
}

template<Instr I, Mode M, Size S> void
CPU::dasmScc(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Byte> src { _____________xxx(op), e1, e2 };

    str << Ins<I>{} << tab << src;
}

template<Instr I, Mode M, Size S> void
CPU::dasmNegNot(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Byte> dst { _____________xxx(op), e1, e2 };
    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmTas(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,Byte> dst { _____________xxx(op), e1, e2 };
    str << Ins<I>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmTst(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    Ea<M,S> ea { _____________xxx(op), e1, e2 };

    str << Ins<TST>{} << tab << ea;
}

