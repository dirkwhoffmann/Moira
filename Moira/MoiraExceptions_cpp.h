// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

template <Core C> void
Moira::writeStackFrameAEBE(StackFrame &frame)
{
    if constexpr (EXACT_68000) {

        // MAME and UAE write the words in a different order
        U32_DEC(reg.sp, 14);
        write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 12), (u16)frame.pc);
        write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 8), frame.sr);
        write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 10), frame.pc >> 16);
        write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 6), frame.ird);
        write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 4), (u16)frame.addr);
        write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 0), frame.code);
        write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 2), frame.addr >> 16);
        return;
    }

    // Push PC
    push<C, Word>((u16)frame.pc);
    push<C, Word>(frame.pc >> 16);

    // Push SR and IRD
    push<C, Word>(frame.sr);
    push<C, Word>(frame.ird);

    // Push address
    push<C, Word>((u16)frame.addr);
    push<C, Word>(frame.addr >> 16);

    // Push memory access type and function code
    push<C, Word>(frame.code);
}

template <Core C> void
Moira::writeStackFrame0000(u16 sr, u32 pc, u16 nr)
{
    switch (C) {

        case Core::C68000:

            if constexpr ((compat == Compatibility::MUSASHI)) {

                push<C, Long>(pc);
                push<C, Word>(sr);

            } else {

                U32_DEC(reg.sp, 6);
                if constexpr (compat == Compatibility::MAME) {

                    // MAME has not adjusted the stack pointer yet if the first write fails
                    try { write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 4) & (C == Core::C68020 ? ~0u : ~1u), pc & 0xFFFF); }
                    catch (BusError &) { U32_INC(reg.sp, 6); throw; }

                } else {

                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 4) & (C == Core::C68020 ? ~0u : ~1u), pc & 0xFFFF);
                }
                write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 0) & (C == Core::C68020 ? ~0u : ~1u), sr);
                write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 2) & (C == Core::C68020 ? ~0u : ~1u), pc >> 16);
            }
            break;

        case Core::C68010:
        case Core::C68020:

            if constexpr ((compat == Compatibility::MUSASHI)) {

                push<C, Word>(nr << 2);
                push<C, Long>(pc);
                push<C, Word>(sr);

            } else {

                U32_DEC(reg.sp, 8);
                if constexpr ((compat == Compatibility::UAE) && C == Core::C68010) {
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 4) & (C == Core::C68020 ? ~0u : ~1u), pc & 0xFFFF);
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 0) & (C == Core::C68020 ? ~0u : ~1u), sr);
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 2) & (C == Core::C68020 ? ~0u : ~1u), pc >> 16);
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 6) & (C == Core::C68020 ? ~0u : ~1u), 4 * nr);
                } else if constexpr ((compat == Compatibility::UAE) && C == Core::C68020) {
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 6) & (C == Core::C68020 ? ~0u : ~1u), 4 * nr);
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 2) & (C == Core::C68020 ? ~0u : ~1u), pc >> 16);
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 4) & (C == Core::C68020 ? ~0u : ~1u), pc & 0xFFFF);
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 0) & (C == Core::C68020 ? ~0u : ~1u), sr);
                } else {
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 6) & (C == Core::C68020 ? ~0u : ~1u), 4 * nr);
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 4) & (C == Core::C68020 ? ~0u : ~1u), pc & 0xFFFF);
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 0) & (C == Core::C68020 ? ~0u : ~1u), sr);
                    write<C, AddrSpace::DATA, Word>(U32_ADD(reg.sp, 2) & (C == Core::C68020 ? ~0u : ~1u), pc >> 16);
                }
            }
            break;
    }
}

template <Core C> void
Moira::writeStackFrame0001(u16 sr, u32 pc, u16 nr)
{
    assert(C == Core::C68020);

    // 0001 | Vector offset
    push<C, Word>(0x1000 | nr << 2);

    // Program counter
    push<C, Long>(pc);

    // Status register
    push<C, Word>(sr);
}

template <Core C> void
Moira::writeStackFrame0010(u16 sr, u32 pc, u32 ia, u16 nr)
{
    assert(C == Core::C68020);

    // Instruction address
    push<C, Long>(ia);

    // 0010 | Vector offset
    push<C, Word>(0x2000 | nr << 2);

    // Program counter
    push<C, Long>(pc);

    // Status register
    push<C, Word>(sr);
}

