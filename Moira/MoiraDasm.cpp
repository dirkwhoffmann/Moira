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
Moira::dasmIllegal(StrWriter &str, uint16_t op, uint16_t ext1, uint16_t ext2)
{
    str << "ILLEGAL";
}

template<Instr I, Size S> void 
Moira::dasmRegShift(StrWriter &str, uint16_t op, uint16_t ext1, uint16_t ext2)
{
    str
    << (const char *)"REG_SHIFT "
    << "size = " << (uint8_t)42
    << "mode = " << (uint16_t)13;
}

template <Instr I, Size S> void
Moira::dasmImmShift(StrWriter &str, uint16_t op, uint16_t ext1, uint16_t ext2)
{
    str
    << (const char *)"IMM_SHIFT "
    << "size = " << (uint8_t)42
    << "mode = " << (uint16_t)13;
}

template <Instr I, Mode M> void
Moira::dasmEaShift(StrWriter &str, uint16_t op, uint16_t ext1, uint16_t ext2)
{
    str
    << (const char *)"EA_SHIFT "
    << "size = " << (uint8_t)42
    << "mode = " << (uint16_t)13;
}

template <Mode M> void
Moira::dasmLea(StrWriter &str, uint16_t op, uint16_t ext1, uint16_t ext2)
{
    Ea ea { M, op, ext1, ext2 };
    An an { (op >> 9) & 7 };

    str << "lea     " << ea << ", " << an;
}
