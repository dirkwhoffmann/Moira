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
CPU::dasmRegList(u16 regs, char *result)
{
    char *p = result;
    u8 hi = regs >> 8;
    u8 lo = regs & 0xFF;

    dasmRegList(hi, 'D', p);
    if (hi && lo) *p++ = '/';
    dasmRegList(lo, 'A', p);
}

void
CPU::dasmRegList(u8 regs, char sym, char *&p)
{
    int r[8];
    // This functions converts, e.g., 0xED to "D0-D2/D4/D5/D7"

    // Step 1: Convert 0xED to 11101101
    for (int i = 0; i <= 7; i++) r[i] = (regs & (0x80 >> i)) ? 1 : 0;

    // Step 2: Convert 11101101 to 12301201
    for (int i = 1; i <= 7; i++) if (r[i]) r[i] = r[i-1] + 1;

    // Step 3: Convert 12301201 to 33302201
    for (int i = 6; i >= 0; i--) if (r[i] && r[i+1]) r[i] = r[i+1];

    // Step 4: Convert 33302201 to "D0-D2/D4/D5/D7"
    bool first = true;
    for (int i = 0; i <= 7; i++) {
        if (r[i] == 1) {
            if (first) first = false; else *p++ = '/';
            *p++ = sym; *p++ = '0'+i;
        }
        if (r[i] == 2) {
            if (first) first = false; else *p++ = '/';
            *p++ = sym; *p++ = '0'+i; *p++ = '/'; *p++ = sym; *p++ = '0'+i+1;
        }
        if (r[i] > 2) {
            if (first) first = false; else *p++ = '/';
            *p++ = sym; *p++ = '0'+i; *p++ = '-'; *p++ = sym; *p++ = '0'+i+r[i]-1;
        }
        i += r[i];
    }
    *p = 0;
}

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
CPU::dasmJmp(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
CPU::dasmJsr(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
CPU::dasmLea(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    An         dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
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
CPU::dasmMovemEaRg(StrWriter &str, u32 addr, u16 op)
{
    assert(false);
}

template<Instr I, Mode M, Size S> void
CPU::dasmMovemRgEa(StrWriter &str, u32 addr, u16 op)
{
    char s[32];

    dasmRegList(0b11101101, s);
    printf("'%s'\n", s);
    dasmRegList(0b1110110100000000, s);
    printf("'%s'\n", s);
    dasmRegList(0b0001010111111111, s);
    printf("'%s'\n", s);
    dasmRegList(0b1111111101010111, s);
    printf("'%s'\n", s);
    dasmRegList(0b1111000011110000, s);
    printf("'%s'\n", s);
    dasmRegList(0b1101100011011001, s);
    printf("'%s'\n", s);
    dasmRegList(0b1111101111100000, s);
    printf("'%s'\n", s);

    assert(false);
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

template <Instr I, Mode M, Size S> void
CPU::dasmPea(StrWriter &str, u32 addr, u16 op)
{
    Ea<M,Long> src = makeOp<M,Long>(addr, _____________xxx(op));
    An         dst { ____xxx_________(op) };

    str << Ins<I>{} << tab << src << ", " << dst;
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

