// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template <Core C, Mode M, Size S, Flags F> bool
Moira::readOp(int n, u32 &ea, u32 &result)
{
    switch (M) {
            
        // Handle non-memory modes
        case MODE_DN: result = readD <S> (n); return true;
        case MODE_AN: result = readA <S> (n); return true;
        case MODE_IM: result = readI <C,S> ();  return true;
            
        default:
            
            // Compute effective address
            ea = computeEA <C,M,S,F> (n);

            // Read from effective address
            bool error; result = readM <C,M,S,F> (ea, error);

            // Emulate -(An) register modification
            updateAnPD <M,S> (n);

            // Exit if an address error has occurred
            if (error) return false;

            // Emulate (An)+ register modification
            updateAnPI <M,S> (n);
            
            return !error;
    }
}

template <Core C, Mode M, Size S, Flags F> bool
Moira::writeOp(int n, u32 val)
{
    switch (M) {
            
        // Handle non-memory modes
        case MODE_DN: writeD <S> (n, val); return true;
        case MODE_AN: writeA <S> (n, val); return true;
        case MODE_IM: fatalError;

        default:
            
            // Compute effective address
            u32 ea = computeEA <C,M,S> (n);
            
            // Write to effective address
            bool error; writeM <C,M,S,F> (ea, val, error);
            
            // Emulate -(An) register modification
            updateAnPD <M,S> (n);
            
            // Early exit in case of an address error
            if (error) return false;
            
            // Emulate (An)+ register modification
            updateAnPI <M,S> (n);
            
            return !error;
    }
}

template <Core C, Mode M, Size S, Flags F> void
Moira::writeOp(int n, u32 ea, u32 val)
{
    switch (M) {
            
        // Handle non-memory modes
        case MODE_DN: writeD <S> (n, val); return;
        case MODE_AN: writeA <S> (n, val); return;
        case MODE_IM: fatalError;

        default:
            writeM <C,M,S,F> (ea, val);
    }
}

template <Core C, Mode M, Size S, Flags F> u32
Moira::computeEA(u32 n) {

    assert(n < 8);

    u32 result;

    switch (M) {

        case 0:  // Dn
        case 1:  // An
        {
            result = n;
            break;
        }
        case 2:  // (An)
        {
            result = readA(n);
            break;
        }
        case 3:  // (An)+
        {
            result = readA(n);
            break;
        }
        case 4:  // -(An)
        {
            if ((F & IMPLICIT_DECR) == 0) SYNC(2);
            result = readA(n) - ((n == 7 && S == Byte) ? 2 : S);
            break;
        }
        case 5: // (d,An)
        {
            u32 an = readA(n);
            i16  d = (i16)queue.irc;
            
            result = U32_ADD(an, d);
            if ((F & SKIP_LAST_READ) == 0) readExt<C>();
            break;
        }
        case 6: // (d,An,Xi)
        {
            if constexpr (C == M68020) {

                if (queue.irc & 0x100) {
                    result = computeEAfe<C,M,S>(readA(n));
                } else {
                    result = computeEAbe<C,M,S>(readA(n));
                }
                break;
            }

            i8   d = (i8)queue.irc;
            u32 an = readA(n);
            u32 xi = readR((queue.irc >> 12) & 0b1111);

            result = U32_ADD3(an, d, ((queue.irc & 0x800) ? xi : SEXT<Word>(xi)));

            SYNC(2);
            if ((F & SKIP_LAST_READ) == 0) readExt<C>();
            break;
        }
        case 7: // ABS.W
        {
            result = (i16)queue.irc;
            if ((F & SKIP_LAST_READ) == 0) readExt<C>();
            break;
        }
        case 8: // ABS.L
        {
            result = queue.irc << 16;
            readExt<C>();
            result |= queue.irc;
            if ((F & SKIP_LAST_READ) == 0) readExt<C>();
            break;
        }
        case 9: // (d,PC)
        {
            i16  d = (i16)queue.irc;

            result = U32_ADD(reg.pc, d);
            if ((F & SKIP_LAST_READ) == 0) readExt<C>();
            break;
        }
        case 10: // (d,PC,Xi)
        {
            if constexpr (C == M68020) {

                if (queue.irc & 0x100) {
                    result = computeEAfe<C,M,S>(reg.pc);
                } else {
                    result = computeEAbe<C,M,S>(reg.pc);
                }
                break;
            }

            i8   d = (i8)queue.irc;
            u32 xi = readR((queue.irc >> 12) & 0b1111);
            
            result = U32_ADD3(reg.pc, d, ((queue.irc & 0x800) ? xi : SEXT<Word>(xi)));
            SYNC(2);
            if ((F & SKIP_LAST_READ) == 0) readExt<C>();
            break;
        }
        case 11: // Im
        {
            result = readI <C,S> ();
            break;
        }
        default:
        {
            fatalError;
        }
    }
    return result;
}

