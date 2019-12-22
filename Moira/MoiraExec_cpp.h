// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include <utility>

#define SUPERVISOR_MODE_INSTR if (!sr.s) { privilegeException(); return; }
#define LAST_BUS_CYCLE true

void
Moira::saveToStackDetailed(u16 sr, u32 addr, u16 code)
{
    // Push PC
    push<Word>((u16)reg.pc);
    push<Word>(reg.pc >> 16);

    // Push SR and IRD
    push<Word>(sr);
    push<Word>(ird);
    
    // Push address
    push<Word>((u16)addr);
    push<Word>(addr >> 16);

    // Push memory access type and function code
    push<Word>(code);
}

void
Moira::saveToStackBrief(u16 sr)
{
    // Push PC and SR
    push<Word>((u16)reg.pc);
    push<Word>(reg.pc >> 16);
    push<Word>(sr);
}

void
Moira::execAddressError(u32 addr)
{
    assert(addr & 1);

    // Memory access type and function code (TODO: THIS IS INCOMPLETE)
    u16 code = 0x11 | (sr.s ? 4 : 0);

    // Enter supervisor mode and update the status register
    setSupervisorMode(true);
    sr.t = 0;

    // Write exception information to stack
    sync(8);
    saveToStackDetailed(getSR(), addr, code);
    sync(2);
    
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

    // Write exception information to stack
    sync(4);
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

    // Write exception information to stack
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

    // Write exception information to stack
    sync(4);
    saveToStackBrief(status);

    jumpToVector(8);
}

template<Instr I, Mode M, Size S> void
Moira::execShiftRg(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    int cnt = readD(src) & 0x3F;

    prefetch<LAST_BUS_CYCLE>();
    sync((S == Long ? 4 : 2) + 2 * cnt);

    writeD<S>(dst, shift<I,S>(cnt, readD<S>(dst)));
}

template<Instr I, Mode M, Size S> void
Moira::execShiftIm(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    int cnt = src ? src : 8;

    prefetch<LAST_BUS_CYCLE>();
    sync((S == Long ? 4 : 2) + 2 * cnt);

    writeD<S>(dst, shift<I,S>(cnt, readD<S>(dst)));
}

template<Instr I, Mode M, Size S> void
Moira::execShiftEa(u16 op)
{
    int src = ____xxx_________(op);

    u32 ea, data;
    if (!readOperand<M,S>(src, ea, data)) return;

    prefetch();

    writeM<S,LAST_BUS_CYCLE>(ea, shift<I,S>(1, data));
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
            prefetch<LAST_BUS_CYCLE>();

            sync(S == Long ? 6 : 2);
            writeD<Byte>(dst, result);
            break;
        }
        default: // Ea
        {
            u32 ea1, ea2, data1, data2;
            if (!readOperand<M,S>(src, ea1, data1)) return;
            sync(-2);
            if (!readOperand<M,S>(dst, ea2, data2)) return;

            u32 result = arith<I,Byte>(data1, data2);
            prefetch();

            writeM<Byte,LAST_BUS_CYCLE>(ea2, result);
            break;
        }
    }
}

