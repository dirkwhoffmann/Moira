// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include <utility>

void
Moira::saveToStackDetailed(u16 sr, u32 addr, u16 code)
{
    // Push PC
     reg.sp -= 2; writeM<Word>(reg.sp, reg.pc & 0xFFFF);
     reg.sp -= 2; writeM<Word>(reg.sp, reg.pc >> 16);

     // Push SR and IRD
     reg.sp -= 2; writeM<Word>(reg.sp, sr);
     reg.sp -= 2; writeM<Word>(reg.sp, ird);

     // Push address
     reg.sp -= 2; writeM<Word>(reg.sp, addr & 0xFFFF);
     reg.sp -= 2; writeM<Word>(reg.sp, addr >> 16);

     // Push memory access type and function code
     reg.sp -= 2; writeM<Word>(reg.sp, code);
}

void
Moira::saveToStackBrief(u16 sr)
{
    // Push PC and SR
     reg.sp -= 2;
     writeM<Word>(reg.sp, reg.pc & 0xFFFF);
     reg.sp -= 2;
     writeM<Word>(reg.sp, reg.pc >> 16);
     reg.sp -= 2;
     writeM<Word>(reg.sp, sr);
}

void
Moira::execAddressError(u32 addr)
{
    assert(addr & 1);

    /* Group 0 exceptions indicate a serious error condition. Detailed
     * information about the current CPU state is pushed on the stack to
     * support diagnosis.
     */

    // Memory access type and function code (TODO: THIS IS INCOMPLETE)
    u16 code = 0x11 | (sr.s ? 4 : 0);

    // Enter supervisor mode and update the status register
    setSupervisorMode(true);
    sr.t = 0;

    // addr -= 2;
    saveToStackDetailed(getSR(), addr, code);

    jumpToVector(3);
}

void
Moira::execLineA(u16 opcode)
{
    execGroup1Exception(10);
}

void
Moira::execLineF(u16 opcode)
{
    execGroup1Exception(11);
}

void
Moira::execIllegal(u16 opcode)
{
    u16 status = getSR();

    // Enter supervisor mode and update the status register
    setSupervisorMode(true);
    sr.t = 0;

    // Put information on stack
    saveToStackBrief(status);

    jumpToVector(4);
}

void
Moira::execGroup0Exception(u32 addr, u8 nr)
{
    /* Group 0 exceptions indicate a serious error condition. Detailed
     * information about the current CPU state is pushed on the stack to
     * support diagnosis.
     */

    // Memory access type and function code (TODO: THIS IS INCOMPLETE)
    u16 code = 0x11 | (sr.s ? 4 : 0);

    // Enter supervisor mode and update the status register
    setSupervisorMode(true);
    sr.t = 0;

    saveToStackDetailed(getSR(), addr, code);

    /*
     // Push PC
     reg.sp -= 2; write<Word>(reg.sp, reg.pc & 0xFFFF);
     reg.sp -= 2; write<Word>(reg.sp, reg.pc >> 16);

     // Push SR and IRD
     reg.sp -= 2; write<Word>(reg.sp, getSR());
     reg.sp -= 2; write<Word>(reg.sp, ird);

     // Push address
     reg.sp -= 2; write<Word>(reg.sp, addr & 0xFFFF);
     reg.sp -= 2; write<Word>(reg.sp, addr >> 16);

     // Push memory access type and function code
     reg.sp -= 2; write<Word>(reg.sp, error);
     */

    // Update the prefetch queue
    readExtensionWord();
    prefetch();

    jumpToVector(nr);
}

void
Moira::execGroup1Exception(u8 nr)
{
    u16 status = getSR();

    // Enter supervisor mode and update the status register
    setSupervisorMode(true);
    sr.t = 0;

    saveToStackBrief(status);

    /*
     // Push PC and SR
     reg.sp -= 2;
     write<Word>(reg.sp, reg.pc & 0xFFFF);
     reg.sp -= 2;
     write<Word>(reg.sp, reg.pc >> 16);
     reg.sp -= 2;
     write<Word>(reg.sp, status);
     */

    // Update the prefetch queue
    readExtensionWord();
    prefetch();

    jumpToVector(nr);
}

