// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#define _____________xxx(opcode) ((opcode >> 0) & 0b111)
#define ____xxx_________(opcode) ((opcode >> 9) & 0b111)

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
    int cnt = ____xxx_________(opcode);
    int y   = _____________xxx(opcode);

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

template<Instr I, Mode M, Size S> void
execAdd(u16 opcode)
{
    assert(0);
}

template<Instr I, Mode M, Size S> void
CPU::execAddRg(u16 opcode)
{
    assert(0);
}

template<Instr I, Mode M, Size S> void
CPU::execAddEa(u16 opcode)
{
    assert(0);
}

template<Instr I, Mode M, Size S> void
CPU::execAddImRg(u16 opcode)
{
    int dst = ____xxx_________(opcode);

    u32 result = add<S>(readImm<S>(),readD<S>(dst));

    writeD<S>(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddRgEa(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea  = computeEA<M,S>(dst);

    printf("ea = %x (ea) = %x\n", ea, read<S>(ea));
    u32 result = add<S>(readD<S>(src), read<S>(ea));

    // write<S>(ea, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAdd(u16 opcode)
{
    assert(0);
}

template<Mode M> void
CPU::execLea(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 result = computeEA<M, Long>(src);

    reg.a[dst] = result;
    prefetch();
}
