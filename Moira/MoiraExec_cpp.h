// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#define SUPERVISOR_MODE_ONLY if (!reg.sr.s) { execPrivilegeException(); return; }

#define REVERSE_8(x) (u8)(((x) * 0x0202020202ULL & 0x010884422010ULL) % 1023)
#define REVERSE_16(x) (u16)((REVERSE_8((x) & 0xFF) << 8) | REVERSE_8(((x) >> 8) & 0xFF))
#define U32_ADD(x,y) (u32)((i64)(x) + (i64)(y))
#define U32_SUB(x,y) (u32)((i64)(x) - (i64)(y))
#define U32_INC(x,y) x = U32_ADD(x,y)
#define U32_DEC(x,y) x = U32_SUB(x,y)

#define ______________xx(opcode) (u8)((opcode >> 0)  & 0b11)
#define _____________xxx(opcode) (u8)((opcode >> 0)  & 0b111)
#define ____________xxxx(opcode) (u8)((opcode >> 0)  & 0b1111)
#define ___________xxxxx(opcode) (u8)((opcode >> 0)  & 0b11111)
#define __________xxxxxx(opcode) (u8)((opcode >> 0)  & 0b111111)
#define ________xxxxxxxx(opcode) (u8)((opcode >> 0)  & 0b11111111)
#define ____xxxxxxxxxxxx(opcode) (u16)((opcode >> 0) & 0b111111111111)
#define __________xxx___(opcode) (u8)((opcode >> 3)  & 0b111)
#define __________xx____(opcode) (u8)((opcode >> 4)  & 0b11)
#define _______xxx______(opcode) (u8)((opcode >> 6)  & 0b111)
#define ______xxxx______(opcode) (u8)((opcode >> 6)  & 0b1111)
#define _____xxxxx______(opcode) (u8)((opcode >> 6)  & 0b11111)
#define _________x______(opcode) (u8)((opcode >> 6)  & 0b1)
#define ________x_______(opcode) (u8)((opcode >> 7)  & 0b1)
#define _______x________(opcode) (u8)((opcode >> 8)  & 0b1)
#define _____xx_________(opcode) (u8)((opcode >> 9)  & 0b11)
#define ____xxx_________(opcode) (u8)((opcode >> 9)  & 0b111)
#define ____x___________(opcode) (u8)((opcode >> 11) & 0b1)
#define _xxx____________(opcode) (u8)((opcode >> 12) & 0b111)
#define xxxx____________(opcode) (u8)((opcode >> 12) & 0b1111)

#define STD_AE_FRAME \
(M == MODE_PD && S != Long) ? AE_INC_PC : \
(M == MODE_DI)              ? AE_DEC_PC : \
(M == MODE_IX)              ? AE_DEC_PC : \
(M == MODE_DIPC)            ? AE_DEC_PC : \
(M == MODE_IXPC)            ? AE_DEC_PC : 0

