// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void
CPU::execLineA(u16 opcode)
{
    printf("Moira::execLineA\n");
}

void
CPU::execLineF(u16 opcode)
{
    printf("Moira::execLineF\n");
}

void
CPU::execIllegal(u16 opcode)
{
    printf("Moira::execIllegal\n");


}

void execException(u8 nr)
{
    // Enter supervisor mode and update the status register
     setSupervisorMode(true);
     sr.t = 0;

     // Push PC and SR
     reg.sp -= 2;
     write<Word>(reg.sp, pc & 0xFFFF);
     reg.sp -= 2;
     write<Word>(reg.sp, pc >> 16);
     reg.sp -= 2;
     write<Word>(reg.sp, getSR());

     // Update the prefetch queue
     readExtensionWord();
     prefetch();

     jumpToVector(nr);
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
CPU::execAddXXRg(u16 opcode)
{
    u32 result;

    int dst = ____xxx_________(opcode);

    switch (M) {

        case 0: // Dn
        {
            int src = _____________xxx(opcode);
            result = add<S>(readD<S>(src), readD<S>(dst));
            break;
        }
        case 1: // An
        {
            int src = _____________xxx(opcode);
            result = add<S>(readA<S>(src), readD<S>(dst));
            break;
        }
        case 11: // Imm
        {
            result = add<S>(readImm<S>(), readD<S>(dst));
            break;
        }
        default: // Ea
        {
            assert(M >= 3 && M <= 10);

            int src = _____________xxx(opcode);
            u32 ea  = computeEA<M,S>(src);
            result = add<S>(read<S>(ea), readD<S>(dst));
            break;
        }
    }

    writeD<S>(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddRgXX(u16 opcode)
{
    assert(M >= 3 && M <= 8);

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    u32 ea  = computeEA<M,S>(dst);

    u32 result = add<S>(readD<S>(src), read<S>(ea));

    write<S>(ea, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddRgRg(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 result = add<S>(readD<S>(src), readD<S>(dst));

    writeD<S>(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddEaRg(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);
    u32 ea  = computeEA<M,S>(src);

    printf("execAddRgEa:: src %x dst %x ea %x\n", src, dst, ea);
    printf("read<S>(%x) = %x\n", ea, read<S>(ea));
    u32 result = add<S>(readD<S>(ea), read<S>(dst));

    write<S>(ea, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddImRg(u16 opcode)
{
    int dst = ____xxx_________(opcode);

    u32 result = add<S>(readImm<S>(), readD<S>(dst));

    writeD<S>(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddRgEa(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    u32 ea  = computeEA<M,S>(dst);

    printf("execAddRgEa:: src %x dst %x ea %x\n", src, dst, ea);
    printf("read<S>(%x) = %x\n", ea, read<S>(ea));
    u32 result = add<S>(readD<S>(src), read<S>(ea));

    write<S>(ea, result);
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
