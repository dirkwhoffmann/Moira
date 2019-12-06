// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void
CPU::execAddressError(u32 addr)
{
    assert(addr & 1);
    printf("Moira::execAddressViolation\n");
    execGroup0Exception(addr, 3);
}

void
CPU::execLineA(u16 opcode)
{
    printf("Moira::execLineA\n");
    execGroup1Exception(10);
}

void
CPU::execLineF(u16 opcode)
{
    printf("Moira::execLineF\n");
    execGroup1Exception(11);
}

void
CPU::execIllegal(u16 opcode)
{
    // printf("Moira::execIllegal\n");
    execGroup1Exception(4);
}

void
CPU::execGroup0Exception(u32 addr, u8 nr)
{
    /* Group 0 exceptions indicate a serious error condition. Detailed
     * information about the current CPU state is pushed on the stack to
     * support diagnosis.
     */

    // Error description (TODO: THIS IS INCOMPLETE)
    u16 error = 0x11 | (sr.s ? 4 : 0);

    // Enter supervisor mode and update the status register
    setSupervisorMode(true);
    sr.t = 0;

    printf("Moira group0: %x %x %x %x\n, ", pc, addr, getSR(), error);

    // Push PC
    reg.sp -= 2; write<Word>(reg.sp, pc & 0xFFFF);
    reg.sp -= 2; write<Word>(reg.sp, pc >> 16);

    // Push SR and IRD
    reg.sp -= 2; write<Word>(reg.sp, getSR());
    reg.sp -= 2; write<Word>(reg.sp, ird);

    // Push address
    reg.sp -= 2; write<Word>(reg.sp, addr & 0xFFFF);
    reg.sp -= 2; write<Word>(reg.sp, addr >> 16);

    // Push memory access type and function code
    reg.sp -= 2; write<Word>(reg.sp, error);

    // Update the prefetch queue
     readExtensionWord();
     prefetch();

     jumpToVector(nr);
}

void
CPU::execGroup1Exception(u8 nr)
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

template<Instr I, Mode M, Size S> void
CPU::execShift(u16 op)
{
    int src = ____xxx_________(op);
    int dst = _____________xxx(op);
    int cnt;

    switch (M) {

        case 0: // Dn
        {
            cnt = readD(src) & 0x3F;
            writeD<S>(dst, shift<I,S>(cnt, readD<S>(dst)));
            break;
        }
        case 11: // Imm
        {
            cnt = src ? src : 8;
            writeD<S>(dst, shift<I,S>(cnt, readD<S>(dst)));
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 8);
            u32 ea = computeEA<M,S>(src);
            if (addressError(ea)) return;

            write<S>(ea, shift<I,S>(1, read<S>(ea)));
            break;
        }
    }

    prefetch();
}