template <Core C> void
Moira::writeStackFrame0111(u16 sr, u32 pc, u16 nr, u16 ssw, u32 addr, u32 dataOut)
{
    // Write-back 1 (unused)
    for (int i = 0; i < 4; i++) push<C, Long>(0); // PD3 ... PD0
    push<C, Long>(0);                             // WB1A

    // Write-back 2 (unused)
    push<C, Long>(0);                             // WB2D
    push<C, Long>(0);                             // WB2A

    // Write-back 3 (the faulting write)
    bool write = !(ssw & SSW040::RW);
    push<C, Long>(dataOut);                       // WB3D
    push<C, Long>(write ? addr : 0);              // WB3A

    // Fault address
    push<C, Long>(addr);

    // Write-back status words
    push<C, Word>(0);                             // WB1S
    push<C, Word>(0);                             // WB2S
    push<C, Word>(write ? 0x80 | (ssw & 0x7F) : 0); // WB3S

    // Special status word
    push<C, Word>(ssw);

    // Effective address
    push<C, Long>(addr);

    // 0111 | Vector offset
    push<C, Word>(0x7000 | nr << 2);

    // Program counter
    push<C, Long>(pc);

    // Status register
    push<C, Word>(sr);
}

template <Core C> void
Moira::writeStackFrame1000(StackFrame &frame, u16 sr, u32 pc, u32 ia, u16 nr, u32 addr)
{
    assert(C == Core::C68010);

    // Internal information
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);

    // Instruction input buffer
    push<C, Word>(queue.irc);

    // Unused, reserved
    reg.sp -= 2;

    // Data input buffer
    push<C, Word>(readBuffer);

    // Unused, reserved
    reg.sp -= 2;

    // Data output buffer
    push<C, Word>(writeBuffer);

    // Unused, reserved
    reg.sp -= 2;

    // Fault address
    push<C, Long>(frame.addr);

    // Special status word
    push<C, Word>(frame.ssw);

    // 1000 | Vector offset
    push<C, Word>(0x8000 | nr << 2);

    // Program counter
    push<C, Long>(pc);

    // Status register
    push<C, Word>(sr);
}

template <Core C> void
Moira::writeStackFrame1001(u16 sr, u32 pc, u32 ia, u16 nr)
{

}

template <Core C> void
Moira::writeStackFrame1010(u16 sr, u32 pc, u16 nr, u16 ssw, u32 addr, u32 dataOut, u16 stageC, u16 stageB)
{
    // Internal registers
    push<C, Word>(0);
    push<C, Word>(0);

    // Data output buffer
    push<C, Long>(dataOut);

    // Internal registers
    push<C, Word>(0);
    push<C, Word>(0);

    // Data cycle fault address
    push<C, Long>(addr);

    // Instruction pipe stage B
    push<C, Word>(stageB);

    // Instruction pipe stage C
    push<C, Word>(stageC);

    // Special status word
    push<C, Word>(ssw);

    // Internal register
    push<C, Word>(0);

    // 1010 | Vector offset
    push<C, Word>(0xA000 | nr << 2);

    // Program counter
    push<C, Long>(pc);

    // Status register
    push<C, Word>(sr);
}

template <Core C> void
Moira::writeStackFrame1011(u16 sr, u32 pc, u32 ia, u16 nr, u16 ssw, u32 addr, u32 dataOut, u16 stageC, u16 stageB)
{
    // Internal registers
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);
    push<C, Long>(0);

    // Version#, Internal information
    push<C, Word>(0);

    // Internal registers
    push<C, Long>(0);
    push<C, Word>(0);

    // Data input buffer
    push<C, Long>(0);

    // Internal registers
    push<C, Long>(0);

    // Stage B address
    push<C, Long>(0);

    // Internal registers
    push<C, Long>(0);
    push<C, Long>(0);

    // Data output buffer
    push<C, Long>(dataOut);

    // Internal registers
    push<C, Word>(0);
    push<C, Word>(0);

    // Data cycle fault address
    push<C, Long>(addr);

    // Instruction pipe stage B
    push<C, Word>(stageB);

    // Instruction pipe stage C
    push<C, Word>(stageC);

    // Special status register
    push<C, Word>(ssw);

    // Internal register
    push<C, Word>(0);

    // 1011 | Vector offset
    push<C, Word>(0xB000 | nr << 2);

    // Program counter
    push<C, Long>(ia);

    // Status register
    push<C, Word>(sr);
}