template <Core C, Mode M, Size S, Flags F> u32
Moira::computeEAbe(u32 an)
{
    i32 result;

    printf("Moira: computeEAbe\n");

    i8   d = (i8)queue.irc;
    u32 xi = readR((queue.irc >> 12) & 0b1111);
    int scale = (queue.irc >> 9) & 0b11;
    u32 offset = (queue.irc & 0x800) ? xi : SEXT<Word>(xi);
    offset = u32(offset << scale);
    result = U32_ADD3(an, d, offset);

    SYNC(2);
    if ((F & SKIP_LAST_READ) == 0) readExt<C>();

    return result;
}

template <Core C, Mode M, Size S, Flags F> u32
Moira::computeEAfe(u32 an)
{
    printf("Moira: computeEAfe\n");

    u32 xn = 0;                        /* Index register */
    u32 bd = 0;                        /* Base Displacement */
    u32 od = 0;

    u16 extension = queue.irc;
    readExt<C>();

    /* Check if base register is present */
    if(extension & 0x80) {                /* BS */
        printf("Moira: (1)\n");
        an = 0;                           /* An */
    }

    /* Check if index is present */
    if(!(extension & 0x40))
    {
        xn = readR(extension>>12);     /* Xn */
        printf("Moira: (2) xn = %x\n", xn);
        if(!(extension & 0x800)) {     /* W/L */
            xn = SEXT<Word>(xn);
            printf("Moira: (3) xn = %x\n", xn);
        }
        xn <<= (extension>>9) & 3;      /* SCALE */
        printf("Moira: Scaled: xn = %x\n", xn);
    }

    /* Check if base displacement is present */
    if (extension & 0x20) {       /* BD SIZE */
        printf("Moira: (4)\n");
        if (extension & 0x10) {
            printf("Moira: (4.1)\n");
            bd = (queue.irc << 16);
            readExt<C>();
            bd |= queue.irc;
            readExt<C>();
            printf("bd = %x\n", bd);
        } else {
            printf("Moira: (4.2)\n");
            bd = queue.irc;
            readExt<C>();
            printf("bd = %x\n", bd);

        }
    }

    /* If no indirect action, we are done */
    if(!(extension & 7)) {                  /* No Memory Indirect */
        printf("Moira: (5)\n");
        return an + bd + xn;
    }

    /* Check if outer displacement is present */
    if(extension & 0x2) {
        printf("Moira: (6)\n");
        if (extension & 0x1) {
            printf("Moira: (6.1)\n");
            od = (queue.irc << 16);
            readExt<C>();
            od |= queue.irc;
            readExt<C>();
            printf("od = %x\n", od);
        } else {
            printf("Moira: (6.2)\n");
            od = queue.irc;
            readExt<C>();
            printf("od = %x\n", od);
        }
    }

    /* Postindex */
    if (extension & 0x4) {   /* I/IS:  0 = preindex, 1 = postindex */
        u32 result = readM<C,M,Long>(an + bd) + xn + od;
        printf("Moira: (7) result = %x\n", result);
        return result;
    }

    /* Preindex */
    printf("Moira: (8)\n");
    return readM<C,M,S>(an + bd+ xn) + od;
}