void
Moira::execTrapException(u8 nr)
{
    u16 status = getSR();

    // Enter supervisor mode and update the status register
    setSupervisorMode(true);
    sr.t = 0;

    // Push PC and SR
    saveToStackBrief(status);

    jumpToVector(nr);
}

void
Moira::privilegeException()
{
    u16 status = getSR();

    // Enter supervisor mode and update the status register
    setSupervisorMode(true);
    sr.t = 0;

    reg.pc -= 2;
    saveToStackBrief(status);

    // Update the prefetch queue
    // readExtensionWord();
    // prefetch();

    jumpToVector(8);
}

template<Instr I, Mode M, Size S> void
Moira::execShiftRg(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    int cnt;

    cnt = readD(src) & 0x3F;
    writeD<S>(dst, shift<I,S>(cnt, readD<S>(dst)));
}

template<Instr I, Mode M, Size S> void
Moira::execShiftIm(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    int cnt;

    cnt = src ? src : 8;
    writeD<S>(dst, shift<I,S>(cnt, readD<S>(dst)));
}

template<Instr I, Mode M, Size S> void
Moira::execShiftEa(u16 op)
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
            prefetch();

            cnt = src ? src : 8;
            writeD<S>(dst, shift<I,S>(cnt, readD<S>(dst)));
            break;
        }
        default: // Ea
        {
            u32 ea, data;
            if (!readOperand<M,S>(src, ea, data)) return;
            prefetch();
            writeM<S>(ea, shift<I,S>(1, data));
            break;
        }
    }
}

template<Instr I, Mode M, Size S> void
Moira::execAbcd(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    switch (M) {

        case 0: // Dn
        {
            u32 result = arith<I,Byte>(readD<Byte>(src), readD<Byte>(dst));
            prefetch();
            writeD<Byte>(dst, result);
            break;
        }
        default: // Ea
        {
            assert(M == 4);

            u32 ea1, ea2, data1, data2;
            if (!readOperand<M,S>(src, ea1, data1)) return;
            if (!readOperand<M,S>(dst, ea2, data2)) return;

            u32 result = arith<I,Byte>(data1, data2);

            prefetch();

            writeM<Byte>(ea2, result);
            break;
        }
    }
}