/* The following code reproduces the 68020 bus error and address error frames of UAE.
 * UAE reports the stacked PC, the SSW and the pipe stages by simple rules. Many internal
 * words are not modeled. All frames use the long format ($B).
 */
static bool hasIndexedOperand(u16 ird)
{
    auto indexed = [](u16 m, u16 r) { return m == 6 || (m == 7 && r == 3); };

    switch (ird >> 12) {

        case 0x6: case 0x7: case 0xA: case 0xF: return false;
        case 0xE: if (((ird >> 6) & 3) != 3) return false; break;
        case 0x4: if ((ird & 0xFF80) == 0x4E00) return false; break;   // TRAP, LINK, UNLK, RTS, ...
    }
    if (indexed((ird >> 3) & 7, ird & 7)) return true;

    // MOVE has a second effective address
    return (ird & 0xC000) == 0 && (ird & 0x3000) && indexed((ird >> 6) & 7, (ird >> 9) & 7);
}

template <Core C> void
Moira::writeStackFrame1011UAE(u16 sr, u16 nr, u32 pc, u16 ssw, u32 addr, u32 end, bool sv)
{
    u16 stageC = 0, stageB = 0;
    if (nr == 2 && !busWriteAccess) {
        stageC = peekProgramWord(end + 2, sv);
        stageB = peekProgramWord(end + 4, sv);
    }
    writeStackFrame1011<C>(sr, 0, pc, nr, ssw, addr, 0, stageC, stageB);
}

template <Core C> void
Moira::execAddressError(StackFrame frame, int delay)
{
    u16 status = getSR();
    u32 pcNow = reg.pc, pc0Now = reg.pc0;
    u16 ssw = 0x60 | (readFC() & 7);

    // Inform the delegate
    willExecute(M68kException::ADDRESS_ERROR, 3);

    // Emulate additional delay
    sync(delay);

    // Enter supervisor mode
    setSupervisorMode(true);

    // Disable tracing
    clearTraceFlags();
    flags &= ~State::TRACE_EXC;

    // Interrupts are taken at an instruction boundary
    reg.pc = reg.pc0;
    SYNC(8);

    // A misaligned stack pointer will cause a double fault
    if (misaligned<C>(reg.sp)) throw DoubleFault();

    // Write stack frame
    // MAME delays the stack frame instead of the vector fetch
    if constexpr ((compat == Compatibility::MAME || (compat == Compatibility::UAE && C == Core::C68000))) SYNC(2);

    if constexpr (C == Core::C68000) {
        writeStackFrameAEBE<C>(frame);
    } else if constexpr (C == Core::C68020) {

        // The 68020 reports an odd instruction fetch with a frame of format B. According
        // to recordings made on a real A1200, the stacked PC is always the address of the
        // instruction (JMP, JSR, BSR, DBcc, RTS, RTR).
        u16 op = instrIrd;
        u32 pc = pc0Now;

        // Stage C and B are not emulated faithfully (UAE shows the next instruction word)
        bool ix = (op & 0xFFC0) == 0x4EC0 && hasIndexedOperand(op);
        u16 stageC = ix ? queue.irc : 0, stageB = (op == 0x4E75 || op == 0x4E77) ? queue.irc : 0;

        // Special status word as recorded on the A1200: Rerun flags for stage C (always) and
        // stage B (except for JMP d16(An) and BSR), supervisor program space, and the data
        // direction / size bits of the stack read performed by RTS and RTR
        bool jmpD16 = (op & 0xFFF8) == 0x4EE8;
        ssw = 0x2005 | ((jmpD16 || (op >> 8) == 0x61) ? 0 : 0x1000);
        if (op == 0x4E75) ssw |= 0x40;
        if (op == 0x4E77) ssw |= 0x60;
        writeStackFrame1011<C>(status, 0, pc, 3, ssw, frame.addr, 0, stageC, stageB);

    } else {
        writeStackFrame1000<C>(frame, status, frame.pc, reg.pc0, 3, frame.addr);
    }
    if constexpr (!(compat == Compatibility::MAME || (compat == Compatibility::UAE && C == Core::C68000))) SYNC(2);

    // Jump to exception vector
    jumpToVector<C>(3);

    // Inform the delegate
    didExecute(M68kException::ADDRESS_ERROR, 3);
}

