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

void
CPU::execTrapException(u8 nr)
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

    jumpToVector(nr);
}

template<Instr I, Mode M, Size S> void
CPU::execShiftRg(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    int cnt;

    cnt = readD(src) & 0x3F;
    writeD<S>(dst, shift<I,S>(cnt, readD<S>(dst)));
}

template<Instr I, Mode M, Size S> void
CPU::execShiftIm(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    int cnt;

    cnt = src ? src : 8;
    writeD<S>(dst, shift<I,S>(cnt, readD<S>(dst)));
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
            if (addressError<S>(ea)) return;

            write<S>(ea, shift<I,S>(1, read<S>(ea)));
            break;
        }
    }

    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAbcd(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    switch (M) {

        case 0: // Dn
        {
            u32 result = arith<I,Byte>(readD<Byte>(src), readD<Byte>(dst));
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
CPU::execAddEaRg(u16 opcode)
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
            printf("ea = %x\n", ea); 
            if (addressError<S>(ea)) return;

            result = arith<I,S>(read<S>(ea), readD<S>(dst));
            break;
        }
    }

    writeD<S>(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddRgEa(u16 opcode)
{
    assert(M >= 2 && M <= 8);

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea = computeEA<M,S>(dst);
    if (addressError<S>(ea)) return;

    u32 result = arith<I,S>(readD<S>(src), read<S>(ea));

    write<S>(ea, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAdda(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);
    u32 ea, sop, dop;

    if (!readOperand<M,S>(src, ea, sop)) return;
    sop = SEXT<S>(sop);
    dop = readA(dst);

    u32 result = (I == ADDA) ? dop + sop : dop - sop;

    prefetch();
    writeA(dst, result);
}

template<Instr I, Mode M, Size S> void
CPU::execAddi(u16 opcode)
{
    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;

    u32 result = arith<I,S>(src, data);

    writeOperand<M,S>(dst, ea, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddq(u16 opcode)
{
    assert(I == ADDQ || I == SUBQ);

    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;

    if (src == 0) src = 8;
    u32 result = arith<I,S>(src, data);
    
    writeOperand<M,S>(dst, ea, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddqAn(u16 opcode)
{
    assert(I == ADDQ || I == SUBQ);

    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,Long>(dst, ea, data)) return;

    if (src == 0) src = 8;
    u32 result = (I == ADDQ) ? readA(dst) + src : readA(dst) - src;

    writeA(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAddxRg(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 result = arith<I,S>(readD<S>(src), readD<S>(dst));
    writeD<S>(dst, result);
}

template<Instr I, Mode M, Size S> void
CPU::execAddxEa(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, ea2, data1, data2;
    if (!readOperand<M,S>(src, ea1, data1)) return;
    if (!readOperand<M,S>(dst, ea2, data2)) return;

    u32 result = arith<I,S>(data1, data2);
    write<S>(ea2, result);
}

template<Instr I, Mode M, Size S> void
CPU::execAndEaRg(u16 opcode)
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
            if (addressError<S>(ea)) return;

            result = logic<I,S>(read<S>(ea), readD<S>(dst));
            break;
        }
    }

    writeD<S>(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAndRgEa(u16 opcode)
{
     int src = ____xxx_________(opcode);
     int dst = _____________xxx(opcode);

    switch (M) {

        case 0: // Dn
        {
            u32 result = logic<I,S>(readD<S>(src), readD<S>(dst));
            writeD<S>(dst, result);
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 8);

            u32 ea = computeEA<M,S>(dst);
            if (addressError<S>(ea)) return;

            u32 result = logic<I,S>(readD<S>(src), read<S>(ea));
            write<S>(ea, result);
            break;
        }
    }

    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execAndi(u16 opcode)
{
    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;

    u32 result = logic<I,S>(src, data);

    writeOperand<M,S>(dst, ea, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execBcc(u16 opcode)
{
    if (bcond<I>()) {

        i16 offset = S == Word ? (i16)irc : (i8)opcode;
        u32 newpc = pc + offset;

        readExtensionWord();

        // Take branch
        pc = newpc;

        prefetch();
        return;
     }

     // Fall through to next instruction
     if (S == Word) readExtensionWord();
     prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execBitDxEa(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    switch (M) {

        case 0:
        {
            u8 bit = readD(src) & 0b11111;
            u32 data = readD(dst);
            data = bitop<I>(data, bit);

            if (I != BTST) writeD(dst, data);
            break;
        }
        default:
        {
            u8 bit = readD(src) & 0b111;
            u32 ea = computeEA<M,Byte>(dst);

            u32 data = read<Byte>(ea);
            data = bitop<I>(data, bit);

            if (I != BTST) write<Byte>(ea, data);
        }
    }

    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execBitImEa(u16 opcode)
{
    int dst = _____________xxx(opcode);
    u8  bit = irc & 0b11111;
    readExtensionWord();

    switch (M)
    {
        case 0:
        {
            u32 data = readD(dst);
            data = bitop<I>(data, bit);

            if (I != BTST) writeD(dst, data);
            break;
        }
        default:
        {
            u32 ea = computeEA<M,Byte>(dst);

            u32 data = read<Byte>(ea);
            data = bitop<I>(data, bit);

            if (I != BTST) write<Byte>(ea, data);
        }
    }
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execBsr(u16 opcode)
{
    i16 offset = S == Word ? (i16)irc : (i8)opcode;
    u32 newpc = pc + offset;
    u32 retpc = pc + (S == Word ? 2 : 0);

    // Save the return address
    writeStack(retpc);

    readExtensionWord();

    // Take branch
    pc = newpc;

    prefetch();
    return;
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
            if (addressError<S>(ea)) return;

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

template<Instr I, Mode M, Size S> void
CPU::execCmp(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);
    u32 ea, sop, dop;

    if (!readOperand<M,S>(src, ea, sop)) return;
    prefetch();

    dop = readD<S>(dst);
    cmp<S>(sop, dop);
}

template<Instr I, Mode M, Size S> void
CPU::execCmpa(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);
    u32 ea, sop, dop;

    if (!readOperand<M,S>(src, ea, sop)) return;
    prefetch();

    sop = SEXT<S>(sop);
    dop = readA(dst);
    cmp<Long>(sop, dop);
}

template<Instr I, Mode M, Size S> void
CPU::execCmpi(u16 opcode)
{
    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;
    prefetch();

    cmp<S>(src, data);
}

template<Instr I, Mode M, Size S> void
CPU::execDbcc(u16 opcode)
{
    int dn = _____________xxx(opcode);

    /* IF (condition false)
     *   THEN [Dn] := [Dn] - 1 {decrement loop counter}
     *     IF [Dn] == -1 THEN [PC] ← [PC] + 2 {fall through to next instruction}
     *                   ELSE [PC] ← [PC] + d {take branch}
     * ELSE [PC] := [PC] + 2 {fall through to next instruction}
     */

    if (!bcond<I>()) {

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

template<Instr I, Mode M, Size S> void
CPU::execExt(u16 opcode)
{
    int n     = _____________xxx(opcode);
    u32 dn    = readD(n);
    u32 mask  = (S == Word) ? MASK<Byte>() << 8 : MASK<Word>() << 16;
    bool neg  = (S == Word) ? NBIT<Byte>(dn) : NBIT<Word>(dn);

    if (neg) { dn |= mask; } else {dn &= ~mask; }

    writeD(n, dn);
    sr.c = 0;
    sr.v = 0;
    sr.n = NBIT<S>(dn);
    sr.z = ZERO<S>(dn);

    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execJmp(u16 opcode)
{
    int src = _____________xxx(opcode);

    u32 ea = computeEA<M,Long>(src);

    pc = ea;
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execJsr(u16 opcode)
{
    int src = _____________xxx(opcode);

    u32 ea = computeEA<M,Long>(src);

    writeStack(pc);

    pc = ea;
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execLea(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea = computeEA<M,Long>(src);

    reg.a[dst] = ea;
    prefetch();
}

template<Instr I, Mode M1, Mode M2, Size S> void
CPU::execMove(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, data1, ea2, data2;

    if (!readOperand<M1,S>(src, ea1, data1)) return;
    if (!readOperand<M2,S>(dst, ea2, data2)) return;

    writeOperand<M2,S>(dst, ea2, data1);

    sr.c = 0;
    sr.v = 0;
    sr.n = NBIT<S>(data1);
    sr.z = ZERO<S>(data1);

    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execMovea(u16 opcode)
{
    i32 result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    switch(M) {

       case 0: // Dn
        {
            result = SIGN<S>(readD<S>(src));
            break;
        }
        case 1: // An
        {
            result =  SIGN<S>(readA<S>(src));
            break;
        }
        case 11: // Imm
        {
            result = SIGN<S>(readImm<S>());
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 11);

            u32 ea = computeEA<M,S>(src);
            if (addressError<S>(ea)) return;

            result = SIGN<S>(read<S>(ea));
            break;
        }
    }

    writeA(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execMovemEaRg(u16 opcode)
{
    int src  = _____________xxx(opcode);
    u16 mask = irc;

    switch (M) {

        case 3: // (An)+
        {
            u32 ea = readA(src);
            if (addressError<S>(ea)) return; // TODO: Trigger exception

            readExtensionWord();

            for(int i = 0; i <= 15; i++) {

                if (mask & (1 << i)) {
                    writeR(i, SEXT<S>(read<S>(ea)));
                    ea += S;
                }
            }
            writeA(src, ea);
            break;
        }
        default:
        {
            u32 ea = computeEA<M,S>(src);
            if (addressError<S>(ea)) return; // TODO: Trigger exception

            readExtensionWord();

            for(int i = 0; i <= 15; i++) {

                if (mask & (1 << i)) {
                    writeR(i, SEXT<S>(read<S>(ea)));
                    ea += S;
                }
            }
            break;
        }
    }
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execMovemRgEa(u16 opcode)
{
    int dst  = _____________xxx(opcode);
    u16 mask = irc;

    switch (M) {

        case 3: // (An)+
        {
            assert(false);
            break;
        }
        case 4: // -(An)
        {
            u32 ea = readA(dst);
            if (addressError<S>(ea)) return; // TODO: Trigger exception

            readExtensionWord();

            for(int i = 15; i >= 0; i--) {

                if (mask & (0x8000 >> i)) {
                    ea -= S;
                    write<S>(ea, reg.r[i]);
                }
            }
            writeA(dst, ea);
            break;
        }
        default:
        {
            u32 ea = computeEA<M,S>(dst);
            if (addressError<S>(ea)) return; // TODO: Trigger exception

            readExtensionWord();

            for(int i = 0; i < 16; i++) {

                if (mask & (1 << i)) {
                    write<S>(ea, reg.r[i]);
                    ea += S;
                }
            }
            break;
        }
    }
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execMoveq(u16 opcode)
{
    i8  src = (i8)(opcode & 0xFF);
    int dst = ____xxx_________(opcode);

    writeD<Long>(dst, (i32)src);

    sr.c = 0;
    sr.v = 0;
    sr.n = NBIT<Byte>(src);
    sr.z = ZERO<Byte>(src);

    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execMoveUsp(u16 opcode)
{
    int an = _____________xxx(opcode);

    // Check for supervisior mode
    if (!sr.s) privilegeException();

    prefetch();

    if (M == 0) {
        writeA(an, reg.usp);
    } else {
        reg.usp = readA(an);
    }
}

template<Instr I, Mode M, Size S> void
CPU::execMulDiv(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);
    u16 dn  = readD<Word>(dst);
    u32 ea, data;

    if (!readOperand<M, Word>(src, ea, data)) return;

    printf("data = %x\n", data);

    switch (I) {

        case MULS: // Signed multiplication
        {
            prefetch();

            u32 result = (i16)data * (i16)dn;
            writeD(dst, result);

            sr.c = 0;
            sr.v = 0;
            sr.n = NBIT<Long>(result);
            sr.z = ZERO<Long>(result);
            break;
        }

        case MULU: // Unsigned multiplication
        {
            prefetch();

            u32 result = data * dn;
            writeD(dst, result);

            sr.c = 0;
            sr.v = 0;
            sr.n = NBIT<Long>(result);
            sr.z = ZERO<Long>(result);
            break;
        }
        case DIVS: // Signed division
        {
            sr.n = sr.z = sr.v = sr.c = 0;

            if (data == 0) {
                 return execTrapException(5);
            }
            
            u32 dividend = readD(dst);
            prefetch();

            if (dividend == 0x80000000 && (i16)data == -1) {
                sr.v = sr.n = 1;
                return;
            }
            i32 result = (i32)dividend / (i16)data;
            i16 remainder = (i32)dividend % (i16)data;

            if ((result & 0xffff8000) != 0 && (result & 0xffff8000) != 0xffff8000) {
                sr.v = 1;
                sr.n = 1;
                break;
            }
            sr.n = NBIT<Word>(result);
            sr.z = ZERO<Word>(result);
            
            writeD(dst, (result & 0xffff) | (remainder << 16));
            break;
        }
        case DIVU: // Unsigned division
        {
            sr.n = sr.z = sr.v = sr.c = 0;

            if (data == 0) {
                return execTrapException(5);
            }

            u32 dividend = readD(dst);
            prefetch();

            u32 result = dividend / data;
            u16 remainder = dividend % data;

            if (result > 0xffff) {
                sr.v = 1;
                sr.n = 1;
                break;
            }
            sr.n = NBIT<Word>(result);
            sr.z = ZERO<Word>(result);

            writeD(dst, (result & 0xffff) | (remainder << 16));
            break;
        }
    }
}

template<Instr I, Mode M, Size S> void
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

template<Instr I, Mode M, Size S> void
CPU::execNegNot(u16 opcode)
{
    int dst = { _____________xxx(opcode) };
    u32 ea, data;

    if (!readOperand<M,S>(dst, ea, data)) return;

    switch (I) {
        case NEG:  data = arith<SUB,S>(data, 0); break;
        case NEGX: data = arith<SUBX,S>(data, 0); break;
        case NOT:  data = logic<NOT,S>(data); break;
        default:   assert(false); break;
    }

    prefetch();
    // write<S>(ea, data);
    writeOperand<M,S>(dst, ea, data); 
}

template<Instr I, Mode M, Size S> void
CPU::execNop(u16 opcode)
{

}

template<Instr I, Mode M, Size S> void
CPU::execPea(u16 opcode)
{
    int src = _____________xxx(opcode);

    u32 ea = computeEA<M,Long>(src);

    writeStack(ea);
    prefetch();
}

template<Instr I, Mode M, Size S> void
CPU::execScc(u16 opcode)
{
    int dst = { _____________xxx(opcode) };
    u32 ea, data;

    if (!readOperand<M,Byte>(dst, ea, data)) return;

    prefetch();
    writeOperand<M,Byte>(dst, ea, bcond<I>() ? 0xFF : 0);
}

template<Instr I, Mode M, Size S> void
CPU::execTas(u16 opcode)
{
    int dst = { _____________xxx(opcode) };

    switch (M) {

        case 0:
        {
            u8 data = readD<Byte>(dst);

            sr.c = 0;
            sr.v = 0;
            sr.n = NBIT<Byte>(data);
            sr.z = ZERO<Byte>(data);

            data |= 0x80;
            writeD<Byte>(dst, data);
            break;
        }
        default:
        {
            u32 ea = computeEA<M,Byte>(dst);
            u8 data = read<Byte>(ea);

            sr.c = 0;
            sr.v = 0;
            sr.n = NBIT<Byte>(data);
            sr.z = ZERO<Byte>(data);

            data |= 0x80;
            write<Byte>(ea, data);
            break;
        }
    }
    prefetch();
}

template<Instr I, Mode M, Size S> void
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
            if (addressError<S>(ea)) return;

            value = read<S>(ea);
            break;
        }
    }

    sr.c = 0;
    sr.v = 0;
    sr.n = NBIT<S>(value);
    sr.z = ZERO<S>(value);

    prefetch();
}
