// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------



//
// Disassembling adressing modes
//


//
// Disassembling instructions
//

void
CPU::dasmIllegal(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    str << "ILLEGAL";
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
    Ea<M> src { _____________xxx(op), e1, e2 };
    Dn    dst { ____xxx_________(op) };

    str << "add" << Sz<S>{} << " " << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddRgXX(StrWriter &str, u16 op, u16 e1, u16 e2)
{
    str << "dasmAddRg__";
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddRgRg(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    Dn    src { _____________xxx(op) };
    Dn    dst { ____xxx_________(op) };

    str << "add " << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddEaRg(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    Ea<M> src { _____________xxx(op), ext1, ext2 };
    Dn    dst { ____xxx_________(op) };

    str << "add" << Sz<S>{} << "  " << src << ", " << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddImRg(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    Im<S> src { ext1, ext2 };
    Dn    dst { ____xxx_________(op) };

    str << "add " << src << "," << dst;
}

template<Instr I, Mode M, Size S> void
CPU::dasmAddRgEa(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    Dn    src { ____xxx_________(op) };
    Ea<M> dst { _____________xxx(op), ext1, ext2 };

    str << "add" << Sz<S>{} << "  " << src << ", " << dst;
}

template <Mode M> void
CPU::dasmLea(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    Ea<M> src { _____________xxx(op), ext1, ext2 };
    An    dst { ____xxx_________(op) };

    str << "lea     " << src << ", " << dst;
}