template <Core C> void
Moira::execBusError(StackFrame frame, int delay)
{
    u16 status = getSR();
    u16 ssw = 0x60 | (readFC() & 7);
    u32 end = reg.pc0 != instrStart ? reg.pc0 : reg.pc;

    // Inform the delegate
    willExecute(M68kException::BUS_ERROR, 2);

    // Emulate additional delay
    sync(delay);

    // Enter supervisor mode
    Registers faultReg = reg;
    setSupervisorMode(true);
    u32 entrySsp = reg.sp, entryUsp = reg.usp;

    // Disable tracing
    clearTraceFlags();
    flags &= ~State::TRACE_EXC;
    SYNC(8);

    // A misaligned stack pointer will cause a double fault
    if (misaligned<C>(reg.sp)) throw DoubleFault();

    // Write stack frame
    // MAME completes the faulting bus cycle and delays the frame instead of the vector fetch
    if constexpr (EXACT_68000) sync(4);

    if constexpr (C == Core::C68000) {
        writeStackFrameAEBE<C>(frame);
    } else if constexpr (C == Core::C68020) {

        /* The 68030 reports the cause of the fault in the special status word
         * and the logical fault address. A fault in a write cycle is reported
         * with the short format ($A), all others with the long format ($B).
         */
        if (cpuModel >= CPUModel::M68EC040) {

            // The write-back buffer 3 holds the data of a faulting write
            bool write = !(frame.ssw & SSW040::RW);
            writeStackFrame0111<C>(status, reg.pc0, 2, frame.ssw, frame.addr, write ? busWriteData : 0);

        } else if (cpuModel >= CPUModel::M68EC030) {

            // The pipe stages hold the two instruction words behind the opcode
            bool supervisor = status & 0x2000;
            u16 stageC = peekProgramWord(reg.pc0 + 2, supervisor);
            u16 stageB = peekProgramWord(reg.pc0 + 4, supervisor);

            // The data output buffer holds the value of a faulting write
            bool write = !(frame.ssw & SSW030::RW) && (frame.ssw & SSW030::DF);
            u32 dataOut = write ? busWriteData : 0;

            if (write) {
                writeStackFrame1010<C>(status, reg.pc0, 2, frame.ssw, frame.addr, dataOut, stageC, stageB);
            } else {
                writeStackFrame1011<C>(status, frame.pc, reg.pc0, 2, frame.ssw, frame.addr, dataOut, stageC, stageB);
            }
        } else if (compat == Compatibility::UAE) {

            // UAE stacks the start of the instruction. Indexed operands move it behind the instruction words.
            // The extension word of the destination of a MOVE is consumed after the source has been read
            bool ix = hasIndexedOperand(instrIrd);
            if (ix && (instrIrd & 0xC000) == 0 && (instrIrd & 0x3000)) {
                auto indexed = [](u16 m, u16 r) { return m == 6 || (m == 7 && r == 3); };
                if (!indexed((instrIrd >> 3) & 7, instrIrd & 7) && !busWriteAccess) ix = false;
            }
            u32 pc = ix ? end : instrStart;

            // If the source of a MOVE is indexed, UAE stacks the address of the extension words of the destination
            if (ix && (instrIrd & 0xC000) == 0 && (instrIrd & 0x3000) && busWriteAccess) {
                auto indexed = [](u16 m, u16 r) { return m == 6 || (m == 7 && r == 3); };
                u16 dm = (instrIrd >> 6) & 7, dr = (instrIrd >> 9) & 7;
                if (indexed((instrIrd >> 3) & 7, instrIrd & 7)) {
                    if (dm == 5 || (dm == 7 && dr == 0)) pc -= 2;
                    else if (dm == 7 && dr == 1) pc -= 2;
                }
            }
            writeStackFrame1011UAE<C>(status, 2, pc, ssw, frame.addr, end, status & 0x2000);

            // Remember how to restart the instruction
            restart.valid = savedRegValid;
            if (restart.valid) {

                restart.pre = savedReg;
                restart.fault = faultReg;
                auto sr = reg.sr; reg.sr = savedReg.sr; restart.preSr = getSR(); reg.sr = sr;
                restart.pc = instrStart;
                restart.frameSp = reg.sp;
                restart.frameSr = status;
                restart.framePC = pc;
                restart.entrySsp = entrySsp;
                restart.entryUsp = entryUsp;
            }

        } else {
            writeStackFrame1011<C>(status, frame.pc, reg.pc0, 2);
        }
    } else {
        writeStackFrame1000<C>(frame, status, frame.pc, reg.pc0, 2, frame.addr);
    }
    if constexpr (!(EXACT_68000)) SYNC(2);

    // Jump to exception vector
    jumpToVector<C>(2);

    // Inform the delegate
    didExecute(M68kException::BUS_ERROR, 2);
}