template<Instr I, Mode M, Size S> void
Moira::execAddEaRg(u16 opcode)
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

            u32 ea, data;
            if (!readOperand<M,S>(src, ea, data)) return;

            result = arith<I,S>(data, readD<S>(dst));
            break;
        }
    }

    prefetch();
    writeD<S>(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddRgEa(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    printf("execAddRgEa(M = %d S = %d)\n", M, S);

    printf("clock = %lld\n", clock);
    
    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;

    printf("clock = %lld execAddRgEa(2)\n", clock);

    u32 result = arith<I,S>(readD<S>(src), data);

    printf("clock = %lld\n", clock);
    prefetch();
    // writeMDeprecated<S>(ea, result);
    writeM<S>(ea, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAdda(u16 opcode)
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
Moira::execAddi(u16 opcode)
{
    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;

    u32 result = arith<I,S>(src, data);
    prefetch();

    writeOperand<M,S>(dst, ea, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddq(u16 opcode)
{
    assert(I == ADDQ || I == SUBQ);

    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;

    if (src == 0) src = 8;
    u32 result = arith<I,S>(src, data);

    prefetch();
    writeOperand<M,S>(dst, ea, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddqAn(u16 opcode)
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
Moira::execAddxRg(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 result = arith<I,S>(readD<S>(src), readD<S>(dst));
    writeD<S>(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddxEa(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, ea2, data1, data2;
    if (!readOperand<M,S>(src, ea1, data1)) return;
    if (!readOperand<M,S>(dst, ea2, data2)) return;

    u32 result = arith<I,S>(data1, data2);

    if (S == Long) {
        writeM<Word>(ea2 + 2, result & 0xFFFF);
        prefetch();
        writeM<Word>(ea2, result >> 16);
    } else {
        prefetch();
        writeM<S>(ea2, result);
    }
}

template<Instr I, Mode M, Size S> void
Moira::execAndEaRg(u16 opcode)
{
    u32 result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    switch (M) {

        case 0: // Dn
        {
            result = logic<I,S>(readD<S>(src), readD<S>(dst));
            prefetch();
            break;
        }
        case 11: // Imm
        {
            result = logic<I,S>(readImm<S>(), readD<S>(dst));
            prefetch();
            break;
        }
        default: // Ea
        {
            u32 ea, data;

            if (!readOperand<M,S>(src, ea, data)) return;
            prefetch();

            result = logic<I,S>(data, readD<S>(dst));
            break;
        }
    }

    writeD<S>(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAndRgEa(u16 opcode)
{
     int src = ____xxx_________(opcode);
     int dst = _____________xxx(opcode);

    switch (M) {

        case 0: // Dn
        {
            u32 result = logic<I,S>(readD<S>(src), readD<S>(dst));
            prefetch();
            writeD<S>(dst, result);
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 8);

            u32 ea, data;
            if (!readOperand<M,S>(dst, ea, data)) return;

            u32 result = logic<I,S>(readD<S>(src), data);
            prefetch();
            writeM<S>(ea, result);
            break;
        }
    }
}

template<Instr I, Mode M, Size S> void
Moira::execAndi(u16 opcode)
{
    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;

    u32 result = logic<I,S>(src, data);

    prefetch();
    writeOperand<M,S>(dst, ea, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAndiccr(u16 opcode)
{
    u32 src = readImm<S>();
    u8  dst = getCCR();

    u32 result = logic<I,S>(src, dst);
    setCCR(result);

    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execAndisr(u16 opcode)
{
    // This instruction requires supervisor mode
    if (!sr.s) { privilegeException(); return; }

    u32 src = readImm<S>();
    u16 dst = getSR();

    u32 result = logic<I,S>(src, dst);

    setSR(result);

    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execBcc(u16 opcode)
{
    if (bcond<I>()) {

        i16 offset = S == Word ? (i16)irc : (i8)opcode;
        u32 newpc = reg.pc + offset;

        // Take branch
        reg.pc = newpc;
        fullPrefetch();

    } else {

        // Fall through to next instruction
        if (S == Word) readExtensionWord();
        prefetch();
    }
}

template<Instr I, Mode M, Size S> void
Moira::execBitDxEa(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    switch (M) {

        case 0:
        {
            u8 bit = readD(src) & 0b11111;
            u32 data = readD(dst);
            data = bitop<I>(data, bit);

            prefetch();

            if (I != BTST) writeD(dst, data);
            break;
        }
        default:
        {
            u8 bit = readD(src) & 0b111;

            u32 ea, data;
            if (!readOperand<M,Byte>(dst, ea, data)) return;

            data = bitop<I>(data, bit);
            prefetch();

            if (I != BTST) writeM<Byte>(ea, data);
        }
    }
}

template<Instr I, Mode M, Size S> void
Moira::execBitImEa(u16 opcode)
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

            prefetch();

            if (I != BTST) writeD(dst, data);
            break;
        }
        default:
        {
            u32 ea, data;
            if (!readOperand<M,Byte>(dst, ea, data)) return;

            data = bitop<I>(data, bit);
            prefetch();

            if (I != BTST) writeM<Byte>(ea, data);
        }
    }
}

template<Instr I, Mode M, Size S> void
Moira::execBsr(u16 opcode)
{
    i16 offset = S == Word ? (i16)irc : (i8)opcode;
    u32 newpc = reg.pc + offset;
    u32 retpc = reg.pc + (S == Word ? 2 : 0);

    // Save the return address
    push(retpc);

    // Take branch
    reg.pc = newpc;

    fullPrefetch();
    return;
}

template<Instr I, Mode M, Size S> void
Moira::execChk(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);
    u32 ea, sop, dop;

    if (!readOperand<M,S>(src, ea, sop)) return;
    dop = readD<S>(dst);

    prefetch();

    sr.z = ZERO<S>(dop);
    sr.v = 0;
    sr.c = 0;
    sr.n = 0;

    if ((i16)dop > (i16)sop) {
        execTrapException(6);
    }
    else if ((i16)dop < 0) {
        sr.n = 1;
        execTrapException(6);
    }
}

template<Instr I, Mode M, Size S> void
Moira::execClr(u16 opcode)
{
    int dst = _____________xxx(opcode);

    switch (M) {

        case 0: // Dn
        {
            prefetch();
            writeD<S>(dst, 0);
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 8);

            u32 ea, data;
            if (!readOperand<M,S>(dst, ea, data)) return;

            prefetch();
            writeM<S>(ea, 0);
            break;
        }
    }

    sr.n = 0;
    sr.z = 1;
    sr.v = 0;
    sr.c = 0;
}

template<Instr I, Mode M, Size S> void
Moira::execCmp(u16 opcode)
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
Moira::execCmpa(u16 opcode)
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
Moira::execCmpi(u16 opcode)
{
    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;
    prefetch();

    cmp<S>(src, data);
}

template<Instr I, Mode M, Size S> void
Moira::execCmpm(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, ea2, data1, data2;

    if (!readOperand<M,S>(src, ea1, data1)) return;
    if (!readOperand<M,S>(dst, ea2, data2)) return;

    cmp<S>(data1, data2);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execDbcc(u16 opcode)
{
    int dn = _____________xxx(opcode);

    /* IF (condition false)
     *   THEN [Dn] := [Dn] - 1 {decrement loop counter}
     *     IF [Dn] == -1 THEN [PC] ← [PC] + 2 {fall through to next instruction}
     *                   ELSE [PC] ← [PC] + d {take branch}
     * ELSE [PC] := [PC] + 2 {fall through to next instruction}
     */

    if (!bcond<I>()) {

        u32 newpc = reg.pc + (i16)irc;

        // Decrement loop counter
        writeD<Word>(dn, readD<Word>(dn) - 1);

        // Take branch if Dn does not equal -1
        if ((i16)readD<Word>(dn) != -1) {

            reg.pc = newpc;
            fullPrefetch();
            return;
        } else {
            (void)memory->moiraRead16(reg.pc + 2);
        }
    }

    // Fall through to next instruction
    reg.pc += 2;
    fullPrefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execExgDxDy(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    prefetch();
    std::swap(reg.d[src], reg.d[dst]);
}

template<Instr I, Mode M, Size S> void
Moira::execExgAxDy(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    prefetch();
    std::swap(reg.a[src], reg.d[dst]);
}

template<Instr I, Mode M, Size S> void
Moira::execExgAxAy(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    prefetch();
    std::swap(reg.a[src], reg.a[dst]);
}

template<Instr I, Mode M, Size S> void
Moira::execExt(u16 opcode)
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
Moira::execJmp(u16 opcode)
{
    u32 ea;
    int src = _____________xxx(opcode);

    ea = computeEA<M,Long,SKIP_LAST_READ>(src);

    reg.pc = ea;
    fullPrefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execJsr(u16 opcode)
{
    int src = _____________xxx(opcode);

    u32 ea = computeEA<M,Long,SKIP_LAST_READ>(src);
    u32 pc = reg.pc;

    reg.pc = ea;
    irc = memory->moiraRead16(reg.pc);
    push(pc + 2);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execLea(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    reg.a[dst] = computeEA<M,S>(src);
    if (isIndexMode(M)) sync(2);

    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execLink(u16 opcode)
{
    int ax   = _____________xxx(opcode);
    i16 disp = (i16)irc;

    readExtensionWord();
    push(readA(ax) - (ax == 7 ? 4 : 0));
    prefetch();
    writeA(ax, reg.sp);
    reg.sp += (i32)disp;
}

template<Instr I, Mode M1, Mode M2, Size S> void
Moira::execMove(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, data;

    /* Check for mode combinations that show special timing behaviour
     * http://pasti.fxatari.com/68kdocs/68kPrefetch.html
     */
    if (M2 == 4) {

        if (!readOperand<M1,S>(src, ea1, data)) return;
        prefetch();
        if (!writeOperand<M2,S>(dst, data)) return;

    } else if (M2 == 8 && M1 >= 2 && M1 <= 10) {

        u32 ea2;

        if (!readOperand<M1,S>(src, ea1, data)) return;
        ea2 = irc << 16;
        readExtensionWord();
        ea2 |= irc;
        writeM<Long>(ea2, data);
        readExtensionWord();
        prefetch();

    } else {

        if (!readOperand<M1,S>(src, ea1, data)) return;
        if (!writeOperand<M2,S>(dst, data)) return;
        prefetch();
    }

    sr.c = 0;
    sr.v = 0;
    sr.n = NBIT<S>(data);
    sr.z = ZERO<S>(data);
}

template<Instr I, Mode M, Size S> void
Moira::execMovea(u16 opcode)
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

            u32 ea, data;
            if (!readOperand<M,S>(src, ea, data)) return;

            result = SIGN<S>(data);
            break;
        }
    }

    writeA(dst, result);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execMovemEaRg(u16 opcode)
{
    int src  = _____________xxx(opcode);
    u16 mask = irc;

    u32 ea = computeEA<M,S>(src);
    if (addressErrorDeprecated<M,S>(ea)) return; // TODO: Trigger exception

    readExtensionWord();
    if (S == Long) dummyRead(ea);

    switch (M) {

        case 3: // (An)+
        {
            for(int i = 0; i <= 15; i++) {

                if (mask & (1 << i)) {
                    writeR(i, SEXT<S>(readM<S>(ea)));
                    ea += S;
                }
            }
            writeA(src, ea);
            break;
        }
        default:
        {
            for(int i = 0; i <= 15; i++) {

                if (mask & (1 << i)) {
                    writeR(i, SEXT<S>(readM<S>(ea)));
                    ea += S;
                }
            }
            break;
        }
    }
    if (S == Word) (void)readM<Word>(ea);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execMovemRgEa(u16 opcode)
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
            if (addressErrorDeprecated<M,S>(ea)) return; // TODO: Trigger exception

            readExtensionWord();

            for(int i = 15; i >= 0; i--) {

                if (mask & (0x8000 >> i)) {
                    ea -= S;
                    writeM<S>(ea, reg.r[i]);
                }
            }
            writeA(dst, ea);
            break;
        }
        default:
        {
            u32 ea = computeEA<M,S>(dst);
            if (addressErrorDeprecated<M,S>(ea)) return; // TODO: Trigger exception

            readExtensionWord();

            for(int i = 0; i < 16; i++) {

                if (mask & (1 << i)) {
                    writeM<S>(ea, reg.r[i]);
                    ea += S;
                }
            }
            break;
        }
    }
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execMovepDxEa(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea = computeEA<M,S>(dst);
    if (addressErrorDeprecated<M,S>(ea)) return;

    u32 dx = readD(src);

    switch (S) {

        case Long:
        {
            writeM<Byte>(ea, (dx >> 24) & 0xFF); ea += 2;
            writeM<Byte>(ea, (dx >> 16) & 0xFF); ea += 2;
        }
        case Word:
        {
            writeM<Byte>(ea, (dx >>  8) & 0xFF); ea += 2;
            writeM<Byte>(ea, (dx >>  0) & 0xFF);
        }
    }
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execMovepEaDx(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea = computeEA<M,S>(src);
    if (addressErrorDeprecated<M,S>(ea)) return;

    u32 dx = 0;

    switch (S) {

        case Long:
        {
            dx |= readM<Byte>(ea) << 24; ea += 2;
            dx |= readM<Byte>(ea) << 16; ea += 2;
        }
        case Word:
        {
            dx |= readM<Byte>(ea) << 8; ea += 2;
            dx |= readM<Byte>(ea) << 0;
        }

    }
    writeD(dst, dx);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execMoveq(u16 opcode)
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
Moira::execMoveToCcr(u16 opcode)
{
    int src = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(src, ea, data)) return;
    setCCR(data);

    dummyRead(reg.pc + 2);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execMoveFromSr(u16 opcode)
{
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;
    prefetch();

    writeOperand<M,S>(dst, ea, getSR());
}

template<Instr I, Mode M, Size S> void
Moira::execMoveToSr(u16 opcode)
{
    // This instruction requires supervisor mode
    if (!sr.s) { privilegeException(); return; }

    int src = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(src, ea, data)) return;
    setSR(data);

    dummyRead(reg.pc + 2);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execMoveUspAn(u16 opcode)
{
    int an = _____________xxx(opcode);

    // This instruction requires supervisor mode
    if (!sr.s) { privilegeException(); return; }

    prefetch();
    writeA(an, reg.usp);
}

template<Instr I, Mode M, Size S> void
Moira::execMoveAnUsp(u16 opcode)
{
    int an = _____________xxx(opcode);

    // This instruction requires supervisor mode
    if (!sr.s) { privilegeException(); return; }

    prefetch();
    reg.usp = readA(an);
}

template<Instr I, Mode M, Size S> void
Moira::execMulDiv(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);
    u16 dn  = readD<Word>(dst);
    u32 ea, data;

    if (!readOperand<M, Word>(src, ea, data)) return;

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
Moira::execNbcd(u16 opcode)
{
    int reg = _____________xxx(opcode);

    switch (M) {

        case 0: // Dn
        {
            prefetch();
            writeD<Byte>(reg, arith<SBCD,Byte>(readD<Byte>(reg), 0));
            break;
        }
        default: // Ea
        {
            assert(M >= 2 && M <= 8);
            u32 ea, data;
            if (!readOperand<M,Byte>(reg, ea, data)) return;
            prefetch();
            writeM<Byte>(ea, arith<SBCD,Byte>(data, 0));
            break;
        }
    }
}

template<Instr I, Mode M, Size S> void
Moira::execNegNot(u16 opcode)
{
    int dst = ( _____________xxx(opcode) );
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
Moira::execNop(u16 opcode)
{

}

template<Instr I, Mode M, Size S> void
Moira::execPea(u16 opcode)
{
    int src = _____________xxx(opcode);

    u32 ea = computeEA<M,Long>(src);

    if (isAbsMode(M)) {
        push(ea);
        prefetch();
    } else {
        prefetch();
        push(ea);
    }
}

template<Instr I, Mode M, Size S> void
Moira::execReset(u16 opcode)
{
    // This instruction requires supervisor mode
    if (!sr.s) { privilegeException(); return; }

    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execRte(u16 opcode)
{
    // This instruction requires supervisor mode
     if (!sr.s) { privilegeException(); return; }

    setSR(readM<Word>(reg.sp));
    reg.sp += 2;

    u32 newpc = readM<Long>(reg.sp);
    reg.sp += 4;

    readExtensionWord();
    reg.pc = newpc;
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execRtr(u16 opcode)
{
    setCCR(readM<Word>(reg.sp));
    reg.sp += 2;
    u32 newpc = readM<Long>(reg.sp);
    reg.sp += 4;

    readExtensionWord();
    reg.pc = newpc;
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execRts(u16 opcode)
{
    u32 newpc = readM<Long>(reg.sp);
    reg.sp += 4;

    readExtensionWord();
    reg.pc = newpc;
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execScc(u16 opcode)
{
    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if (!readOperand<M,Byte>(dst, ea, data)) return;

    prefetch();
    writeOperand<M,Byte>(dst, ea, bcond<I>() ? 0xFF : 0);
}

template<Instr I, Mode M, Size S> void
Moira::execStop(u16 opcode)
{
    // This instruction requires supervisor mode
    if (!sr.s) { privilegeException(); return; }

    setSR(irc | 1 << 13);
    readExtensionWord();

    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execSwap(u16 opcode)
{
    int reg = ( _____________xxx(opcode) );
    u32 dat = readD(reg);

    prefetch();

    dat = (dat >> 16) | (dat & 0xFFFF) << 16;
    writeD(reg,dat);

    sr.c = 0;
    sr.v = 0;
    sr.n = NBIT<Long>(dat);
    sr.z = ZERO<Long>(dat);
}

template<Instr I, Mode M, Size S> void
Moira::execTas(u16 opcode)
{
    int dst = ( _____________xxx(opcode) );

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
            u8 data = readM<Byte>(ea);

            sr.c = 0;
            sr.v = 0;
            sr.n = NBIT<Byte>(data);
            sr.z = ZERO<Byte>(data);

            data |= 0x80;
            writeM<Byte>(ea, data);
            break;
        }
    }
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execTrap(u16 opcode)
{
    int nr = ____________xxxx(opcode) + 32;

    execTrapException(nr);
}

template<Instr I, Mode M, Size S> void
Moira::execTrapv(u16 opcode)
{
    prefetch();
    if (sr.v) execTrapException(7);
}

template<Instr I, Mode M, Size S> void
Moira::execTst(u16 opcode)
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

            u32 ea;
            if (!readOperand<M,S>(reg, ea, value)) return;
            break;
        }
    }

    sr.c = 0;
    sr.v = 0;
    sr.n = NBIT<S>(value);
    sr.z = ZERO<S>(value);

    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execUnlk(u16 opcode)
{
    int an = _____________xxx(opcode);
    reg.sp = readA(an);

    u32 ea, data;
    if (!readOperand<2,Long>(7, ea, data)) return;
    writeA(an, data);

    if (an != 7) reg.sp += 4;
    prefetch();
}