template<Instr I, Mode M> void
CPU::execAbcd(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    printf("src = %x dst = %x\n", src, dst);
    switch (M) {

        case 0: // Dn
        {
            u32 result = arith<I,Byte>(readD<Byte>(src), readD<Byte>(dst));
            printf("(src) = %x (dst) = %x\n", readD<Byte>(src), readD<Byte>(dst));
            writeD<Byte>(dst, result);
            break;
        }
        default: // Ea
        {
            assert(M == 4);

            u32 ea1 = computeEA<M,Byte>(src);
            u32 ea2 = computeEA<M,Byte>(dst);

            u32 result = arith<I,Byte>(read<Byte>(ea1), read<Byte>(ea2));
            write<Byte>(ea2, result);
            break;
        }
    }

    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddXXRg(u16 opcode)
{
    u32 result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    switch (M) {

        case 0: // Dn
        {
            result = arith<I,S>(readD<S>(src), readD<S>(dst));
            break;
        }
        case 1: // An
        {
            result = arith<I,S>(readA<S>(src), readD<S>(dst));
            break;
        }
        case 11: // Imm
        {
            result = arith<I,S>(readImm<S>(), readD<S>(dst));
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 10);

            u32 ea = computeEA<M,S>(src);
            if (addressError(ea)) return;

            result = arith<I,S>(read<S>(ea), readD<S>(dst));
            break;
        }
    }

    writeD<S>(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddRgXX(u16 opcode)
{
    assert(M >= 2 && M <= 8);

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea  = computeEA<M,S>(dst);
    if (ea & 1) { execAddressError(ea); return; }

    u32 result = arith<I,S>(readD<S>(src), read<S>(ea));

    write<S>(ea, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAndXXRg(u16 opcode)
{
    u32 result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    switch (M) {

        case 0: // Dn
        {
            result = logic<I,S>(readD<S>(src), readD<S>(dst));
            break;
        }
        case 11: // Imm
        {
            result = logic<I,S>(readImm<S>(), readD<S>(dst));
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 10);

            u32 ea = computeEA<M,S>(src);
            if (addressError(ea)) return;

            result = logic<I,S>(read<S>(ea), readD<S>(dst));
            break;
        }
    }

    writeD<S>(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAndRgXX(u16 opcode)
{
    assert(M >= 2 && M <= 8);

     int src = ____xxx_________(opcode);
     int dst = _____________xxx(opcode);

     u32 ea  = computeEA<M,S>(dst);
     if (ea & 1) { execAddressError(ea); return; }

     u32 result = logic<I,S>(readD<S>(src), read<S>(ea));

     write<S>(ea, result);
     prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execClr(u16 opcode)
{
    int dst = _____________xxx(opcode);

    switch (M) {

        case 0: // Dn
        {
            writeD<S>(dst, 0);
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 8);

            u32 ea = computeEA<M,S>(dst);
            if (addressError(ea)) return;

            (void)read<S>(ea);
            write<S>(ea, 0);
            break;
        }
    }

    sr.n = 0;
    sr.z = 1;
    sr.v = 0;
    sr.c = 0;

    prefetch();
}

template<Cond C> void
CPU::execDbcc(u16 opcode)
{
    int dn = _____________xxx(opcode);

    /* IF (condition false)
     *   THEN [Dn] := [Dn] - 1 {decrement loop counter}
     *     IF [Dn] == -1 THEN [PC] ← [PC] + 2 {fall through to next instruction}
     *                   ELSE [PC] ← [PC] + d {take branch}
     * ELSE [PC] := [PC] + 2 {fall through to next instruction}
     */

    if (!check<C>()) {

        u32 newpc = pc + (i16)irc;

        // Decrement loop counter
        writeD<Word>(dn, readD<Word>(dn) - 1);

        readExtensionWord();

        // Take branch if Dn does not equal -1
        if ((i16)readD<Word>(dn) != -1) {  pc = newpc; }

        prefetch();
        return;
    }

    // Fall through to next instruction
    readExtensionWord();
    prefetch();
}

template<Size S> void
CPU::execExt(u16 opcode)
{
    int n     = _____________xxx(opcode);
    u32 dn    = readD(n);
    u32 mask  = (S == Word) ? MASK<Byte>() << 8 : MASK<Word>() << 16;
    bool msb  = (S == Word) ? MSBIT<Byte>(dn) : MSBIT<Word>(dn);

    if (msb) { dn |= mask; } else {dn &= ~mask; }

    writeD(n, dn);
    sr.c = 0;
    sr.v = 0;
    sr.n = MSBIT<S>(dn);
    sr.z = ZERO<S>(dn);

    prefetch();
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

template<Mode M> void
CPU::execNbcd(u16 opcode)
{
    int reg = _____________xxx(opcode);

    switch (M) {

        case 0: // Dn
        {
            writeD<Byte>(reg, arith<SBCD,Byte>(readD<Byte>(reg), 0));
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 8);

            u32 ea = computeEA<M,Byte>(reg);
            write<Byte>(ea, arith<SBCD,Byte>(read<Byte>(ea), 0));
            break;
        }
    }

    prefetch();
}

void
CPU::execNop(u16 opcode)
{

}

template<Mode M, Size S> void
CPU::execTst(u16 opcode)
{
    int reg = _____________xxx(opcode);
    u32 value;

    switch (M) {

        case 0: // Dn
        {
            value = readD<S>(reg);
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 10);

            u32 ea = computeEA<M,S>(reg);
            if (addressError(ea)) return;

            value = read<S>(ea);
            break;
        }
    }

    sr.c = 0;
    sr.v = 0;
    sr.n = MSBIT<S>(value);
    sr.z = ZERO<S>(value);

    prefetch();
}