template<Instr I, Mode M, Size S> void
Moira::execAddEaRg(u16 opcode)
{
    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOperand<M,S>(src, ea, data)) return;

    result = arith<I,S>(data, readD<S>(dst));
    prefetch<LAST_BUS_CYCLE>();

    if (S == Long) sync(4);
    writeD<S>(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddRgEa(u16 opcode)
{
    u32 ea, data, result;

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    if (!readOperand<M,S>(dst, ea, data)) return;
    result = arith<I,S>(readD<S>(src), data);

    prefetch();
    writeM<S,LAST_BUS_CYCLE>(ea, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAdda(u16 opcode)
{
    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOperand<M,S>(src, ea, data)) return;
    data = SEXT<S>(data);

    result = (I == ADDA) ? readA(dst) + data : readA(dst) - data;
    prefetch<LAST_BUS_CYCLE>();

    sync((S == Word || isRegMode(M) || isImmMode(M)) ? 4 : 2);
    writeA(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddiRg(u16 opcode)
{
    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    u32 ea, data, result;
    if (!readOperand<M,S>(dst, ea, data)) return;

    result = arith<I,S>(src, data);
    prefetch<LAST_BUS_CYCLE>();

    if (S == Long) sync(4);
    writeD(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddiEa(u16 opcode)
{
    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    u32 ea, data, result;
    if (!readOperand<M,S>(dst, ea, data)) return;

    result = arith<I,S>(src, data);
    prefetch();

    writeOperand<M,S,LAST_BUS_CYCLE>(dst, ea, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddqDn(u16 opcode)
{
    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    if (src == 0) src = 8;
    u32 result = arith<I,S>(src, readD<S>(dst));
    prefetch<LAST_BUS_CYCLE>();

    if (S == Long) sync(4);
    writeD<S>(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddqAn(u16 opcode)
{
    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    if (src == 0) src = 8;
    u32 result = (I == ADDQ) ? readA(dst) + src : readA(dst) - src;
    prefetch<LAST_BUS_CYCLE>();

    sync(4);
    writeA(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddqEa(u16 opcode)
{
    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea, data, result;
    if (!readOperand<M,S>(dst, ea, data)) return;

    if (src == 0) src = 8;
    result = arith<I,S>(src, data);
    prefetch();

    writeOperand<M,S,LAST_BUS_CYCLE>(dst, ea, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddxRg(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 result = arith<I,S>(readD<S>(src), readD<S>(dst));
    prefetch();

    if (S == Long) sync(4);
    writeD<S>(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAddxEa(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, ea2, data1, data2;
    if (!readOperand<M,S>(src, ea1, data1)) return;
    sync(-2);
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
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(src, ea, data)) return;

    u32 result = logic<I,S>(data, readD<S>(dst));
    prefetch<LAST_BUS_CYCLE>();

    if (S == Long) sync(isRegMode(M) || isImmMode(M) ? 4 : 2);
    writeD<S>(dst, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAndRgEa(u16 opcode)
{
    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;

    u32 result = logic<I,S>(readD<S>(src), data);
    isMemMode(M) ? prefetch() : prefetch<LAST_BUS_CYCLE>();

    writeOperand<M,S,LAST_BUS_CYCLE>(dst, ea, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAndi(u16 opcode)
{
    u32 ea, data, result;

    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    if (!readOperand<M,S>(dst, ea, data)) return;

    result = logic<I,S>(src, data);
    prefetch();

    writeOperand<M,S>(dst, ea, result);
}

template<Instr I, Mode M, Size S> void
Moira::execAndiccr(u16 opcode)
{
    u32 src = readImm<S>();
    u8  dst = getCCR();

    sync(8);

    u32 result = logic<I,S>(src, dst);
    setCCR(result);

    dummyRead(reg.pc+2);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execAndisr(u16 opcode)
{
    SUPERVISOR_MODE_INSTR

    u32 src = readImm<S>();
    u16 dst = getSR();

    sync(8);
    
    u32 result = logic<I,S>(src, dst);
    setSR(result);

    dummyRead(reg.pc+2);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execBcc(u16 opcode)
{
    sync(2);
    if (bcond<I>()) {

        // i16 offset = S == Word ? (i16)irc : (i8)opcode;
        u32 newpc = reg.pc + (S == Word ? (i16)irc : (i8)opcode);

        // Take branch
        reg.pc = newpc;
        fullPrefetch();

    } else {

        // Fall through to next instruction
        sync(2);
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

            prefetch<LAST_BUS_CYCLE>();

            sync(cyclesBit<I>(bit));
            if (I != BTST) writeD(dst, data);
            break;
        }
        default:
        {
            u8 bit = readD(src) & 0b111;

            u32 ea, data;
            if (!readOperand<M,Byte>(dst, ea, data)) return;

            data = bitop<I>(data, bit);

            if (I != BTST) {
                prefetch();
                writeM<Byte,LAST_BUS_CYCLE>(ea, data);
            } else {
                prefetch<LAST_BUS_CYCLE>();
            }
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

            prefetch<LAST_BUS_CYCLE>();

            sync(cyclesBit<I>(bit));
            if (I != BTST) writeD(dst, data);
            break;
        }
        default:
        {
            u32 ea, data;
            if (!readOperand<M,Byte>(dst, ea, data)) return;

            data = bitop<I>(data, bit);

            if (I != BTST) {
                prefetch();
                writeM<Byte,LAST_BUS_CYCLE>(ea, data);
            } else {
                prefetch<LAST_BUS_CYCLE>();
            }
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
    sync(2);
    push<Long>(retpc);

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
    sync(4);
    
    sr.z = ZERO<S>(dop);
    sr.v = 0;
    sr.c = 0;
    sr.n = 0;

    if ((i16)dop > (i16)sop) {
        sync(4);
        execTrapException(6);
        return;
    }

    sync(2);

    if ((i16)dop < 0) {
        sr.n = 1;
        sync(4);
        execTrapException(6);
    }
}

template<Instr I, Mode M, Size S> void
Moira::execClr(u16 opcode)
{
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;

    isMemMode(M) ? prefetch() : prefetch<LAST_BUS_CYCLE>();

    if (S == Long && isRegMode(M)) sync(2);
    writeOperand<M,S,LAST_BUS_CYCLE>(dst, ea, 0);

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

    u32 ea, data;
    if (!readOperand<M,S>(src, ea, data)) return;

    cmp<S>(data, readD<S>(dst));
    prefetch<LAST_BUS_CYCLE>();

    if (S == Long) sync(2);
}

template<Instr I, Mode M, Size S> void
Moira::execCmpa(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(src, ea, data)) return;

    data = SEXT<S>(data);
    cmp<Long>(data, readA(dst));
    prefetch<LAST_BUS_CYCLE>();

    sync(2);
}

template<Instr I, Mode M, Size S> void
Moira::execCmpi(u16 opcode)
{
    u32 src = readImm<S>();
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(dst, ea, data)) return;
    prefetch();

    if (S == Long && isRegMode(M)) sync(2);
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
    sync(2);
    if (!bcond<I>()) {

        int dn = _____________xxx(opcode);
        u32 newpc = reg.pc + (i16)irc;

        // Decrement loop counter
        writeD<Word>(dn, readD<Word>(dn) - 1);

        // Take branch if Dn does not equal -1
        if ((i16)readD<Word>(dn) != -1) {

            reg.pc = newpc;
            fullPrefetch<LAST_BUS_CYCLE>();
            return;
        } else {
            dummyRead(reg.pc + 2);
        }
    } else {
        sync(2);
    }

    // Fall through to next instruction
    reg.pc += 2;
    fullPrefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execExgDxDy(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    prefetch();
    sync(2);
    std::swap(reg.d[src], reg.d[dst]);
}

template<Instr I, Mode M, Size S> void
Moira::execExgAxDy(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    prefetch();
    sync(2);
    std::swap(reg.a[src], reg.d[dst]);
}

template<Instr I, Mode M, Size S> void
Moira::execExgAxAy(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    prefetch();
    sync(2);
    std::swap(reg.a[src], reg.a[dst]);
}

template<Instr I, Mode M, Size S> void
Moira::execExt(u16 opcode)
{
    int n = _____________xxx(opcode);

    u32 dn = readD(n);
    dn = (S == Long) ? SEXT<Word>(dn) : SEXT<Byte>(dn);

    writeD<S>(n, dn);
    sr.setNZVC<S>(dn);

    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execJmp(u16 opcode)
{
    int src = _____________xxx(opcode);
    u32 ea  = computeEA<M,Long,SKIP_LAST_READ>(src);

    const int delay[] = { 0,0,0,0,0,2,4,2,0,2,4,0 };
    sync(delay[M]);

    // Jump to new address
    reg.pc = ea;

    // Fill the prefetch queue
    fullPrefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execJsr(u16 opcode)
{
    printf("execJsr(1)\n");

    int src = _____________xxx(opcode);
    u32 ea  = computeEA<M,Long,SKIP_LAST_READ>(src);

    const int delay[] = { 0,0,0,0,0,2,4,2,0,2,4,0 };
    sync(delay[M]);

    // Jump to new address
    u32 pc = reg.pc;
    reg.pc = ea;

    if (addressError<Word>(ea)) return;
    irc = readM<Word>(reg.pc);
    push<Long>(pc + 2);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execLea(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    reg.a[dst] = computeEA<M,S>(src);
    if (isIdxMode(M)) sync(2);

    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execLink(u16 opcode)
{
    int ax   = _____________xxx(opcode);
    i16 disp = (i16)irc;

    readExtensionWord();
    push<Long>(readA(ax) - (ax == 7 ? 4 : 0));
    prefetch();
    writeA(ax, reg.sp);
    reg.sp += (i32)disp;
}

template<Instr I, Mode M, Size S> void
Moira::execMove0(u16 opcode)
{
    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOperand<M,S>(src, ea, data)) return;
    sr.setNZVC<S>(data);
    if (!writeOperand<0,S>(dst, data)) return;
    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execMove2(u16 opcode)
{
    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOperand<M,S>(src, ea, data)) return;
    sr.setNZVC<S>(data);
    if (!writeOperand<2,S>(dst, data)) return;
    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execMove3(u16 opcode)
{
    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOperand<M,S>(src, ea, data)) return;
    sr.setNZVC<S>(data);
    if (!writeOperand<3,S>(dst, data)) return;
    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execMove4(u16 opcode)
{
    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    /* Source: http://pasti.fxatari.com/68kdocs/68kPrefetch.html
     *
     * "When the destination addressing mode is pre-decrement, steps 4 and 5
     *  above are inverted. So it behaves like a read modify instruction and it
     *  is a class 0 instruction. Note: The behavior is the same disregarding
     *  transfer size (byte, word or long), and disregarding the source
     *  addressing mode."
     */

    if (!readOperand<M,S>(src, ea, data)) return;
    sr.setNZVC<S>(data);
    prefetch();
    sync(-2);
    if (!writeOperand<4,S,LAST_BUS_CYCLE>(dst, data)) return;
}

template<Instr I, Mode M, Size S> void
Moira::execMove5(u16 opcode)
{
    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOperand<M,S>(src, ea, data)) return;
    sr.setNZVC<S>(data);
    if (!writeOperand<5,S>(dst, data)) return;
    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execMove6(u16 opcode)
{
    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOperand<M,S>(src, ea, data)) return;
    sr.setNZVC<S>(data);
    if (!writeOperand<6,S>(dst, data)) return;
    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execMove7(u16 opcode)
{
    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOperand<M,S>(src, ea, data)) return;
    sr.setNZVC<S>(data);
    if (!writeOperand<7,S>(dst, data)) return;
    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execMove8(u16 opcode)
{
    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    /* Source: http://pasti.fxatari.com/68kdocs/68kPrefetch.html
     *
     * "When the destination addressing mode is long absolute and the source
     *  operand is any memory addr.mode, step 4 is interleaved in the middle of
     *  step 3. Step 3 only performs a single prefetch in this case. The other
     *  prefetch cycle that is normally performed at that step is deferred
     *  after the write cycles. So, two prefetch cycles are performed after the
     *  write ones. It is a class 2 instruction. Note: The behavior is the same
     *  disregarding transfer size (byte, word or long). But if the source
     *  operand is a data or address register, or immediate, then the behavior
     *  is the same as other MOVE variants (class 1 instruction)."
     */
    if (isMemMode(M)) {

        if (!readOperand<M,S>(src, ea, data)) return;
        sr.setNZVC<S>(data);
        u32 ea2 = irc << 16;
        readExtensionWord();
        ea2 |= irc;
        writeM<Long>(ea2, data);
        readExtensionWord();

    } else {

        if (!readOperand<M,S>(src, ea, data)) return;
        sr.setNZVC<S>(data);
        if (!writeOperand<8,S>(dst, data)) return;
    }

    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execMovea(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(src, ea, data)) return;

    prefetch<LAST_BUS_CYCLE>();
    writeA(dst, SIGN<S>(data));
}

template<Instr I, Mode M, Size S> void
Moira::execMovemEaRg(u16 opcode)
{
    int src  = _____________xxx(opcode);
    u16 mask = irc;

    u32 ea = computeEA<M,S>(src);
    if (addressError<S>(ea)) return;

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
    if (S == Word) dummyRead(ea);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execMovemRgEa(u16 opcode)
{
    int dst  = _____________xxx(opcode);
    u16 mask = irc;

    switch (M) {

        case 4: // -(An)
        {
            u32 ea = readA(dst);
            if (addressError<S>(ea)) return;

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
            if (addressError<S>(ea)) return;

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
    if (addressError<S>(ea)) return;

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
    if (addressError<S>(ea)) return;

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

    sr.setNZVC<Byte>(src);
    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execMoveToCcr(u16 opcode)
{
    int src = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(src, ea, data)) return;

    sync(4);
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

    if (isRegMode(M) || isImmMode(M)) sync(2);
    writeOperand<M,S>(dst, ea, getSR());
}

template<Instr I, Mode M, Size S> void
Moira::execMoveToSr(u16 opcode)
{
    SUPERVISOR_MODE_INSTR

    int src = _____________xxx(opcode);

    u32 ea, data;
    if (!readOperand<M,S>(src, ea, data)) return;

    sync(4);
    setSR(data);

    dummyRead(reg.pc + 2);
    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execMoveUspAn(u16 opcode)
{
    SUPERVISOR_MODE_INSTR

    int an = _____________xxx(opcode);

    prefetch();
    writeA(an, reg.usp);
}

template<Instr I, Mode M, Size S> void
Moira::execMoveAnUsp(u16 opcode)
{
    SUPERVISOR_MODE_INSTR

    int an = _____________xxx(opcode);
    prefetch();
    reg.usp = readA(an);
}

template<Instr I, Mode M, Size S> void
Moira::execMul(u16 opcode)
{
    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOperand<M, Word>(src, ea, data)) return;

    prefetch<LAST_BUS_CYCLE>();

    switch (I) {

        case MULS: // Signed multiplication
        {
            result = (i16)data * (i16)readD<Word>(dst);
            sync(cyclesMULS(data));
            break;
        }
        case MULU: // Unsigned multiplication
        {
            result = data * readD<Word>(dst);
            sync(cyclesMULU(data));
            break;
        }
    }

    writeD(dst, result);
    sr.setNZVC<Long>(result);
}

template<Instr I, Mode M, Size S> void
Moira::execDiv(u16 opcode)
{
    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    if (!readOperand<M, Word>(src, ea, data)) return;

    sr.n = sr.z = sr.v = sr.c = 0;

    // Check for division by zero
    if (data == 0) {
        sync(8);
        return execTrapException(5);
    }

    u32 dividend = readD(dst);

    switch (I) {

        case DIVS: // Signed division
        {
            sync(cyclesDIVS(dividend, data) - 4);

            if (dividend == 0x80000000 && (i16)data == -1) {
                sr.v = sr.n = 1;
                break;
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
            sync(cyclesDIVU(dividend, data) - 4);

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

    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execNbcd(u16 opcode)
{
    int reg = _____________xxx(opcode);

    switch (M) {

        case 0: // Dn
        {
            prefetch<LAST_BUS_CYCLE>();

            sync(2);
            writeD<Byte>(reg, arith<SBCD,Byte>(readD<Byte>(reg), 0));
            break;
        }
        default: // Ea
        {
            u32 ea, data;
            if (!readOperand<M,Byte>(reg, ea, data)) return;
            prefetch();
            writeM<Byte,LAST_BUS_CYCLE>(ea, arith<SBCD,Byte>(data, 0));
            break;
        }
    }
}

template<Instr I, Mode M, Size S> void
Moira::execNegRg(u16 opcode)
{
    printf("Moira::execNegRg(u16 opcode)");

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if (!readOperand<M,S>(dst, ea, data)) return;

    data = logic<I,S>(data);
    prefetch<LAST_BUS_CYCLE>();

    if (S == Long) sync(2);
    writeD<S>(dst, data);
}

template<Instr I, Mode M, Size S> void
Moira::execNegEa(u16 opcode)
{
    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if (!readOperand<M,S>(dst, ea, data)) return;

    data = logic<I,S>(data);
    prefetch();

    writeOperand<M,S,LAST_BUS_CYCLE>(dst, ea, data);
}

template<Instr I, Mode M, Size S> void
Moira::execNop(u16 opcode)
{
    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execPea(u16 opcode)
{
    int src = _____________xxx(opcode);

    u32 ea = computeEA<M,Long>(src);

    if (isAbsMode(M)) {
        push<Long>(ea);
        prefetch();
    } else {
        prefetch();
        push<Long>(ea);
    }
}

template<Instr I, Mode M, Size S> void
Moira::execReset(u16 opcode)
{
    SUPERVISOR_MODE_INSTR

    prefetch();
}

template<Instr I, Mode M, Size S> void
Moira::execRte(u16 opcode)
{
    SUPERVISOR_MODE_INSTR

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
Moira::execSccRg(u16 opcode)
{
    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if (!readOperand<M,Byte>(dst, ea, data)) return;

    data = bcond<I>() ? 0xFF : 0;
    prefetch<LAST_BUS_CYCLE>();

    if (data) sync(2);
    writeD<Byte>(dst, data);
}

template<Instr I, Mode M, Size S> void
Moira::execSccEa(u16 opcode)
{
    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if (!readOperand<M,Byte>(dst, ea, data)) return;

    data = bcond<I>() ? 0xFF : 0;
    prefetch();

    writeOperand<M,Byte,LAST_BUS_CYCLE>(dst, ea, data);
}

template<Instr I, Mode M, Size S> void
Moira::execStop(u16 opcode)
{
    SUPERVISOR_MODE_INSTR

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
Moira::execTasRg(u16 opcode)
{
    int dst = ( _____________xxx(opcode) );

    u32 ea, data;
    readOperand<M,Byte>(dst, ea, data);
    sr.setNZVC<Byte>(data);

    data |= 0x80;
    writeD<S>(dst, data);

    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execTasEa(u16 opcode)
{
    int dst = ( _____________xxx(opcode) );

    u32 ea, data;
    readOperand<M,Byte>(dst, ea, data);
    sr.setNZVC<Byte>(data);
    data |= 0x80;

    sync(2);
    writeOperand<M,S>(dst, ea, data);

    prefetch<LAST_BUS_CYCLE>();
}

template<Instr I, Mode M, Size S> void
Moira::execTrap(u16 opcode)
{
    int nr = ____________xxxx(opcode) + 32;

    sync(4);
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

    u32 ea, data;
    if (!readOperand<M,S>(reg, ea, data)) return;

    sr.setNZVC<S>(data);
    prefetch<LAST_BUS_CYCLE>();
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