template <Mode M, Size S> void
Moira::updateAnPD(int n)
{
    // -(An)
    if constexpr (M == 4) reg.a[n] -= (n == 7 && S == Byte) ? 2 : S;
}

template <Mode M, Size S> void
Moira::undoAnPD(int n)
{
    // -(An)
    if constexpr (M == 4) reg.a[n] += (n == 7 && S == Byte) ? 2 : S;
}

template <Mode M, Size S> void
Moira::updateAnPI(int n)
{
    // (An)+
    if constexpr (M == 3) reg.a[n] += (n == 7 && S == Byte) ? 2 : S;

}

template <Mode M, Size S> void
Moira::updateAn(int n)
{
    // (An)+
    if constexpr (M == 3) reg.a[n] += (n == 7 && S == Byte) ? 2 : S;

    // -(An)
    if constexpr (M == 4) reg.a[n] -= (n == 7 && S == Byte) ? 2 : S;
}

template <Core C, Mode M, Size S, Flags F> u32
Moira::readM(u32 addr, bool &error)
{
    if (isPrgMode(M)) {
        return readMS <C,MEM_PROG,S,F> (addr, error);
    } else {
        return readMS <C,MEM_DATA,S,F> (addr, error);
    }
}

template <Core C, Mode M, Size S, Flags F> u32
Moira::readM(u32 addr)
{
    if (isPrgMode(M)) {
        return readMS <C,MEM_PROG,S,F> (addr);
    } else {
        return readMS <C,MEM_DATA,S,F> (addr);
    }
}

template <Core C, MemSpace MS, Size S, Flags F> u32
Moira::readMS(u32 addr, bool &error)
{
    // Check for address errors
    if ((error = misaligned<S>(addr)) == true) {
        
        setFC(MS == MEM_DATA ? FC_USER_DATA : FC_USER_PROG);
        execAddressError(makeFrame<F>(addr), 2);
        return 0;
    }
    
    return readMS <C,MS,S,F> (addr);
}

template <Core C, MemSpace MS, Size S, Flags F> u32
Moira::readMS(u32 addr)
{
    u32 result;
        
    if constexpr (S == Long) {

        // Break down the long word access into two word accesses
        result = readMS <C,MS,Word> (addr) << 16;
        result |= readMS <C,MS,Word,F> (addr + 2);

    } else {
        
        // Update function code pins
        setFC(MS == MEM_DATA ? FC_USER_DATA : FC_USER_PROG);
        
        // Check if a watchpoint is being accessed
        if ((flags & CPU_CHECK_WP) && debugger.watchpointMatches(addr, S)) {
            watchpointReached(addr);
        }
        
        // Perform the read operation
        SYNC(2);
        if (F & POLLIPL) pollIpl();
        result = (S == Byte) ? read8(addr & 0xFFFFFF) : read16(addr & 0xFFFFFF);
        SYNC(2);
    }
    
    return result;
}

template <Core C, Mode M, Size S, Flags F> void
Moira::writeM(u32 addr, u32 val, bool &error)
{
    if (isPrgMode(M)) {
        writeMS <C,MEM_PROG,S,F> (addr, val, error);
    } else {
        writeMS <C,MEM_DATA,S,F> (addr, val, error);
    }
}

template <Core C, Mode M, Size S, Flags F> void
Moira::writeM(u32 addr, u32 val)
{
    if (isPrgMode(M)) {
        writeMS <C,MEM_PROG,S,F> (addr, val);
    } else {
        writeMS <C,MEM_DATA,S,F> (addr, val);
    }
}

