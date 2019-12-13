// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "StringWriter.cpp"

/*
static u16 swapBits(u16 value) {

    u16 result = 0;
    for (int i = 0; i < 16; i++) {
        result = result << 1 | !!(value & (1 << i));
    }
    return result;
}
*/

template <> u32
Moira::dasmRead<Word>(u32 &addr)
{
    u32 result = memory->moiraSpyRead16(addr + 2);
    addr += 2;
    return result;
}

template <> u32
Moira::dasmRead<Long>(u32 &addr)
{
    u32 result = dasmRead<Word>(addr) << 16;
    result |= dasmRead<Word>(addr);
    return result;
}

template <Mode M, Size S> Ea<M,S>
Moira::makeOp(u32 &addr, u16 reg)
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
Moira::dasmIllegal(StrWriter &str, u32 addr, u16 op)
{
    str << "ILLEGAL";
}

void
Moira::dasmLineA(StrWriter &str, u32 addr, u16 op)
{
    str << "LINE A";
}

void
Moira::dasmLineF(StrWriter &str, u32 addr, u16 op)
{
    str << "LINE F";
}

template<Instr I, Mode M, Size S> void
Moira::dasmShiftRg(StrWriter &str, u32 addr, u16 op)
{
    Dn dst { _____________xxx(op) };
    Dn src { ____xxx_________(op) };
    
    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmShiftIm(StrWriter &str, u32 addr, u16 op)
{
    Dn dst { _____________xxx(op) };
    u8 src = ____xxx_________(op);
    if (src == 0) src = 8;

    str << Ins<I>{} << Sz<S>{} << tab << "#" << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmShift(StrWriter &str, u32 addr, u16 op)
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
Moira::dasmAbcd(StrWriter &str, u32 addr, u16 op)
{
    assert(M == 0 || M == 4);

    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    Ea<M,Long> dst = makeOp<M,Long>(addr, ____xxx_________(op));

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAddEaRg(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    Dn      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAddRgEa(StrWriter &str, u32 addr, u16 op)
{
    Dn      src { ____xxx_________(op) };
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAndEaRg(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    Dn      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAdda(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    An      dst { ____xxx_________(op) };

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAddi(StrWriter &str, u32 addr, u16 op)
{
    Ea<11,S> src = makeOp<11,S>(addr);
    Ea<M, S> dst = makeOp<M, S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAddq(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, ____xxx_________(op));
    str << Ins<I>{} << tab << "#" << (u8)op << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAddqAn(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, ____xxx_________(op));
    str << Ins<I>{} << tab << "#" << (u8)op << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAddxRg(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    Ea<M,S> dst = makeOp<M,S>(addr, ____xxx_________(op));

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAddxEa(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    Ea<M,S> dst = makeOp<M,S>(addr, ____xxx_________(op));

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAndRgEa(StrWriter &str, u32 addr, u16 op)
{
    Dn      src { ____xxx_________(op) };
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAndi(StrWriter &str, u32 addr, u16 op)
{
    Ea<11,S> src = makeOp<11,S>(addr);
    Ea<M, S> dst = makeOp<M, S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmAndiccr(StrWriter &str, u32 addr, u16 op)
{
    Ea<11,S> src = makeOp<11,S>(addr);

    str << Ins<I>{} << tab << src << ", CCR";
}

template<Instr I, Mode M, Size S> void
Moira::dasmAndisr(StrWriter &str, u32 addr, u16 op)
{
    Ea<11,S> src = makeOp<11,S>(addr);

    str << Ins<I>{} << tab << src << ", SR";
}

template<Instr I, Mode M, Size S> void
Moira::dasmBsr(StrWriter &str, u32 addr, u16 op)
{
    if (S == Word) {
        Ea<11,S> value = makeOp<11,S>(addr);
        str << Ins<I>{} << tab << "#" << value;
    } else {
        str << Ins<I>{} << tab << "#" << Disp8{ (i8)op };
    }
}

template<Instr I, Mode M, Size S> void
Moira::dasmChk(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    Dn      dst = Dn { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmClr(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> dst = makeOp<M,Long>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmCmp(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmCmpa(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmCmpi(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmCmpm(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));
    Ea<M,S> dst = makeOp<M,S>(addr, ____xxx_________(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmBcc(StrWriter &str, u32 addr, u16 op)
{
    str << Ins<I>{} << tab << "#" << Disp8{ (i8)op };
}

template<Instr I, Mode M, Size S> void
Moira::dasmBitDxEa(StrWriter &str, u32 addr, u16 op)
{
    Dn         src { ____xxx_________(op) };
    Ea<M,Byte> dst = makeOp<M,Byte>(addr, _____________xxx(op));

    str << Ins<I>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmBitImEa(StrWriter &str, u32 addr, u16 op)
{
    // u8         src = e1 & 0b11111;
    Ea<11,Byte> src = makeOp<11,Byte>(addr);
    Ea<M, Byte> dst = makeOp<M, Byte>(addr, _____________xxx(op));

    src.ext1 &= 0b11111;
    str << Ins<I>{} << tab << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmDbcc(StrWriter &str, u32 addr, u16 op)
{
    Dn src { _____________xxx(op) };

    str << Ins<I>{} << tab << Dn{src} << ", " << irc;
}

template<Instr I, Mode M, Size S> void
Moira::dasmExgDxDy(StrWriter &str, u32 addr, u16 op)
{
    Dn src { ____xxx_________(op) };
    Dn dst { _____________xxx(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmExgAxDy(StrWriter &str, u32 addr, u16 op)
{
    An src { _____________xxx(op) };
    Dn dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmExgAxAy(StrWriter &str, u32 addr, u16 op)
{
    An src { ____xxx_________(op) };
    An dst { _____________xxx(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmExt(StrWriter &str, u32 addr, u16 op)
{
    Dn src { _____________xxx(op) };

    str << Ins<I>{} << Sz<S>{} << tab << Dn{src};
}

template <Instr I, Mode M, Size S> void
Moira::dasmJmp(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmJsr(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmLea(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    An         dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmLink(StrWriter &str, u32 addr, u16 op)
{
    An src { _____________xxx(op) };
    i16 disp = (i16)dasmRead<Word>(addr);

    str << Ins<I>{} << tab << src << ", #" << Disp16{disp};
}

template<Instr I, Mode M1, Mode M2, Size S> void
Moira::dasmMove(StrWriter &str, u32 addr, u16 op)
{
    Ea<M1,S> src = makeOp<M1,S>(addr, _____________xxx(op));
    Ea<M2,S> dst = makeOp<M2,S>(addr, ____xxx_________(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmMovea(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    An         dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmMovemEaRg(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src   = makeOp<M,S>(addr, _____________xxx(op));
    u16     mask  = irc;
    u8      amask = mask >> 8;
    u8      dmask = mask & 0xFF;

    str << Ins<I>{} << tab;

    str << src << ",";

    str << RegList{dmask,'d'};
    if (dmask && amask) str << "/";
    str << RegList{amask,'a'};
}

template<Instr I, Mode M, Size S> void
Moira::dasmMovemRgEa(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst   = makeOp<M,S>(addr, _____________xxx(op));
    u16     mask  = M == 4 ? REVERSE_16(irc) : irc;
    u8      amask = mask >> 8;
    u8      dmask = mask & 0xFF;

    str << Ins<I>{} << tab;

    str << RegList{dmask,'d'};
    if (dmask && amask) str << "/";
    str << RegList{amask,'a'};

    str << "," << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmMovepDxEa(StrWriter &str, u32 addr, u16 op)
{
    assert(S == Word || S == Long);

    Ea<0,S> src   = makeOp<0,S>(addr, ____xxx_________(op));
    Ea<5,S> dst   = makeOp<5,S>(addr, _____________xxx(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmMovepEaDx(StrWriter &str, u32 addr, u16 op)
{
    assert(S == Word || S == Long);

    Ea<5,S> src   = makeOp<5,S>(addr, _____________xxx(op));
    Ea<0,S> dst   = makeOp<0,S>(addr, ____xxx_________(op));

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmMoveq(StrWriter &str, u32 addr, u16 op)
{
    Dn dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << "#" << (u8)op << ", " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmMoveToCcr(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << tab << src << ", CCR";
}

template<Instr I, Mode M, Size S> void
Moira::dasmMoveFromSr(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> dst = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << tab << "SR, " << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmMoveToSr(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> src = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<I>{} << tab << src << ", SR";
}

template<Instr I, Mode M, Size S> void
Moira::dasmMoveUsp(StrWriter &str, u32 addr, u16 op)
{
    An reg { _____________xxx(op) };

    if (M == 0) {
        str << Ins<I>{} << tab << "USP," << reg;
    } else {
        str << Ins<I>{} << tab << reg << ",USP";
    }
}

template<Instr I, Mode M, Size S> void
Moira::dasmMulDiv(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Word> src = makeOp<M,Word>(addr, _____________xxx(op));
    Dn         dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << "," << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmNbcd(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Byte> dst = makeOp<M,Byte>(addr, _____________xxx(op));

    str << Ins<NBCD>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmNop(StrWriter &str, u32 addr, u16 op)
{
    str << Ins<NOP>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmPea(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    An         dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmRtr(StrWriter &str, u32 addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRts(StrWriter &str, u32 addr, u16 op)
{
    str << Ins<I>{};
}

template<Instr I, Mode M, Size S> void
Moira::dasmScc(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Byte> src = makeOp<M,Byte>(addr, _____________xxx(op));

    str << Ins<I>{} << tab << src;
}

template<Instr I, Mode M, Size S> void
Moira::dasmNegNot(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Byte> dst = makeOp<M,Byte>(addr, _____________xxx(op));
    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmSwap(StrWriter &str, u32 addr, u16 op)
{
    Dn reg = Dn { _____________xxx(op) };
    str << Ins<I>{} << tab << reg;
}

template<Instr I, Mode M, Size S> void
Moira::dasmTas(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Byte> dst = makeOp<M,Byte>(addr, _____________xxx(op));
    str << Ins<I>{} << tab << dst;
}

template<Instr I, Mode M, Size S> void
Moira::dasmTrap(StrWriter &str, u32 addr, u16 op)
{
    int nr = ____________xxxx(op);
    str << Ins<I>{} << tab << "#" << (u8)nr;
}

template<Instr I, Mode M, Size S> void
Moira::dasmTrapv(StrWriter &str, u32 addr, u16 op)
{
    str << Ins<I>{};
}

template<Instr I, Mode M, Size S> void
Moira::dasmTst(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,S> ea = makeOp<M,S>(addr, _____________xxx(op));

    str << Ins<TST>{} << tab << ea;
}

template <Instr I, Mode M, Size S> void
Moira::dasmUnlk(StrWriter &str, u32 addr, u16 op)
{
    An reg { _____________xxx(op) };
    str << Ins<I>{} << tab << reg;
}