// The following definitions are not templated. They are compiled into the
// main translation unit only (see MoiraCore_cpp.h).
#ifdef MOIRA_MAIN_TU

u16
Moira::peekProgramWord(u32 addr, bool supervisor)
{
    addr &= addrMask();

    if (mmu.isEnabled()) {

        auto phys = mmu.translate(addr, supervisor ? FC::SUPERVISOR_PROG : FC::USER_PROG, false);
        if (!phys) return 0;
        addr = *phys & addrMask();
    }

    return read16(addr);
}

void
Moira::execException(M68kException exc, int nr)
{
    switch (cpuModel) {

        case CPUModel::M68000: execException<Core::C68000>(exc, nr); break;
        case CPUModel::M68010: execException<Core::C68010>(exc, nr); break;
        default:            execException<Core::C68020>(exc, nr); break;
    }
}

#endif

template <Core C> void
Moira::execException(M68kException exc, int nr)
{
    u16 status = getSR();

    // Determine the exception vector number
    u16 vector = (exc == M68kException::TRAP) ? u16(exc) + u16(nr) : (exc == M68kException::BKPT) ? 4 : u16(exc);

    // Inform the delegate
    willExecute(exc, vector);

    // Remember the exception vector
    exception = vector;

    // Enter supervisor mode and leave trace mode
    setSupervisorMode(true);
    clearTraceFlags();

    // A misaligned stack pointer causes a double fault
    if constexpr (EXACT_68000) {
        if (exc != M68kException::BUS_ERROR && misaligned<C>(reg.sp)) throw DoubleFault();
    }

    switch (exc) {

        case M68kException::BUS_ERROR:

            // Write stack frame
            writeStackFrame1011<C>(status, reg.pc, reg.pc0, 2);

            // Branch to exception handler
            jumpToVector<C>(2);
            break;

        case M68kException::ILLEGAL:
        case M68kException::LINEA:
        case M68kException::LINEF:

            // Clear any pending trace event
            flags &= ~State::TRACE_EXC;

            SYNC(4);

            // Write stack frame
            if constexpr (EXACT_68000) {

                // MAME marks bus errors during the exception sequence as "not instruction" cycles
                try {
                    try { writeStackFrame0000<C>(status, reg.pc - 2, vector); }
                    catch (BusError &e) { e.stackFrame.pc -= 2; throw; }
                    jumpToVector<C, AE_SET_CB3>(vector);
                }
                catch (BusError &e) { e.stackFrame.code |= 0x8; throw; }
                break;
            }
            if constexpr (C == Core::C68010 || C == Core::C68020) {
                writeStackFrame0000<C>(status, reg.pc0, vector);
            } else {
                writeStackFrame0000<C>(status, reg.pc - 2, vector);
            }

            // Branch to exception handler
            jumpToVector<C, AE_SET_CB3>(vector);
            break;

        case M68kException::BKPT:

            // Clear any pending trace event
            flags &= ~State::TRACE_EXC;

            SYNC(2);
            (void)readM<C, Mode::DN, Word>(reg.pc);
            SYNC(2);

            // Write stack frame
            writeStackFrame0000<C>(status, U32_SUB(reg.pc, 2), vector);

            // Branch to exception handler
            jumpToVector<C, AE_SET_CB3>(vector);
            break;

        case M68kException::DIVIDE_BY_ZERO:
        case M68kException::CHK:
        case M68kException::TRAPV:

            // Write stack frame
            C == Core::C68020 ?
            writeStackFrame0010<C>(status, reg.pc, reg.pc0, vector) :
            writeStackFrame0000<C>(status, reg.pc, vector);

            // Branch to exception handler
            jumpToVector<C,AE_FETCH>(vector);
            break;

        case M68kException::PRIVILEGE:

            // Clear any pending trace event
            flags &= ~State::TRACE_EXC;

            SYNC(4);

            // Write stack frame
            if constexpr (EXACT_68000) {

                try {
                    try { writeStackFrame0000<C>(status, reg.pc - 2, vector); }
                    catch (BusError &e) { e.stackFrame.pc -= 2; throw; }
                    jumpToVector<C,AE_SET_CB3>(vector);
                }
                catch (BusError &e) { e.stackFrame.code |= 0x8; throw; }
                break;
            }
            writeStackFrame0000<C>(status, reg.pc - 2, vector);

            // Branch to exception handler
            jumpToVector<C,AE_SET_CB3>(vector);
            break;

        case M68kException::TRACE:

            // Clear any pending trace event
            flags &= ~State::TRACE_EXC;

            // Recover from stop state
            flags &= ~State::STOPPED;

            SYNC(4);

            // Trace exceptions are taken at an instruction boundary
            reg.pc = reg.pc0;

            // Write stack frame (the 68020 stacks the address of the traced instruction)
            if constexpr (C == Core::C68020) {
                writeStackFrame0010<C>(status, reg.pc, instrStart, vector);
            } else {
                writeStackFrame0000<C>(status, reg.pc, vector);
            }

            // Branch to exception handler
            jumpToVector<C>(vector);
            break;

        case M68kException::FORMAT_ERROR:

            // Clear any pending trace event
            flags &= ~State::TRACE_EXC;

            // Write stack frame
            if ((compat == Compatibility::MUSASHI)) {
                writeStackFrame0000<C>(status, reg.pc, vector);
            } else {
                writeStackFrame0000<C>(status, reg.pc - 2, vector);
            }

            // Branch to exception handler
            jumpToVector<C, AE_SET_CB3>(vector);
            break;

        case M68kException::TRAP:

            // Write stack frame
            writeStackFrame0000<C>(status, reg.pc, u16(vector));

            // Branch to exception handler
            jumpToVector<C>(vector);
            break;

        default:
            break;
    }

    // Inform the delegate
    didExecute(exc, vector);
}