template <Core C, MemSpace MS, Size S, Flags F> void
Moira::writeMS(u32 addr, u32 val, bool &error)
{
    // Check for address errors
    if ((error = misaligned<S>(addr)) == true) {
        setFC(MS == MEM_DATA ? FC_USER_DATA : FC_USER_PROG);
        execAddressError(makeFrame <F|AE_WRITE> (addr), 2);
        return;
    }
    
    writeMS <C,MS,S,F> (addr, val);
}

template <Core C, MemSpace MS, Size S, Flags F> void
Moira::writeMS(u32 addr, u32 val)
{
    if constexpr (S == Long) {

        // Break down the long word access into two word accesses
        if (F & REVERSE) {
            writeMS <C,MS,Word>   (addr + 2, val & 0xFFFF);
            writeMS <C,MS,Word,F> (addr,     val >> 16   );
        } else {
            writeMS <C,MS,Word>   (addr,     val >> 16   );
            writeMS <C,MS,Word,F> (addr + 2, val & 0xFFFF);
        }

    } else {
        
        // Update function code pins
        setFC(MS == MEM_DATA ? FC_USER_DATA : FC_USER_PROG);
        
        // Check if a watchpoint is being accessed
        if ((flags & CPU_CHECK_WP) && debugger.watchpointMatches(addr, S)) {
            watchpointReached(addr);
        }
        
        // Perform the write operation
        SYNC(2);
        if (F & POLLIPL) pollIpl();
        S == Byte ? write8(addr & 0xFFFFFF, (u8)val) : write16(addr & 0xFFFFFF, (u16)val);
        SYNC(2);
    }
}

template <Core C, Size S> u32
Moira::readI()
{
    u32 result;

    switch (S) {
            
        case Byte:
            
            result = (u8)queue.irc;
            readExt<C>();
            break;
            
        case Word:
            
            result = queue.irc;
            readExt<C>();
            break;
            
        case Long:
            
            result = queue.irc << 16;
            readExt<C>();
            result |= queue.irc;
            readExt<C>();
            break;
            
        default:
            fatalError;
    }

    return result;
}

template <Core C, Size S, Flags F> void
Moira::push(u32 val)
{
    reg.sp -= S;
    writeMS <C,MEM_DATA,S,F> (reg.sp, val);
}

template <Core C, Size S, Flags F> void
Moira::push(u32 val, bool &error)
{
    reg.sp -= S;
    writeMS <C,MEM_DATA,S,F> (reg.sp, val, error);
}

template <Size S> bool
Moira::misaligned(u32 addr)
{
    return EMULATE_ADDRESS_ERROR ? ((addr & 1) && S != Byte) : false;
}

template <Flags F> AEStackFrame
Moira::makeFrame(u32 addr, u32 pc, u16 sr, u16 ird)
{
    AEStackFrame frame;
    u16 read = 0x10;
    
    // Prepare
    if (F & AE_WRITE) read = 0;
    if (F & AE_PROG) setFC(FC_USER_PROG);
    if (F & AE_DATA) setFC(FC_USER_DATA);

    // Create
    frame.code = (ird & 0xFFE0) | (u16)readFC() | read;
    frame.addr = addr;
    frame.ird = ird;
    frame.sr = sr;
    frame.pc = pc;

    // Adjust
    if (F & AE_INC_PC) frame.pc += 2;
    if (F & AE_DEC_PC) frame.pc -= 2;
    if (F & AE_INC_ADDR) frame.addr += 2;
    if (F & AE_DEC_ADDR) frame.addr -= 2;
    if (F & AE_SET_CB3) frame.code |= (1 << 3);
        
    return frame;
}

template <Flags F> AEStackFrame
Moira::makeFrame(u32 addr, u32 pc)
{
    return makeFrame <F> (addr, pc, getSR(), getIRD());
}

