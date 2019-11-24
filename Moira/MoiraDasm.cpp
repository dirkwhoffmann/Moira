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

template <Mode M> void
CPU::dasmLea(StrWriter &str, u16 op, u16 ext1, u16 ext2)
{
    Ea ea { M, op, ext1, ext2 };
    An an { (op >> 9) & 7 };

    str << "lea     " << ea << ", " << an;
}