template <Core C, Instr I, Mode M, Size S> void
Moira::execLineA(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // Check if a software trap is set for this instruction
    if (debugger.swTraps.traps.contains(opcode)) {

        auto &trap = debugger.swTraps.traps[opcode];

        // Smuggle the original instruction back into the CPU
        reg.pc = reg.pc0;
        queue.irc = trap.instruction;
        prefetch <C> ();

        // Call the delegates
        signalSoftwareTrap(opcode, debugger.swTraps.traps[opcode]);
        swTrapReached(reg.pc0);
        return;
    }

    signalLineAException(opcode);
    execUnimplemented<C>(10);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execLineF(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    signalLineFException(opcode);
    execUnimplemented<C>(11);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execIllegal(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    signalIllegalOpcodeException(opcode);
    execUnimplemented<C>(4);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execShiftRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    int cnt = readD(src) & 0x3F;

    prefetch <C,POLLIPL> ();
    SYNC((S == Long ? 4 : 2) + 2 * cnt);

    writeD<S>(dst, shift<C, I, S>(cnt, readD<S>(dst)));
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execShiftIm(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);
    int cnt = src ? src : 8;

    prefetch <C,POLLIPL> ();
    SYNC((S == Long ? 4 : 2) + 2 * cnt);

    writeD<S>(dst, shift<C, I, S>(cnt, readD<S>(dst)));
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execShiftEa(u16 op)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(op);

    u32 ea, data;
    if (!readOp<C, M, S, STD_AE_FRAME>(src, ea, data)) return;

    looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();

    writeM <C,M,S> (ea, shift<C, I, S>(1, data));
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAbcd(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    assert(S == Byte);

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    switch (M) {

        case 0: // Dn
        {
            u32 result = bcd<C, I, Byte>(readD<Byte>(src), readD<Byte>(dst));
            prefetch <C,POLLIPL> ();
            SYNC(2);
            writeD<Byte>(dst, result);
            break;
        }
        default: // Ea
        {
            u32 ea1, ea2, data1, data2;
            if (!readOp<C, M, S>(src, ea1, data1)) return;
            pollIpl();
            if (!readOp<C, M, S, IMPLICIT_DECR>(dst, ea2, data2)) return;

            u32 result = bcd<C, I, Byte>(data1, data2);
            looping<I>() ? noPrefetch() : prefetch <C> ();

            writeM <C,M,Byte> (ea2, result);
            break;
        }
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddEaRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOp<C, M, S, STD_AE_FRAME>(src, ea, data)) return;

    result = addsub <C,I,S> (data, readD<S>(dst));

    looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();

    if (core == M68000) {
        if constexpr (S == Long) SYNC(2 + (isMemMode(M) ? 0 : 2));
    } else {
        if constexpr (S == Long) SYNC(2);
    }
    writeD<S>(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddRgEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data, result;

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    if (!readOp <C,M,S,STD_AE_FRAME> (dst, ea, data)) return;
    result = addsub <C,I,S> (readD<S>(src), data);

    looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();

    writeM <C,M,S> (ea, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAdda(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOp<C, M, S, STD_AE_FRAME>(src, ea, data)) return;
    data = SEXT<S>(data);

    result = (I == ADDA) ? U32_ADD(readA(dst), data) : U32_SUB(readA(dst), data);
    looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();

    SYNC(2);
    if (core == M68000) {
        if constexpr (S == Word || isRegMode(M) || isImmMode(M)) SYNC(2);
    } else {
        if constexpr (S == Word) SYNC(2);
    }

    writeA(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddiRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 src = readI <C,S> ();
    int dst = _____________xxx(opcode);

    u32 ea, data, result;
    if (!readOp <C,M,S> (dst, ea, data)) return;

    result = addsub <C,I,S> (src, data);
    prefetch <C,POLLIPL> ();

    if constexpr (S == Long) { SYNC_68000(4); SYNC_68010(2); }
    writeD<S>(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddiEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 src = readI <C,S> ();
    int dst = _____________xxx(opcode);

    u32 ea, data, result;
    if (!readOp <C,M,S,STD_AE_FRAME> (dst, ea, data)) return;

    result = addsub <C,I,S> (src, data);
    prefetch <C> ();

    writeOp <C,M,S,POLLIPL> (dst, ea, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddqDn(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    if (src == 0) src = 8;
    u32 result = addsub <C,I,S> (src, readD<S>(dst));
    prefetch <C,POLLIPL> ();

    if constexpr (S == Long) SYNC(4);
    writeD<S>(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddqAn(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    if (src == 0) src = 8;
    u32 result = (I == ADDQ) ? readA(dst) + src : readA(dst) - src;
    prefetch <C,POLLIPL> ();

    if (core == M68000 || S == Long) SYNC(4);
    writeA(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddqEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    i8  src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea, data, result;
    if (!readOp <C,M,S,STD_AE_FRAME> (dst, ea, data)) return;

    if (src == 0) src = 8;
    result = addsub <C,I,S> (src, data);
    prefetch <C,POLLIPL> ();

    writeOp<C, M, S>(dst, ea, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddxRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 result = addsub <C,I,S> (readD<S>(src), readD<S>(dst));
    prefetch <C,POLLIPL> ();

    if (core == M68000) {
        if constexpr (S == Long) SYNC(4);
    } else {
        if constexpr (S == Long) SYNC(2);
    }

    writeD<S>(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAddxEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    const u64 flags =
    (S == Word) ? AE_INC_PC : (S == Long) ? AE_INC_PC | AE_INC_ADDR : 0;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, ea2, data1, data2;

    if (!readOp <C,M,S,flags> (src, ea1, data1)) {
        if constexpr (S == Long) undoAnPD<M,S>(src);
        return;
    }
    if constexpr (S != Long) pollIpl();

    if (!readOp <C,M,S,flags|IMPLICIT_DECR> (dst, ea2, data2)) {
        if constexpr (S == Long) undoAnPD<M,S>(dst);
        return;
    }

    u32 result = addsub<C, I, S>(data1, data2);

    if constexpr (S == Long && !MIMIC_MUSASHI) {

        writeM <C, M, Word, POLLIPL> (ea2 + 2, result & 0xFFFF);
        looping<I>() ? noPrefetch() : prefetch <C> ();
        writeM <C, M, Word> (ea2, result >> 16);

    } else {

        looping<I>() ? noPrefetch() : prefetch <C> ();
        writeM <C,M,S> (ea2, result);
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndEaRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    if (!readOp <C,M,S,STD_AE_FRAME> (src, ea, data)) return;

    u32 result = logic <C,I,S> (data, readD<S>(dst));
    prefetch <C,POLLIPL> ();

    if (core == M68000) {
        if constexpr (S == Long) SYNC(isRegMode(M) || isImmMode(M) ? 4 : 2);
    } else {
        if constexpr (S == Long) SYNC(2);
    }

    writeD<S>(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndRgEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOp <C,M,S,STD_AE_FRAME> (dst, ea, data)) return;

    u32 result = logic <C,I,S> (readD<S>(src), data);
    looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();

    if (core == M68000) {
        if constexpr (S == Long && isRegMode(M)) SYNC(4);
    } else {
        if constexpr (S == Long && isRegMode(M)) SYNC(2);
    }

    if constexpr (MIMIC_MUSASHI) {
        writeOp <C,M,S> (dst, ea, result);
    } else {
        writeOp <C,M,S,REVERSE> (dst, ea, result);
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndiRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 src = readI <C,S> ();
    int dst = _____________xxx(opcode);

    u32 result = logic <C,I,S> (src, readD<S>(dst));
    prefetch <C,POLLIPL> ();

    if constexpr (S == Long) { SYNC_68000(4); SYNC_68010(2); }
    writeD<S>(dst, result);

    CYCLES_MBWL(MODE_DN,    8,  8,  2,      8,  8,  2,      16,  14,   2)
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndiEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data, result;

    u32 src = readI <C,S> ();
    int dst = _____________xxx(opcode);

    if (!readOp <C,M,S,STD_AE_FRAME> (dst, ea, data)) return;

    result = logic <C,I,S> (src, data);
    prefetch <C,POLLIPL> ();

    if constexpr (MIMIC_MUSASHI) {
        writeOp <C,M,S> (dst, ea, result);
    } else {
        writeOp <C,M,S,REVERSE> (dst, ea, result);
    }

    CYCLES_AI (16, 16,  8,      16, 16,  8,     28, 28,  8)
    CYCLES_PI (16, 16,  8,      16, 16,  8,     28, 28,  8)
    CYCLES_PD (18, 18,  9,      18, 18,  9,     30, 30,  9)
    CYCLES_DI (20, 20,  9,      20, 20,  9,     32, 32,  9)
    CYCLES_IX (22, 22, 11,      22, 22, 11,     34, 34, 11)
    CYCLES_AW (20, 20,  8,      20, 20,  8,     32, 32,  8)
    CYCLES_AL (24, 24,  8,      24, 24,  8,     36, 36,  8)
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndiccr(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 src = readI <C,S> ();
    u8  dst = getCCR();

    SYNC_68000(8);
    SYNC_68010(4);

    u32 result = logic <C,I,S> (src, dst);
    setCCR((u8)result);

    (void)readMS <C,MEM_DATA,Word> (reg.pc+2);
    prefetch <C,POLLIPL> ();

    CYCLES(20, 16, 12);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execAndisr(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    SUPERVISOR_MODE_ONLY

    u32 src = readI <C,S> ();
    u16 dst = getSR();

    SYNC_68000(8);
    SYNC_68010(4);

    u32 result = logic <C,I,S> (src, dst);
    setSR((u16)result);

    (void)readMS <C,MEM_DATA,Word> (reg.pc+2);
    prefetch <C,POLLIPL> ();

    CYCLES(20, 16, 12);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBcc(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    SYNC(2);
    if (cond(I)) {

        u32 newpc = U32_ADD(reg.pc, S == Word ? (i16)queue.irc : (i8)opcode);

        // Check for address error
        if (misaligned<Word>(newpc)) {
            execAddressError(makeFrame(newpc, reg.pc));
            return;
        }

        // Take branch
        reg.pc = newpc;
        fullPrefetch <C, POLLIPL> ();

    } else {

        // Fall through to next instruction
        if (core == M68000) SYNC(2);
        if constexpr (S == Word) readExt<C>();
        prefetch <C,POLLIPL> ();
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBitDxEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    switch (M) {

        case 0:
        {
            u8 b = readD(src) & 0b11111;
            u32 data = readD(dst);
            data = bit <C,I> (data, b);

            prefetch <C,POLLIPL> ();

            [[maybe_unused]] auto cycles = cyclesBit<C,I>(b);
            SYNC(cycles);
            if (I != BTST) writeD(dst, data);

            CYCLES(4 + cycles, 4 + cycles, 2 + cycles);
            break;
        }
        default:
        {
            u8 b = readD(src) & 0b111;

            u32 ea, data;
            if (!readOp<C, M, Byte>(dst, ea, data)) return;

            data = bit <C,I> (data, b);

            if (I == BCLR && core == M68010) SYNC(2);

            prefetch <C,POLLIPL> ();
            if (I != BTST) writeM <C,M,Byte> (ea, data);

            CYCLES_AI   ( 8,  8,  6,     0,  0,  0,     0,  0,  0)
            CYCLES_PD   (10, 10,  9,     0,  0,  0,     0,  0,  0)
            CYCLES_PI   ( 8,  8,  8,     0,  0,  0,     0,  0,  0)
            CYCLES_DI   (12, 12,  9,     0,  0,  0,     0,  0,  0)
            CYCLES_IX   (14, 14, 11,     0,  0,  0,     0,  0,  0)
            CYCLES_AW   (12, 12,  8,     0,  0,  0,     0,  0,  0)
            CYCLES_AL   (16, 16,  8,     0,  0,  0,     0,  0,  0)
            CYCLES_DIPC (12, 12,  9,     0,  0,  0,     0,  0,  0)
            CYCLES_IXPC (14, 14, 11,     0,  0,  0,     0,  0,  0)
            CYCLES_IM   ( 8,  8,  6,     0,  0,  0,     0,  0,  0)
        }
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBitImEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u8  src = (u8)readI <C,S> ();
    int dst = _____________xxx(opcode);

    switch (M)
    {
        case 0:
        {
            src &= 0b11111;
            u32 data = readD(dst);
            data = bit<C, I>(data, src);

            prefetch <C,POLLIPL> ();

            [[maybe_unused]] auto cycles = cyclesBit <C,I> (src);
            SYNC(cycles);
            if (I != BTST) writeD(dst, data);
            break;
        }
        default:
        {
            src &= 0b111;
            u32 ea, data;
            if (!readOp<C, M, S>(dst, ea, data)) return;

            data = bit<C, I>(data, src);

            prefetch <C,POLLIPL> ();
            if (I != BTST) writeM <C,M,S> (ea, data);
        }
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBitField(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBkpt(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    if (!MIMIC_MUSASHI) SYNC(4);

    signalIllegalOpcodeException(opcode);
    execUnimplemented<C>(4);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execBsr(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    i16 offset = S == Word ? (i16)queue.irc : (i8)opcode;

    u32 newpc = U32_ADD(reg.pc, offset);
    u32 retpc = U32_ADD(reg.pc, S == Word ? 2 : 0);

    // Check for address error
    if (misaligned<Word>(newpc)) {
        execAddressError(makeFrame(newpc));
        return;
    }

    // Save return address on stack
    SYNC(2);
    bool error;
    push <C,Long> (retpc, error);
    if (error) return;

    // Jump to new address
    auto oldpc = reg.pc;
    reg.pc = newpc;

    fullPrefetch <C, POLLIPL> ();

    signalJsrBsrInstr(opcode, oldpc, reg.pc);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCallm(u16 opcode)
{
    // TODO
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCas(u16 opcode)
{
    // TODO
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCas2(u16 opcode)
{
    // TODO
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execChk(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    [[maybe_unused]] i64 c = clock;
    u32 ea, data, dy;
    if (!readOp<C, M, S, STD_AE_FRAME>(src, ea, data)) return;
    dy = readD<S>(dst);

    SYNC_68000(6);
    SYNC_68010(4);

    reg.sr.z = ZERO<S>(dy);
    reg.sr.v = 0;
    reg.sr.c = 0;
    reg.sr.n = MIMIC_MUSASHI ? reg.sr.n : 0;

    if ((i16)dy > (i16)data) {

        SYNC(MIMIC_MUSASHI ? 10 - (int)(clock - c) : 2);
        reg.sr.n = NBIT<S>(dy);
        execTrapException(6);
        return;
    }

    if ((i16)dy < 0) {

        SYNC(MIMIC_MUSASHI ? 10 - (int)(clock - c) : 4);
        reg.sr.n = MIMIC_MUSASHI ? NBIT<S>(dy) : 1;
        execTrapException(6);
        return;
    }

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execChk2(u16 opcode)
{
    // TODO
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execClr(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    if constexpr (C == M68000) {

        int dst = _____________xxx(opcode);

        u32 ea, data;
        if (!readOp<C, M, S, STD_AE_FRAME>(dst, ea, data)) return;

        prefetch <C,POLLIPL> ();

        if constexpr (S == Long && isRegMode(M)) SYNC(2);

        if constexpr (MIMIC_MUSASHI) {
            writeOp <C, M, S> (dst, ea, 0);
        } else {
            writeOp <C, M, S, REVERSE> (dst, ea, 0);
        }

        reg.sr.n = 0;
        reg.sr.z = 1;
        reg.sr.v = 0;
        reg.sr.c = 0;
    }

    if constexpr (C == M68010) {

        int dst = _____________xxx(opcode);

        if constexpr (S == Long && isRegMode(M)) SYNC(2);
        if constexpr (S == Long && isIdxMode(M)) SYNC(2);

        if constexpr (MIMIC_MUSASHI) {
            writeOp <C, M, S> (dst, 0);
        } else {
            writeOp <C, M, S, REVERSE> (dst, 0);
        }

        looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();

        reg.sr.n = 0;
        reg.sr.z = 1;
        reg.sr.v = 0;
        reg.sr.c = 0;
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmp(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    if (!readOp<C, M, S, STD_AE_FRAME>(src, ea, data)) return;

    cmp<C, S>(data, readD<S>(dst));
    prefetch <C,POLLIPL> ();

    if constexpr (S == Long) SYNC(2);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmpa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    if (!readOp<C, M, S, STD_AE_FRAME>(src, ea, data)) return;

    data = SEXT<S>(data);
    cmp<C, Long>(data, readA(dst));
    looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();

    SYNC(2);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmpiRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 src = readI <C,S> ();
    int dst = _____________xxx(opcode);

    prefetch <C,POLLIPL> ();

    if constexpr (S == Long && C == M68000) SYNC(2);
    cmp<C, S>(src, readD<S>(dst));
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmpiEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 src = readI <C,S> ();
    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOp<C, M, S, STD_AE_FRAME>(dst, ea, data)) return;
    prefetch <C,POLLIPL> ();

    cmp<C, S>(src, data);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCmpm(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea1, ea2, data1, data2;

    if (!readOp<C, M, S, AE_INC_PC>(src, ea1, data1)) return;
    pollIpl();
    if (!readOp<C, M, S, AE_INC_PC>(dst, ea2, data2)) return;

    cmp<C, S>(data1, data2);
    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpBcc(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpDbcc(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpGen(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpRestore(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpSave(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpScc(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execCpTrapcc(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDbcc(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    auto exec68000 = [&]() {

        SYNC(2);
        if (!cond(I)) {

            int dn = _____________xxx(opcode);
            u32 newpc = U32_ADD(reg.pc, (i16)queue.irc);

            bool takeBranch = readD<Word>(dn) != 0;

            // Check for address error
            if (misaligned<S>(newpc)) {
                execAddressError(makeFrame(newpc, newpc + 2));
                return;
            }

            // Decrement loop counter
            writeD<Word>(dn, U32_SUB(readD<Word>(dn), 1));

            // Branch
            if (takeBranch) {
                reg.pc = newpc;
                fullPrefetch <C, POLLIPL> ();
                return;
            } else {
                (void)readMS <C,MEM_PROG,Word> (reg.pc + 2);
            }
        } else {
            SYNC(2);
        }

        // Fall through to next instruction
        reg.pc += 2;
        fullPrefetch <C, POLLIPL> ();
    };

    auto exec68010 = [&]() {

        SYNC(2);
        if (!cond(I)) {

            int dn = _____________xxx(opcode);
            i16 disp = (i16)queue.irc;

            u32 newpc = U32_ADD(reg.pc, disp);

            bool takeBranch = readD<Word>(dn) != 0;

            // Check for address error
            if (misaligned<S>(newpc)) {
                execAddressError(makeFrame(newpc, newpc + 2));
                return;
            }

            // Decrement loop counter
            writeD<Word>(dn, U32_SUB(readD<Word>(dn), 1));

            // Branch
            if (takeBranch) {

                reg.pc = newpc;
                fullPrefetch <C, POLLIPL> ();

                /*
                if (loop[queue.ird] && disp == -4) {

                    // Enter loop mode
                    flags |= CPU_IS_LOOPING;
                    queue.irc = opcode;
                    // printf("Entering loop mode (IRD: %x IRC: %x)\n", queue.ird, queue.irc);
                }
                */

                if (MIMIC_MUSASHI) SYNC(2);
                return;

            } else {

                if (core == M68010) SYNC(MIMIC_MUSASHI ? 4 : 2);
                (void)readMS <C,MEM_PROG,Word> (reg.pc + 2);
            }

        } else {

            SYNC(2);
        }

        // Fall through to next instruction
        reg.pc += 2;
        fullPrefetch <C, POLLIPL> ();
    };

    auto execLoop = [&]() {

        SYNC(2);
        if (!cond(I)) {

            int dn = _____________xxx(opcode);
            u32 newpc = U32_ADD(reg.pc, -4);

            bool takeBranch = readD<Word>(dn) != 0;

            // Check for address error
            if (misaligned<S>(newpc)) {
                execAddressError(makeFrame(newpc, newpc + 2));
                return;
            }

            // Decrement loop counter
            writeD<Word>(dn, U32_SUB(readD<Word>(dn), 1));

            // Branch
            if (takeBranch) {
                reg.pc = newpc;
                reg.pc0 = reg.pc;
                queue.ird = queue.irc;
                queue.irc = opcode;
                return;
            } else {
                (void)readMS <C,MEM_PROG,Word> (reg.pc + 2);
            }
        } else {
            SYNC(2);
        }

        // Fall through to next instruction
        reg.pc += 2;
        fullPrefetch <C, POLLIPL> ();
        flags &= ~CPU_IS_LOOPING;
        // printf("Exiting loop mode (IRD: %x IRC: %x)\n", queue.ird, queue.irc);
    };

    switch (C) {

        case M68000: exec68000(); return;
        case M68010: looping<I>() ? execLoop() : exec68010(); return;

        default:
            fatalError;
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExgDxDy(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    std::swap(reg.d[src], reg.d[dst]);
    prefetch <C,POLLIPL> ();

    SYNC(2);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExgAxDy(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    std::swap(reg.a[src], reg.d[dst]);

    prefetch <C,POLLIPL> ();
    SYNC(2);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExgAxAy(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    std::swap(reg.a[src], reg.a[dst]);

    prefetch <C,POLLIPL> ();
    SYNC(2);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExt(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int n = _____________xxx(opcode);

    u32 dn = readD(n);
    dn = (S == Long) ? SEXT<Word>(dn) : SEXT<Byte>(dn);

    writeD<S>(n, dn);
    reg.sr.n = NBIT<S>(dn);
    reg.sr.z = ZERO<S>(dn);
    reg.sr.v = 0;
    reg.sr.c = 0;

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execExtb(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execJmp(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 oldpc = reg.pc;

    int src = _____________xxx(opcode);
    u32 ea  = computeEA <C, M, Long, SKIP_LAST_READ> (src);

    [[maybe_unused]] const int delay[] = { 0,0,0,0,0,2,4,2,0,2,4,0 };
    SYNC(delay[M]);

    // Check for address error
    if (misaligned<Word>(ea)) {
        execAddressError(makeFrame(ea, oldpc));
        return;
    }

    // Jump to new address
    reg.pc = ea;

    // Fill the prefetch queue
    fullPrefetch <C, POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execJsr(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    u32 ea  = computeEA <C, M, Long, SKIP_LAST_READ> (src);

    [[maybe_unused]] const int delay[] = { 0,0,0,0,0,2,4,2,0,2,4,0 };
    SYNC(delay[M]);

    // Check for address error in displacement modes
    if (isDspMode(M) && misaligned<Word>(ea)) {
        execAddressError(makeFrame(ea));
        return;
    }

    // Update program counter
    if (isAbsMode(M) || isDspMode(M)) reg.pc += 2;

    // Check for address error in all other modes
    if (misaligned<Word>(ea)) {
        execAddressError(makeFrame(ea));
        return;
    }

    // Save return address on stack
    bool error;
    push <C,Long> (reg.pc, error);
    if (error) return;

    // Jump to new address
    auto oldpc = reg.pc;
    reg.pc = ea;

    queue.irc = (u16)readMS <C,MEM_PROG,Word> (ea);
    prefetch <C,POLLIPL> ();

    signalJsrBsrInstr(opcode, oldpc, reg.pc);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execLea(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    reg.a[dst] = computeEA <C,M,S> (src);
    if (isIdxMode(M)) SYNC(2);

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execLink(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u16 ird  = getIRD();
    u32 sp   = getSP() - 4;

    int ax   = _____________xxx(opcode);
    i16 disp = (i16)readI <C,S> ();

    // Check for address error
    if (misaligned<Long>(sp)) {
        writeA(ax, sp);
        execAddressError(makeFrame<AE_DATA|AE_WRITE>(sp, getPC() + 2, getSR(), ird));
        return;
    }

    pollIpl();

    // Write to stack
    push <C,Long> (readA(ax) - ((MIMIC_MUSASHI && ax == 7) ? 4 : 0));

    // Modify address register and stack pointer
    writeA(ax, sp);
    reg.sp = U32_ADD(reg.sp, disp);

    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove0(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    reg.sr.n = NBIT<S>(data);
    reg.sr.z = ZERO<S>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;

    if (!writeOp <C, MODE_DN, S> (dst, data)) return;

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove2(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    if constexpr (S == Long && !isMemMode(M)) {

        if (!writeOp <C, MODE_AI, S, AE_INC_PC|POLLIPL> (dst, data)) return;

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        looping<I>() ? noPrefetch() : prefetch <C> ();

    } else {

        reg.sr.n = NBIT<Word>(data);
        reg.sr.z = ZERO<Word>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        if (!writeOp <C, MODE_AI, S, AE_INC_PC> (dst, data)) return;

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);

        looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove3(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    if constexpr (S == Long && !isMemMode(M)) {

        if (!writeOp <C, MODE_PI, S, AE_INC_PC|POLLIPL> (dst, data)) return;

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        looping<I>() ? noPrefetch() : prefetch <C> ();

    } else {

        reg.sr.n = NBIT<Word>(data);
        reg.sr.z = ZERO<Word>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        if (!writeOp <C, MODE_PI, S, AE_INC_PC|POLLIPL> (dst, data)) return;

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);

        looping<I>() ? noPrefetch() : prefetch <C> ();
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove4(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u16 ird = getIRD();
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
    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    // Determine next address error stack frame format
    const u64 flags0 = AE_WRITE | AE_DATA;
    const u64 flags1 = AE_WRITE | AE_DATA | AE_SET_CB3;
    const u64 flags2 = AE_WRITE | AE_DATA;
    int format = (S == Long) ? 0 : reg.sr.c ? 1 : 2;

    reg.sr.n = NBIT<S>(data);
    reg.sr.z = ZERO<S>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;

    looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();

    ea = computeEA <C, MODE_PD, S, IMPLICIT_DECR> (dst);

    // Check for address error
    if (misaligned<S>(ea)) {
        if (format == 0) execAddressError(makeFrame<flags0>(ea + 2, reg.pc + 2, getSR(), ird));
        if (format == 1) execAddressError(makeFrame<flags1>(ea, reg.pc + 2), 2);
        if (format == 2) execAddressError(makeFrame<flags2>(ea, reg.pc + 2), 2);
        if constexpr (S != Long) updateAn <MODE_PD, S> (dst);
        return;
    }

    if (core == M68010 && S == Long) SYNC(2);

    writeM <C, MODE_PD, S, REVERSE> (ea, data);
    updateAn <MODE_PD, S> (dst);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove5(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    if constexpr (S == Long && !isMemMode(M)) {

        reg.sr.n = NBIT<Word>(data >> 16);
        reg.sr.z = ZERO<Word>(data >> 16) && reg.sr.z;

        if (!writeOp <C, MODE_DI, S, POLLIPL> (dst, data)) return;

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        prefetch <C> ();

    } else {

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        if (!writeOp <C, MODE_DI, S> (dst, data)) return;

        prefetch <C,POLLIPL> ();
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove6(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    if constexpr (S == Long && !isMemMode(M)) {

        reg.sr.n = NBIT<Word>(data >> 16);
        reg.sr.z = ZERO<Word>(data >> 16) && reg.sr.z;

        if (!writeOp <C, MODE_IX, S, POLLIPL> (dst, data)) return;

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        prefetch <C> ();

    } else {

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        if (!writeOp <C, MODE_IX, S> (dst, data)) return;

        prefetch <C,POLLIPL> ();
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove7(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    reg.sr.n = NBIT<S>(data);
    reg.sr.z = ZERO<S>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;

    if (!writeOp <C, MODE_AW, S> (dst, data)) return;

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMove8(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

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

        if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

        reg.sr.n = NBIT<Word>(data);
        reg.sr.z = ZERO<Word>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        u32 ea2 = queue.irc << 16;
        readExt<C>();
        ea2 |= queue.irc;

        if (misaligned<S>(ea2)) {
            execAddressError(makeFrame<AE_WRITE|AE_DATA>(ea2));
            return;
        }

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        writeM <C,MODE_AL,S> (ea2, data);
        readExt<C>();

    } else {

        if (!readOp <C, M, S> (src, ea, data)) return;

        reg.sr.n = NBIT<S>(data);
        reg.sr.z = ZERO<S>(data);
        reg.sr.v = 0;
        reg.sr.c = 0;

        if (!writeOp <C, MODE_AL, S> (dst, data)) return;
    }

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovea(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, data;
    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    prefetch <C,POLLIPL> ();
    writeA(dst, SEXT<S>(data));
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovecRcRx(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)
    SUPERVISOR_MODE_ONLY

    switch(queue.irc & 0x0FFF) {

        case 0x000:
        case 0x001:
        case 0x800:
        case 0x801: break;

        default:
            execIllegal <M68010,I,M,S> (opcode);
            return;
    }

    SYNC(4);

    auto arg = readI <C,Word> ();
    int dst = xxxx____________(arg);

    switch(arg & 0x0FFF) {

        case 0x000: reg.r[dst] = reg.sfc & 0b111; break;
        case 0x001: reg.r[dst] = reg.dfc & 0b111; break;
        case 0x800: reg.r[dst] = reg.usp; break;
        case 0x801: reg.r[dst] = reg.vbr; break;
    }

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovecRxRc(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)
    SUPERVISOR_MODE_ONLY

    switch(queue.irc & 0xFFF) {

        case 0x000:
        case 0x001:
        case 0x800:
        case 0x801: break;

        default:
            execIllegal <M68010,I,M,S> (opcode);
            return;
    }

    SYNC(2);

    auto arg = readI <C,Word> ();
    int src = xxxx____________(arg);

    switch(arg & 0x0FFF) {

        case 0x000: setSFC(reg.r[src]); break;
        case 0x001: setDFC(reg.r[src]); break;
        case 0x800: reg.usp = reg.r[src]; break;
        case 0x801: setVBR(reg.r[src]); break;
    }

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovemEaRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src  = _____________xxx(opcode);
    u16 mask = (u16)readI <C,Word> ();
    u32 ea   = computeEA <C,M,S> (src);

    // Check for address error
    if (misaligned<S>(ea)) {
        setFC<M>();
        if constexpr (M == MODE_IX || M == MODE_IXPC) {
            execAddressError(makeFrame <AE_DEC_PC> (ea));
        } else {
            execAddressError(makeFrame <AE_INC_PC> (ea));
        }
        return;
    }

    if constexpr (S == Long) (void)readMS <C,MEM_DATA,Word> (ea);

    switch (M) {

        case 3: // (An)+
        {
            for(int i = 0; i <= 15; i++) {

                if (mask & (1 << i)) {
                    writeR(i, SEXT<S>(readM <C,M,S> (ea)));
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
                    writeR(i, SEXT<S>(readM <C,M,S> (ea)));
                    ea += S;
                }
            }
            break;
        }
    }
    if constexpr (S == Word) (void)readMS <C,MEM_DATA,Word> (ea);

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovemRgEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int dst  = _____________xxx(opcode);
    u16 mask = (u16)readI <C,Word> ();

    switch (M) {

        case 4: // -(An)
        {
            u32 ea = readA(dst);

            // Check for address error
            if (mask && misaligned<S>(ea)) {
                setFC<M>();
                execAddressError(makeFrame <AE_INC_PC|AE_WRITE> (ea - S));
                return;
            }

            for(int i = 15; i >= 0; i--) {

                if (mask & (0x8000 >> i)) {
                    ea -= S;
                    writeM <C, M, S, MIMIC_MUSASHI ? REVERSE : 0> (ea, reg.r[i]);
                }
            }
            writeA(dst, ea);
            break;
        }
        default:
        {
            u32 ea = computeEA <C,M,S> (dst);

            // Check for address error
            if (mask && misaligned<S>(ea)) {
                setFC<M>();
                execAddressError(makeFrame <AE_INC_PC|AE_WRITE> (ea));
                return;
            }

            for(int i = 0; i < 16; i++) {

                if (mask & (1 << i)) {
                    writeM <C,M,S> (ea, reg.r[i]);
                    ea += S;
                }
            }
            break;
        }
    }
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovepDxEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = ____xxx_________(opcode);
    int dst = _____________xxx(opcode);

    u32 ea = computeEA <C,M,S> (dst);
    u32 dx = readD(src);

    switch (S) {

        case Long:
        {
            writeM <C, M, Byte> (ea, (dx >> 24) & 0xFF); ea += 2;
            writeM <C, M, Byte> (ea, (dx >> 16) & 0xFF); ea += 2;
            [[fallthrough]];
        }
        case Word:
        {
            writeM <C, M, Byte> (ea, (dx >>  8) & 0xFF); ea += 2;
            writeM <C, M, Byte> (ea, (dx >>  0) & 0xFF);
        }
    }
    prefetch <C,POLLIPL> ();

    CYCLES_DI (16, 16, 11,      16, 16, 11,     24, 24, 17)
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMovepEaDx(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea = computeEA <C,M,S> (src);
    u32 dx = 0;

    switch (S) {

        case Long:
        {
            dx |= readMS <C,MEM_DATA,Byte> (ea) << 24; ea += 2;
            dx |= readMS <C,MEM_DATA,Byte> (ea) << 16; ea += 2;
            [[fallthrough]];
        }
        case Word:
        {
            dx |= readMS <C,MEM_DATA,Byte> (ea) << 8; ea += 2;
            pollIpl();
            dx |= readMS <C,MEM_DATA,Byte> (ea) << 0;
        }

    }
    writeD <S> (dst, dx);
    prefetch <C> ();

    CYCLES_DI (16, 16, 11,      16, 16, 11,     24, 24, 17)
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveq(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    i8  src = (i8)(opcode & 0xFF);
    int dst = ____xxx_________(opcode);

    writeD <Long> (dst, (i32)src);

    reg.sr.n = NBIT<Byte>(src);
    reg.sr.z = ZERO<Byte>(src);
    reg.sr.v = 0;
    reg.sr.c = 0;

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoves(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)
    SUPERVISOR_MODE_ONLY

    u32 ea, data;

    if (queue.irc & 0x800) {    // Rg -> Ea

        auto arg = readI <C,Word> ();
        int src = xxxx____________(arg);
        int dst = _____________xxx(opcode);

        // Make the DFC register visible on the FC pins
        fcSource = FC_FROM_DFC;

        auto value = readR<S>(src);

        // Take care of some special cases
        if (M == MODE_PI && src == (dst | 0x8)) {

            // MOVES An,(An)+
            value += dst == 7 ? (S == Long ? 4 : 2) : S;
        }
        if (M == MODE_PD && src == (dst | 0x8)) {

            // MOVES An,-(An)
            value -= dst == 7 ? (S == Long ? 4 : 2) : S;
        }

        writeOp <C, M, S> (dst, value);

        // Switch back to the old FC pin values
        fcSource = FC_FROM_FCL;

        switch(M) {

            case MODE_AI: SYNC(6); break;
            case MODE_PD: SYNC(S == Long ? 10 : 6); break;
            case MODE_IX: SYNC(S == Long ? 14 : 12); break;
            case MODE_PI: SYNC(6); break;
            case MODE_DI: SYNC(S == Long ? 12 : 10); break;
            case MODE_AW: SYNC(S == Long ? 12 : 10); break;
            case MODE_AL: SYNC(S == Long ? 12 : 10); break;

            default:
                fatalError;
        }

    } else {                    // Ea -> Rg

        auto arg = readI <C,Word> ();
        int src = _____________xxx(opcode);
        int dst = xxxx____________(arg);

        if (!readOp<C, M, S, STD_AE_FRAME>(src, ea, data)) return;

        // Make the SFC register visible on the FC pins
        fcSource = FC_FROM_SFC;

        if (dst < 8) {
            writeR<S>(dst, data);
        } else {
            writeR<Long>(dst, SEXT<S>(data));
        }

        // Switch back to the old FC pin values
        fcSource = FC_FROM_FCL;

        switch(M) {

            case MODE_AI: SYNC(6); break;
            case MODE_PD: SYNC(S == Long ? 10 : 6); break;
            case MODE_IX: SYNC(S == Long ? 14 : 12); break;
            case MODE_PI: SYNC(6); break;
            case MODE_DI: SYNC(S == Long ? 12 : 10); break;
            case MODE_AW: SYNC(S == Long ? 12 : 10); break;
            case MODE_AL: SYNC(S == Long ? 12 : 10); break;

            default:
                fatalError;
        }
    }

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveFromCcrRg(u16 opcode)
{
    assert(core >= M68010);
    EXEC_DEBUG(C,I,M,S)

    int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOp <C, M, S> (dst, ea, data)) return;
    prefetch <C,POLLIPL> ();

    writeD <S> (dst, getCCR());
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveFromCcrEa(u16 opcode)
{
    assert(core >= M68010);
    EXEC_DEBUG(C,I,M,S)

    int dst = _____________xxx(opcode);
    u32 ea, data;

    if (!readOp <C, M, S, STD_AE_FRAME> (dst, ea, data)) return;
    prefetch <C> ();

    writeOp <C, M, S, POLLIPL> (dst, ea, getCCR());
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveToCcr(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    u32 ea, data;

    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    SYNC(4);
    setCCR((u8)data);

    (void)readMS <C,MEM_PROG,Word> (reg.pc + 2);
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveFromSrRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)
    if constexpr (C == M68010) SUPERVISOR_MODE_ONLY

        int dst = _____________xxx(opcode);

    u32 ea, data;
    if (!readOp <C, M, S> (dst, ea, data)) return;
    prefetch <C,POLLIPL> ();

    if constexpr (C == M68000) SYNC(2);
    writeD <S> (dst, getSR());
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveFromSrEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)
    if constexpr (C == M68010) SUPERVISOR_MODE_ONLY

        int dst = _____________xxx(opcode);
    u32 ea, data;

    if (!readOp <C, M, S, STD_AE_FRAME> (dst, ea, data)) return;
    prefetch <C> ();

    writeOp <C, M, S, POLLIPL> (dst, ea, getSR());
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveToSr(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    SUPERVISOR_MODE_ONLY

    int src = _____________xxx(opcode);

    u32 ea, data;
    if (!readOp <C, M, S, STD_AE_FRAME> (src, ea, data)) return;

    SYNC(4);
    setSR((u16)data);

    (void)readMS <C,MEM_PROG,Word> (reg.pc + 2);
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveUspAn(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)
    SUPERVISOR_MODE_ONLY

    int an = _____________xxx(opcode);

    if (core >= M68010) SYNC(2);

    prefetch <C,POLLIPL> ();
    writeA(an, getUSP());
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMoveAnUsp(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)
    SUPERVISOR_MODE_ONLY

    int an = _____________xxx(opcode);

    if (core >= M68010) SYNC(2);

    prefetch <C,POLLIPL> ();
    setUSP(readA(an));
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMul(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    if constexpr (MIMIC_MUSASHI) {
        execMulMusashi<C, I, M, S>(opcode);
        return;
    }

    u32 ea, data, result;

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    if (!readOp <C, M, Word, STD_AE_FRAME> (src, ea, data)) return;

    prefetch <C,POLLIPL> ();
    result = mul <C,I> (data, readD<Word>(dst));

    [[maybe_unused]] auto cycles = cyclesMul <C,I> (u16(data));
    SYNC(cycles);

    writeD(dst, result);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMull(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execMulMusashi(u16 op)
{
    EXEC_DEBUG(C,I,M,S)

    u32 ea, data, result;

    int src = _____________xxx(op);
    int dst = ____xxx_________(op);

    if (!readOp <C, M, Word> (src, ea, data)) return;

    prefetch <C,POLLIPL> ();
    result = mulMusashi <C, I> (data, readD<Word>(dst));

    if constexpr (I == MULU) { SYNC_68000(50); SYNC_68010(26); }
    if constexpr (I == MULS) { SYNC_68000(50); SYNC_68010(28); }

    writeD(dst, result);
}


template <Core C, Instr I, Mode M, Size S> void
Moira::execDiv(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    if constexpr (MIMIC_MUSASHI) {
        execDivMusashi <C, I, M, S> (opcode);
        return;
    }

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    u32 ea, divisor, result;
    if (!readOp <C, M, Word, STD_AE_FRAME> (src, ea, divisor)) return;
    u32 dividend = readD(dst);

    // Check for division by zero
    if (divisor == 0) {

        if (I == DIVU) {
            reg.sr.n = NBIT<Long>(dividend);
            reg.sr.z = (dividend & 0xFFFF0000) == 0;
            reg.sr.v = 0;
            reg.sr.c = 0;
        } else {
            reg.sr.n = 0;
            reg.sr.z = 1;
            reg.sr.v = 0;
            reg.sr.c = 0;
        }

        SYNC(8);
        execTrapException(5);
        return;
    }

    result = div <C,I> (dividend, divisor);

    [[maybe_unused]] auto cycles = cyclesDiv <C,I> (dividend, (u16)divisor) - 4;
    SYNC(cycles);

    writeD(dst, result);
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDivl(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execDivMusashi(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);
    int dst = ____xxx_________(opcode);

    [[maybe_unused]] i64 c = clock;
    u32 ea, divisor, result;
    if (!readOp <C,M,Word> (src, ea, divisor)) return;

    // Check for division by zero
    if (divisor == 0) {
        if (core == M68000) {
            SYNC(8 - (int)(clock - c));
        } else {
            SYNC(10 - (int)(clock - c));
        }
        execTrapException(5);
        return;
    }

    u32 dividend = readD(dst);
    result = divMusashi <C,I> (dividend, divisor);

    if constexpr (I == DIVU) { SYNC_68000(136); SYNC_68010(104); }
    if constexpr (I == DIVS) { SYNC_68000(154); SYNC_68010(118); }

    writeD(dst, result);
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execNbcd(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int reg = _____________xxx(opcode);

    switch (M) {

        case 0: // Dn
        {
            prefetch <C,POLLIPL> ();
            SYNC(2);
            writeD<Byte>(reg, bcd<C, SBCD, Byte>(readD<Byte>(reg), 0));
            break;
        }
        default: // Ea
        {
            u32 ea, data;
            if (!readOp <C, M, Byte> (reg, ea, data)) return;
            prefetch <C,POLLIPL> ();
            writeM <C, M, Byte> (ea, bcd<C, SBCD, Byte>(data, 0));
            break;
        }
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execNegRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if (!readOp<C, M, S>(dst, ea, data)) return;

    data = logic<C, I, S>(data);
    prefetch <C,POLLIPL> ();

    if constexpr (S == Long) SYNC(2);
    writeD<S>(dst, data);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execNegEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if (!readOp<C, M, S, STD_AE_FRAME>(dst, ea, data)) return;

    data = logic<C, I, S>(data);
    looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();

    if constexpr (MIMIC_MUSASHI) {
        writeOp <C, M, S> (dst, ea, data);
    } else {
        writeOp <C, M, S, REVERSE> (dst, ea, data);
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execNop(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPack(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPea(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int src = _____________xxx(opcode);

    u32 ea = computeEA <C,M,Long> (src);

    if (isIdxMode(M)) SYNC(2);

    if (misaligned(reg.sp)) {
        reg.sp -= S;
        if (isAbsMode(M)) {
            execAddressError(makeFrame<AE_WRITE|AE_DATA>(reg.sp));
        } else {
            execAddressError(makeFrame<AE_WRITE|AE_DATA|AE_INC_PC>(reg.sp));
        }
        return;
    }

    if (isAbsMode(M)) {

        push <C,Long> (ea);
        prefetch <C,POLLIPL> ();

    } else if (isIdxMode(M)) {

        pollIpl();
        prefetch <C> ();
        push <C,Long> (ea);

    } else {

        prefetch <C,POLLIPL> ();
        push <C,Long> (ea);
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execReset(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    SUPERVISOR_MODE_ONLY

    signalResetInstr();

    SYNC_68000(128);
    SYNC_68010(126);

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRtd(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    signalRtdInstr();

    bool error;
    u32 newpc = readM <C,M,Long> (reg.sp, error);
    if (error) return;

    reg.sp += 4 + i16(queue.irc);

    if (misaligned(newpc)) {
        execAddressError(makeFrame<AE_PROG>(newpc, reg.pc));
        return;
    }

    setPC(newpc);
    fullPrefetch <C, POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRte(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)
    SUPERVISOR_MODE_ONLY

    u16 newsr = 0;
    u32 newpc = 0;

    if constexpr (C == M68000) {

        newsr = (u16)readMS <C,MEM_DATA,Word> (reg.sp);
        reg.sp += 2;

        newpc = readMS <C,MEM_DATA,Long> (reg.sp);
        reg.sp += 4;
    }

    if constexpr (C == M68010) {

        newsr = (u16)readMS <C,MEM_DATA,Word> (reg.sp);
        reg.sp += 2;

        newpc = readMS <C,MEM_DATA,Long> (reg.sp);
        reg.sp += 4;

        u16 format = (u16)(readMS <C,MEM_DATA,Word> (reg.sp) >> 12);
        reg.sp += 2;

        // Check for format errors
        if (format != 0) {

            reg.sp -= 8;
            execFormatError();
            return;
        }
    }

    setSR(newsr);

    if (misaligned(newpc)) {
        execAddressError(makeFrame<AE_PROG>(newpc, reg.pc));
        return;
    }

    setPC(newpc);
    fullPrefetch <C, POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRtm(u16 opcode)
{
    fullPrefetch <C, POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRtr(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    bool error;
    u16 newccr = (u16)readM <C, M, Word> (reg.sp, error);
    if (error) return;

    reg.sp += 2;

    u32 newpc = readMS <C,MEM_DATA,Long> (reg.sp);
    reg.sp += 4;

    setCCR((u8)newccr);

    if (misaligned(newpc)) {
        execAddressError(makeFrame<AE_PROG>(newpc, reg.pc));
        return;
    }

    setPC(newpc);

    fullPrefetch <C, POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execRts(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    signalRtsInstr();

    bool error;
    u32 newpc = readM <C, M, Long> (reg.sp, error);
    if (error) return;

    reg.sp += 4;

    if (misaligned(newpc)) {
        execAddressError(makeFrame<AE_PROG>(newpc, reg.pc));
        return;
    }

    setPC(newpc);
    fullPrefetch <C, POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execSccRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if (!readOp<C, M, Byte>(dst, ea, data)) return;

    data = cond(I) ? 0xFF : 0;
    prefetch <C,POLLIPL> ();

    if (core >= M68010) {
        // if (data) SYNC(2);
    } else {
        if (data) SYNC(2);
    }

    writeD<Byte>(dst, data);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execSccEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int dst = ( _____________xxx(opcode) );
    u32 ea, data;

    if (!readOp<C, M, Byte>(dst, ea, data)) return;

    data = cond(I) ? 0xFF : 0;
    prefetch <C,POLLIPL> ();

    writeOp <C, M, Byte> (dst, ea, data);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execStop(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    SUPERVISOR_MODE_ONLY

    u16 src = (u16)readI <C,Word> ();

    setSR(src);
    flags |= CPU_IS_STOPPED;
    reg.pc0 = reg.pc;

    signalStopInstr(src);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execSwap(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int rg  = ( _____________xxx(opcode) );
    u32 dat = readD(rg);

    prefetch <C,POLLIPL> ();

    dat = (dat >> 16) | (dat & 0xFFFF) << 16;
    writeD(rg, dat);

    reg.sr.n = NBIT<Long>(dat);
    reg.sr.z = ZERO<Long>(dat);
    reg.sr.v = 0;
    reg.sr.c = 0;
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTasRg(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    signalTasInstr();

    int dst = ( _____________xxx(opcode) );

    u32 ea, data;
    readOp<C, M, Byte>(dst, ea, data);

    reg.sr.n = NBIT<Byte>(data);
    reg.sr.z = ZERO<Byte>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;

    data |= 0x80;
    writeD<S>(dst, data);

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTasEa(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    signalTasInstr();

    int dst = ( _____________xxx(opcode) );

    u32 ea, data;
    readOp<C, M, Byte>(dst, ea, data);

    reg.sr.n = NBIT<Byte>(data);
    reg.sr.z = ZERO<Byte>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;
    data |= 0x80;

    if (!isRegMode(M)) SYNC(2);
    writeOp <C, M, S> (dst, ea, data);

    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTrap(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int nr = ____________xxxx(opcode);

    SYNC(4);
    execTrapException(32 + nr);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTrapv(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    if (reg.sr.v) {

        (void)readMS <C,MEM_PROG,Word> (reg.pc + 2);
        execTrapException(7);

    } else {

        prefetch <C,POLLIPL> ();
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTrapcc(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    // TODO
    prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execTst(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int rg = _____________xxx(opcode);

    u32 ea, data;
    if (!readOp<C, M, S, STD_AE_FRAME>(rg, ea, data)) return;

    reg.sr.n = NBIT<S>(data);
    reg.sr.z = ZERO<S>(data);
    reg.sr.v = 0;
    reg.sr.c = 0;

    looping<I>() ? noPrefetch() : prefetch <C,POLLIPL> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execUnlk(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    int an = _____________xxx(opcode);

    // Move address register to stack pointer
    if (misaligned(readA(an))) {
        execAddressError(makeFrame<AE_DATA|AE_INC_PC>(readA(an)));
        return;
    }
    reg.sp = readA(an);

    // Update address register
    u32 ea, data;
    if (!readOp<C, MODE_AI, Long, AE_DATA|AE_INC_PC|POLLIPL>(7, ea, data)) return;
    writeA(an, data);

    if (an != 7) reg.sp += 4;
    prefetch <C> ();
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execUnpk(u16 opcode)
{
    EXEC_DEBUG(C,I,M,S)

    prefetch <C> ();
}
