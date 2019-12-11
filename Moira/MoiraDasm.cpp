// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "StringWriter.cpp"

template <> u32
CPU::dasmRead<Word>(u32 &addr)
{
    u32 result = memory->moiraSpyRead16(addr + 2);
    addr += 2;
    return result;
}

template <> u32
CPU::dasmRead<Long>(u32 &addr)
{
    u32 result = dasmRead<Word>(addr) << 16;
    result |= dasmRead<Word>(addr);
    return result;
}

template <Mode M, Size S> Ea<M,S>
CPU::makeOp(u32 &addr, u16 reg)
{
    Ea<M,S> result; //  = Ea<M,S> { reg, 0, 0 };

    result.reg = reg;

    switch (M)
    {
        case 5:  // (d,An)
        case 6:  // (d,An,Xi)
        case 7:  // ABS.W
        case 9:  // (d,PC)
        case 10: // (d,PC,Xi)
        {
            result.ext1 = dasmRead<Word>(addr);
            result.ext2 = 0;
            break;
        }
        case 8:  // ABS.L
        {
            result.ext1 = dasmRead<Word>(addr);
            result.ext2 = dasmRead<Word>(addr);
            break;
        }
        case 11: // Imm
        {
            result.ext1 = dasmRead<Word>(addr);
            result.ext2 = (S == Long) ? dasmRead<Word>(addr) : 0;
            break;
        }
        default:
        {
            break;
        }
    }
    return result;
}

void
CPU::dasmIllegal(StrWriter &str, u32 addr, u16 op)
{
    str << "ILLEGAL";
}

void
CPU::dasmLineA(StrWriter &str, u32 addr, u16 op)
{
    str << "LINE A";
}

void
CPU::dasmLineF(StrWriter &str, u32 addr, u16 op)
{
    str << "LINE F";
}

template<Instr I, Mode M, Size S> void
CPU::dasmShiftRg(StrWriter &str, u32 addr, u16 op)
{
    Dn dst { _____________xxx(op) };
    Dn src { ____xxx_________(op) };
    
    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmShiftIm(StrWriter &str, u32 addr, u16 op)
{
    Dn dst { _____________xxx(op) };
    u8 src = ____xxx_________(op);
    if (src == 0) src = 8;

    str << Ins<I>{} << Sz<S>{} << tab << "#" << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmShift(StrWriter &str, u32 addr, u16 op)
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
            printf("DEFAULT\n");
            Ea<M,S> src = makeOp<M,S>(addr, ____xxx_________(op));
            str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
        }
    }
}

template<Instr I, Mode M, Size S> void
CPU::dasmAbcd(StrWriter &str, u32 addr, u16 op)
{
    assert(M == 0 || M == 4);

    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    Ea<M,Long> dst = makeOp<M,Long>(addr, ____xxx_________(op));

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddEaRg(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    Dn      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddRgEa(StrWriter &str, u32 addr, u16 op)
{
    Dn      src { ____xxx_________(op) };
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAndEaRg(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    Dn      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAdda(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    An      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddi(StrWriter &str, u32 addr, u16 op)
{
    Ea<11,S> src = makeOp<11,S>(addr);
    Ea<M, S> dst = makeOp<M, S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddq(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, ____xxx_________(op));
    str << Ins<I>{} << tab << "#" << (u8)op << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddqAn(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, ____xxx_________(op));
    str << Ins<I>{} << tab << "#" << (u8)op << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAndRgEa(StrWriter &str, u32 addr, u16 op)
{
    Dn      src { ____xxx_________(op) };
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAndi(StrWriter &str, u32 addr, u16 op)
{
    Ea<11,S> src = makeOp<11,S>(addr);
    Ea<M, S> dst = makeOp<M, S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmBsr(StrWriter &str, u32 addr, u16 op)
{
    if (S == Word) {
        Ea<11,S> value = makeOp<11,S>(addr);
        str << Ins<I>{} << tab << "#" << value;
    } else {
        str << Ins<I>{} << tab << "#" << Disp8{ (i8)op };
    }
}

template<Instr I, Mode M, Size S> void
CPU::dasmClr(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> dst = makeOp<M,Long>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmCmp(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmCmpa(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmCmpi(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmBcc(StrWriter &str, u32 addr, u16 op)
{
    str << Ins<I>{} << tab << "#" << Disp8{ (i8)op };
}

template<Instr I, Mode M, Size S> void
CPU::dasmBitDxEa(StrWriter &str, u32 addr, u16 op)
{
    Dn         src { ____xxx_________(op) };
    Ea<M,Byte> dst = makeOp<M,Byte>(addr, _____________xxx(op));

    str << Ins<I>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmBitImEa(StrWriter &str, u32 addr, u16 op)
{
    // u8         src = e1 & 0b11111;
    Ea<11,Byte> src = makeOp<11,Byte>(addr);
    Ea<M, Byte> dst = makeOp<M, Byte>(addr, _____________xxx(op));

    src.ext1 &= 0b11111;
    str << Ins<I>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmDbcc(StrWriter &str, u32 addr, u16 op)
{
    Dn src { _____________xxx(op) };

    str << Ins<I>{} << tab << Dn{src} << ", " << irc;
}

template<Instr I, Mode M, Size S> void
CPU::dasmExt(StrWriter &str, u32 addr, u16 op)
{
    Dn src { _____________xxx(op) };

    str << Ins<EXT>{} << Sz<S>{} << tab << Dn{src};
}

template <Instr I, Mode M, Size S> void
CPU::dasmLea(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    An         dst { ____xxx_________(op) };

    str << Ins<LEA>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M1, Mode M2, Size S> void
CPU::dasmMove(StrWriter &str, u32 addr, u16 op)
{
    Ea<M1,S> src = makeOp<M1,S>(addr, _____________xxx(op));
    Ea<M2,S> dst = makeOp<M2,S>(addr, ____xxx_________(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmMovea(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    An         dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmMoveq(StrWriter &str, u32 addr, u16 op)
{
    Dn dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << "#" << (u8)op << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmMoveUsp(StrWriter &str, u32 addr, u16 op)
{
    An reg { _____________xxx(op) };

    if (M == 0) {
        str << Ins<I>{} << tab << "USP," << reg;
    } else {
        str << Ins<I>{} << tab << reg << ",USP";
    }
}

template<Instr I, Mode M, Size S> void
CPU::dasmMulDiv(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Word> src = makeOp<M,Word>(addr, _____________xxx(op));
    Dn         dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << "," << dst;
}

template <Instr I, Mode M, Size S> void
CPU::dasmNbcd(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Byte> dst = makeOp<M,Byte>(addr, _____________xxx(op));

    str << Ins<NBCD>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmNop(StrWriter &str, u32 addr, u16 op)
{
    str << Ins<NOP>{};
}

template<Instr I, Mode M, Size S> void
CPU::dasmScc(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Byte> src = makeOp<M,Byte>(addr, _____________xxx(op));

    str << Ins<I>{} << tab << src;
}

template<Instr I, Mode M, Size S> void
CPU::dasmNegNot(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Byte> dst = makeOp<M,Byte>(addr, _____________xxx(op));
    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmTas(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Byte> dst = makeOp<M,Byte>(addr, _____________xxx(op));
    str << Ins<I>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmTst(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> ea = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<TST>{} << tab << ea;
}

