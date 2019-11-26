// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void
CPU::execIllegal(u16 opcode)
{
    printf("Moira::execIllegal\n");
}

template<Instr I, Size S> void
CPU::execRegShift(u16 opcode)
{
    // Dx,Dy
    int x = (opcode >> 9) & 0b111; //  ---- xxx- ---- ----
    int y = (opcode >> 0) & 0b111; //  ---- ---- ---- -xxx

    u32 dx = readD<S>(x);
    u32 dy = readD<S>(y);

    writeD<S>(y, shift<S,I>(dx, dy));
}

template<Instr I, Size S> void
CPU::execImmShift(u16 opcode)
{
    // #<cnt>,Dy
    int cnt = (opcode >> 9) & 0b111; //  ---- xxx- ---- ----
    int y   = (opcode >> 0) & 0b111; //  ---- ---- ---- -xxx

    u32 dy = readD<S>(y);

    writeD<S>(y, shift<S,I>(cnt ? cnt : 8, dy));
}

template<Instr I, Mode M> void
CPU::execEaShift(u16 opcode)
{
    //
    // TODO: Read from Ea
    // value = ...

    // TODO: Shift value by 1:
    // shift<Word,I>(1, value);

    // TODO: Write back value
    // write(..., value)
}

template<Mode M> void
CPU::execLea(u16 opcode)
{
    int src = (opcode >> 0) & 0b111; //  ---- ---- ---- -xxx
    int dst = (opcode >> 9) & 0b111; //  ---- xxx- ---- ----

    u32 addr = computeEA<Long, M>(src);

    reg.a[dst] = addr;
    prefetch();
}