template <Flags F> AEStackFrame
Moira::makeFrame(u32 addr)
{
    return makeFrame <F> (addr, getPC(), getSR(), getIRD());
}

template <Core C, Flags F> void
Moira::prefetch()
{
    /* Whereas pc is a moving target (it moves forward while an instruction is
     * being processed, pc0 stays stable throughout the entire execution of
     * an instruction. It always points to the start address of the currently
     * executed instruction.
     */
    reg.pc0 = reg.pc;

    queue.ird = queue.irc;
    queue.irc = (u16)readMS <C,MEM_PROG,Word,F> (reg.pc + 2);
}

template <Core C, Flags F, int delay> void
Moira::fullPrefetch()
{    
    // Check for address error
    if (misaligned(reg.pc)) {
        execAddressError(makeFrame(reg.pc), 2);
        return;
    }

    queue.irc = (u16)readMS <C,MEM_PROG,Word> (reg.pc);
    if (delay) SYNC(delay);
    prefetch <C,F> ();
}

void
Moira::noPrefetch()
{
    assert(flags & CPU_IS_LOOPING);

    reg.pc0 = reg.pc;
    std::swap(queue.irc, queue.ird);
}

template <Core C> void
Moira::readExt()
{
    reg.pc += 2;
    
    // Check for address error
    if (misaligned<Word>(reg.pc)) {
        execAddressError(makeFrame(reg.pc));
        return;
    }
    
    queue.irc = (u16)readMS <C,MEM_PROG,Word> (reg.pc);
}

template <Core C, Flags F> void
Moira::jumpToVector(int nr)
{
    u32 vectorAddr = reg.vbr + 4 * nr;

    exception = nr;
    
    // Update the program counter
    reg.pc = readMS <C,MEM_DATA,Long> (vectorAddr);
    
    // Check for address error
    if (misaligned(reg.pc)) {
        if (nr != 3) {
            execAddressError(makeFrame <F|AE_PROG> (reg.pc, vectorAddr));
        } else {
            halt(); // Double fault
        }
        return;
    }
    
    // Update the prefetch queue
    queue.irc = (u16)readMS <C,MEM_PROG,Word> (reg.pc);
    SYNC(2);
    prefetch <C,POLLIPL> ();
    
    // Stop emulation if the exception should be catched
    if (debugger.catchpointMatches(nr)) catchpointReached(u8(nr));

    signalJumpToVector(nr, reg.pc);
}

template <Core C, Mode M, Size S> int
Moira::cyclePenalty(u16 ext)
{
    if constexpr (C != M68020) return 0;
    
    const u8 delay[64] =
    {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0, /* ..01.000 no memory indirect, base NULL             */
        5, /* ..01..01 memory indirect,    base NULL, outer NULL */
        7, /* ..01..10 memory indirect,    base NULL, outer 16   */
        7, /* ..01..11 memory indirect,    base NULL, outer 32   */
        0,  5,  7,  7,  0,  5,  7,  7,  0,  5,  7,  7,
        2, /* ..10.000 no memory indirect, base 16               */
        7, /* ..10..01 memory indirect,    base 16,   outer NULL */
        9, /* ..10..10 memory indirect,    base 16,   outer 16   */
        9, /* ..10..11 memory indirect,    base 16,   outer 32   */
        0,  7,  9,  9,  0,  7,  9,  9,  0,  7,  9,  9,
        6, /* ..11.000 no memory indirect, base 32               */
        11, /* ..11..01 memory indirect,    base 32,   outer NULL */
        13, /* ..11..10 memory indirect,    base 32,   outer 16   */
        13, /* ..11..11 memory indirect,    base 32,   outer 32   */
        0, 11, 13, 13,  0, 11, 13, 13,  0, 11, 13, 13
    };

    switch (M) {

        case MODE_IX:
        case MODE_IXPC:

            return delay[ext & 0x3F];

        default:

            return 0;
    }
}