// The following definitions are not templated. They are compiled into the
// main translation unit only (see MoiraCore_cpp.h).
#ifdef MOIRA_MAIN_TU

void
Moira::execInterrupt(u8 level)
{
    switch (cpuModel) {

        case CPUModel::M68000: execInterrupt<Core::C68000>(level); break;
        case CPUModel::M68010: execInterrupt<Core::C68010>(level); break;
        
        default:
            execInterrupt<Core::C68020>(level);
    }
}

#endif

template <Core C> void
Moira::execInterrupt(u8 level)
{
    assert(level < 8);

    // Notify delegate
    willInterrupt(level);

    // Level 7 is edge triggered
    if (level == 7) nmiTaken = true;

    // Remember the current value of the status register
    u16 status = getSR();

    // Recover from stop state and terminate loop mode
    flags &= ~(State::STOPPED | State::LOOPING);

    // Clear the polled IPL value
    reg.ipl = 0;

    // Temporarily raise the interrupt threshold
    reg.sr.ipl = level;

    // Enter supervisor mode
    setSupervisorMode(true);

    // Disable tracing
    clearTraceFlags();
    flags &= ~State::TRACE_EXC;

    switch (C) {

        case Core::C68000:

            SYNC(6);
            reg.sp -= 6;
            write<C, AddrSpace::DATA, Word>(reg.sp + 4, (reg.pc - 2) & 0xFFFF);

            SYNC(4);
            queue.ird = getIrqVector(level);

            SYNC(4);
            write<C, AddrSpace::DATA, Word>(reg.sp + 0, status);
            write<C, AddrSpace::DATA, Word>(reg.sp + 2, (reg.pc - 2) >> 16);
            break;

        case Core::C68010:

            SYNC(12);
            reg.sp -= 8;
            queue.ird = getIrqVector(level);
            write<C, AddrSpace::DATA, Word>(reg.sp + 4, (reg.pc - 2) & 0xFFFF);
            write<C, AddrSpace::DATA, Word>(reg.sp + 0, status);
            write<C, AddrSpace::DATA, Word>(reg.sp + 2, (reg.pc - 2) >> 16);
            write<C, AddrSpace::DATA, Word>(reg.sp + 6, 4 * queue.ird);
            break;

        case Core::C68020:

            queue.ird = getIrqVector(level);

            // Both frame writers take the vector number, not the vector offset
            writeStackFrame0000<C>(status, reg.pc - 2, queue.ird);

            if (reg.sr.m) {

                setMasterMode(false);
                writeStackFrame0001<C>(status | 0x2000, reg.pc - 2, queue.ird);
            }
    }

    jumpToVector<C, AE_SET_CB3>(queue.ird);
}
