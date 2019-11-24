// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void
Moira::execIllegal(uint16_t opcode)
{
    printf("Moira::execIllegal\n");
}

template<Instr I, Size S> void
Moira::execRegShift(uint16_t opcode)
{
    // Dx,Dy
    int x = (opcode >> 9) & 0b111; //  ---- xxx- ---- ----
    int y = (opcode >> 0) & 0b111; //  ---- ---- ---- -xxx

    uint32_t dx = reg.d[x].read<S>();
    uint32_t dy = reg.d[y].read<S>();

    reg.d[y].write<S>(shift<S,I>(dx, dy));
}

template<Instr I, Size S> void
Moira::execImmShift(uint16_t opcode)
{
    // #<cnt>,Dy
    int cnt = (opcode >> 9) & 0b111; //  ---- xxx- ---- ----
    int y   = (opcode >> 0) & 0b111; //  ---- ---- ---- -xxx

    uint32_t dy = reg.d[y].read<S>();

    reg.d[y].write<S>(shift<S,I>(cnt ? cnt : 8, dy));
}

template<Instr I, Mode M> void
Moira::execEaShift(uint16_t opcode)
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
Moira::execLea(uint16_t opcode)
{
    int src = (opcode >> 0) & 0b111; //  ---- ---- ---- -xxx
    int dst = (opcode >> 9) & 0b111; //  ---- xxx- ---- ----

    uint32_t addr = computeEA<Long, M>(src);

    reg.a[dst] = addr;
    prefetch();
}
