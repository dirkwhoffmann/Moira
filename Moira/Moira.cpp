// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "MoiraConfig.h"
#include "Moira.h"
#include "MoiraMacros.h"

#include <cassert>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <bit>
#include <vector>
#include <stdexcept>

namespace MOIRA_NAMESPACE {

using namespace Flag;

// Mark this file as the main translation unit
#define MOIRA_MAIN_TU

#include "MoiraInit_cpp.h"
#include "MoiraALU_cpp.h"
#include "MoiraDataflow_cpp.h"
#include "MoiraExceptions_cpp.h"
#include "MoiraExec_cpp.h"
#include "StrWriter_cpp.h"
#include "MoiraDasm_cpp.h"

// Instantiations used by the caches (see Cache.cpp)
template u32 Moira::translate<Core::C68020, false, Word>(u32, u8, bool *);
template u32 Moira::translate<Core::C68020, false, Long>(u32, u8, bool *);
template u32 Moira::readLongSplit<Core::C68020, 0>(u32);

Moira::Moira()
{
    exec = new ExecPtr[65536];
    loop = new ExecPtr[65536];
    if (MOIRA_ENABLE_DASM) info = new InstrInfo[65536];
    if (MOIRA_ENABLE_DASM) dasm = new DasmPtr[65536];

    createJumpTable(cpuModel, dasmModel);

    instrStyle = DasmStyle {

        .syntax         = Syntax::MOIRA,
        .letterCase     = LetterCase::MIXED_CASE,
        .numberFormat   = { .prefix = "$", .radix = 16, .upperCase = false, .plainZero = false },
        .tab            = 8
    };

    dataStyle = DasmStyle {

        .syntax         = Syntax::MOIRA,
        .letterCase     = LetterCase::MIXED_CASE,
        .numberFormat   = { .prefix = "", .radix = 16, .upperCase = false, .plainZero = false },
        .tab            = 1
    };
}

Moira::~Moira()
{
    if (exec) delete [] exec;
    if (loop) delete [] loop;
    if (info) delete [] info;
    if (dasm) delete [] dasm;
}

std::string
Moira::version()
{
    std::string result;
    
    result = std::to_string(Version::major) + "." + std::to_string(Version::minor);
    if constexpr (Version::patch > 0) result += "." + std::to_string(Version::patch);
    if constexpr (Version::beta > 0) result += 'b' + std::to_string(Version::beta);
    
    return result;
}

std::string
Moira::build()
{
#ifdef NDEBUG
    std::string db = "";
#else
    std::string db = " [DEBUG BUILD]";
#endif
    
    return version() + db + " (" + __DATE__ + " " + __TIME__ + ")";
}

void
Moira::setModel(CPUModel cpuModel, CPUModel dasmModel)
{
    if (this->cpuModel != cpuModel || this->dasmModel != dasmModel) {
        
        this->cpuModel = cpuModel;
        this->dasmModel = dasmModel;

        updateUnits();

        createJumpTable(cpuModel, dasmModel);

        reg.cacr &= cacrMask();
        iCache.reset(cpuModel);
        dCache.reset(cpuModel);
        
        // The 68040 caches are not emulated. They are physically tagged, 4-way
        // set-associative and support copyback mode. Without them, the CPU
        // behaves like it had perfectly coherent caches.
        iCache.emulated = MOIRA_EMULATE_ICACHE && cpuModel >= CPUModel::M68EC020 && cpuModel < CPUModel::M68EC040;
        dCache.emulated = MOIRA_EMULATE_DCACHE && (cpuModel == CPUModel::M68030 || cpuModel == CPUModel::M68EC030);
        dCacheOn = dCache.active();
        flags &= ~State::LOOPING;
    }
}

void
Moira::updateUnits()
{
    // The FPU of the 68040 is built in. The 68020 and the 68030 can be
    // connected to a 6888x via the coprocessor interface. All other models have
    // no FPU.
    if (isInternalFpu()) {
        fpu.setModel(FPUModel::M68040);
    } else if (isExternalFpu()) {
        fpu.setModel(copModel == CopModel::M68881 ? FPUModel::M68881 : FPUModel::M68882);
    }

    // The MMU is part of the CPU
    MMUModel mmuType = MMUModel::NONE;
    if (hasMMU(cpuModel)) {
        mmuType = (cpuModel == CPUModel::M68LC040 || cpuModel == CPUModel::M68040) ? MMUModel::M68040 : MMUModel::M68030;
    }
    mmu.setModel(mmuType);
    updateMmuFlag();
}

void
Moira::setCopModel(CopModel model)
{
    if (copModel == model) return;

    copModel = model;
    updateUnits();

    // The FPU handlers and the disassembler entries depend on the coprocessor
    if (hasCoproInterface(cpuModel) || hasCoproInterface(dasmModel)) createJumpTable(cpuModel, dasmModel);
}

bool
Moira::hasCoproInterface(CPUModel model)
{
    switch (model) {

        case CPUModel::M68EC020:
        case CPUModel::M68020:
        case CPUModel::M68EC030:
        case CPUModel::M68030:     return true;
        default:                return false;
    }
}

bool
Moira::isInternalFpu() const
{
    return cpuModel == CPUModel::M68040;
}

bool
Moira::isExternalFpu() const
{
    return hasCoproInterface(cpuModel) && copModel != CopModel::NONE;
}

void
Moira::setDasmSyntax(Syntax value)
{
    instrStyle.syntax = value;
}

void
Moira::setDasmLetterCase(LetterCase value)
{
    instrStyle.letterCase = value;
}

void
Moira::setNumberFormat(DasmStyle &style, const DasmNumberFormat &value)
{
    auto validPrefix = [&](DasmNumberFormat fmt) { return fmt.prefix != nullptr; };
    auto validRadix = [&](DasmNumberFormat fmt) { return fmt.radix == 10 || fmt.radix == 16; };

    if (!validPrefix(value)) {
        throw std::runtime_error("prefix must not be NULL");
    }
    if (!validRadix(value)) {
        throw std::runtime_error("radix must be 10 or 16");
    }

    style.numberFormat = value;
}

bool
Moira::hasCPI() const
{
    switch (cpuModel) {

        case CPUModel::M68EC020: case CPUModel::M68020: case CPUModel::M68EC030: case CPUModel::M68030:
            return true;

        default:
            return false;
    }
}

bool
Moira::hasMMU(CPUModel model)
{
    switch (model) {

        case CPUModel::M68030: case CPUModel::M68LC040: case CPUModel::M68040:
            return true;

        default:
            return false;
    }
}

bool
Moira::hasMMU() const
{
    return mmu.isPresent();
}

bool
Moira::hasFPU() const
{
    return isInternalFpu() || isExternalFpu();
}

u32
Moira::cacrMask() const
{
    switch (cpuModel) {

        case CPUModel::M68020: case CPUModel::M68EC020: return 0x0003;
        case CPUModel::M68030: case CPUModel::M68EC030: return 0x3313;

        // Instruction cache enable (bit 15) and data cache enable (bit 31)
        case CPUModel::M68EC040: case CPUModel::M68LC040: case CPUModel::M68040: return 0x80008000;

        default:
            return 0xFFFF;
    }
}

u32
Moira::addrMask() const
{
    switch (cpuModel) {

        case CPUModel::M68000: return addrMask<Core::C68000>();
        case CPUModel::M68010: return addrMask<Core::C68010>();
        
        default:
            return addrMask<Core::C68020>();
    }
}

void
Moira::reset()
{
    mmu.reset();
    fpu.reset();

    switch (cpuModel) {

        case CPUModel::M68000:    reset<Core::C68000>(); break;
        case CPUModel::M68010:    reset<Core::C68010>(); break;
        
        default:
            reset<Core::C68020>();
    }
}

template <Core C> void
Moira::reset()
{
    flags = State::CHECK_IRQ;
    updateMmuFlag();

    reg = { };
    reg.sr.s = 1;
    reg.sr.ipl = 7;

    ipl = 0;
    nmiTaken = false;
    fcl = 2;
    fcSource = 0;
    budget = { };

    iCache.flush();
    dCache.flush();
    dCacheOn = false;

    SYNC(16);

    // Read the initial (supervisor) stack pointer from memory
    SYNC(2);
    reg.sp = read16OnReset(0);
    SYNC(4);
    reg.isp = reg.sp = (read16OnReset(2) & ~0x1) | reg.sp << 16;
    SYNC(4);
    reg.pc = read16OnReset(4);
    SYNC(4);
    reg.pc = (read16OnReset(6) & ~0x1) | reg.pc << 16;

    // Fill the prefetch queue
    SYNC(4);
    queue.irc = read16OnReset(reg.pc & addrMask<C>());
    SYNC(2);
    prefetch<C>();

    // Reset subcomponents
    debugger.reset();

    // Inform the delegate
    cpuDidReset();
}

void
Moira::syncCp(int cycles)
{
    // Max number of instructions that may execute without advancing the clock
    static constexpr int maxStalled = 4;

    // Lower bound for the model time debt
    static constexpr int minAccum = -454;

    // Accumulate the cycle penalty (may become negative)
    budget.accumulated += cycles;

    if (budget.accumulated >= 2) {

        // The CPU is ahead of its environment. Spend everything that amounts
        // to whole bus cycles

        int step = budget.accumulated & ~1;
        budget.accumulated -= step;
        budget.stalled = 0;
        sync(step);

    } else if (++budget.stalled >= maxStalled) {

        // Nothing was spend for a while now. Since the environment is stepped
        // from here alone, it has to be moved forward regardless - otherwise
        // the emulation makes no progress and its main loop never terminates.
        // The borrowed bus cycle is booked as a debt.

        budget.stalled = 0;
        budget.accumulated -= 2;
        if (budget.accumulated < minAccum) budget.accumulated = minAccum;
        sync(2);
    }
}

void
Moira::execute()
{
    using namespace State;

    // Check the integrity of the IRQ flag
    if (reg.ipl > reg.sr.ipl || (reg.ipl == 7 && !nmiTaken)) assert(flags & CHECK_IRQ);

    // Check the integrity of the trace flag
    assert(!!(flags & TRACING) == reg.sr.t1);

    // Check the integrity of the program counter
    assert(reg.pc == reg.pc0 + 2);

    if constexpr (compat == Compatibility::MAME || compat == Compatibility::UAE) irdKeep = false;

    // Take the fast path or the slow path
    if (!flags) {

        //
        // Fast path: Call the instruction handler and return
        //

        instrStart = reg.pc0;
        if constexpr (compat == Compatibility::UAE) instrIrd = queue.ird;
        try {
            (this->*exec[queue.ird])(queue.ird);
        } catch (const std::exception &exc) {
            processException(exc);
        }

    } else {

        //
        // Slow path: Process flags one by one
        //

        if (flags & (HALTED | TRACE_EXC | TRACING)) {

            // Only continue if the CPU is not halted
            if (flags & HALTED) {
                sync(2);
                return;
            }

            // Process pending trace exception (if any)
            if (flags & TRACE_EXC) {
                execException(M68kException::TRACE);
                goto done;
            }

            // Check if the T flag is set inside the status register
            if ((flags & TRACING) && !(flags & STOPPED)) {
                flags |= TRACE_EXC;
            }
        }

        // Process pending interrupt (if any)
        if (flags & CHECK_IRQ) {

            try {
                if (checkForIrq()) goto done;
            } catch (const std::exception &exc) {
                processException(exc);
            }
        }

        // If the CPU is stopped, poll the IPL lines and return
        if (flags & STOPPED) {

            POLL_IPL;
            sync((compat == Compatibility::MUSASHI) ? 1 : 2);
            return;
        }

        // If logging is enabled, record the executed instruction
        if (flags & LOGGING) {
            debugger.logInstruction();
        }

        // Execute the instruction
        {
            // Save the registers if a faulted instruction can be restarted
            const Registers *saved = nullptr;
            if (flags & RESTARTABLE) { savedReg = reg; saved = &savedReg; }

            try {

                if (flags & LOOPING) {

                    assert(loop[queue.ird]);
                    (this->*loop[queue.ird])(queue.ird);

                } else {

                    instrStart = reg.pc0;
                    if constexpr (compat == Compatibility::UAE) instrIrd = queue.ird;
                    (this->*exec[queue.ird])(queue.ird);
                }

            } catch (const std::exception &exc) {
                processException(exc, saved);
            }
        }

    done:

        // Check if a breakpoint has been reached
        if (flags & CHECK_BP) {

            // Don't break if the instruction won't be executed due to tracing
            if (flags & TRACE_EXC) return;

            // Check if a softstop has been reached
            if (debugger.softstopMatches(reg.pc0)) didReachSoftstop(reg.pc0);

            // Check if a breakpoint has been reached
            if (debugger.breakpointMatches(reg.pc0)) didReachBreakpoint(reg.pc0);
        }
    }

    // Check the integrity of the program counter again
    assert(reg.pc == reg.pc0 + 2);
}

void
Moira::execute(i64 cycles)
{
    executeUntil(clock + cycles);
}

void
Moira::updateMmuFlag()
{
    // An address translation fault must be restartable. The same holds for a
    // bus error of the 68020 family if the client asks for bus error emulation.
    // The real CPU continues from its internal state after RTE. Restarting the
    // instruction is the approximation used by UAE. Other models and
    // configurations stay on the fast path, which does not save the registers.
    bool restartable = mmu.isEnabled() || (MOIRA_EMULATE_BUS_ERROR && cpuModel >= CPUModel::M68EC020);

    if (restartable) {
        flags |= State::RESTARTABLE;
    } else {
        flags &= ~State::RESTARTABLE;
    }
}

void
Moira::undoPredecrement(u32 addr, const Registers &saved)
{
    /* UAE decrements an address register after the operand has been read, and before
     * a pure write. Hence, only a faulting read has to be taken back. A write fault
     * sees the register as it is.
     */
    if (busWriteAccess) return;

    u16 ird = instrIrd;
    int regs[2] = { -1, -1 };
    u32 size = 0;

    switch (ird >> 12) {

        case 0x1: case 0x2: case 0x3:   // MOVE

            size = (ird >> 12) == 1 ? 1 : (ird >> 12) == 3 ? 2 : 4;
            if (((ird >> 3) & 7) == 4) regs[0] = ird & 7;
            if (((ird >> 6) & 7) == 4) regs[1] = (ird >> 9) & 7;
            break;

        case 0x8: case 0x9: case 0xC: case 0xD:

            // SBCD, SUBX, ABCD, ADDX in memory-to-memory form
            if ((ird & 0x0138) == 0x0108 && (ird & 0xC0) != 0xC0) {
                regs[0] = ird & 7; regs[1] = (ird >> 9) & 7;
                size = ((ird >> 12) == 8 || (ird >> 12) == 0xC) ? 1 : 1 << ((ird >> 6) & 3);
            }
            else if (((ird >> 3) & 7) == 4) regs[0] = ird & 7;
            break;

        case 0x4:

            // MOVEM stacks its registers in its own way
            if ((ird & 0xFB80) == 0x4880) break;
            if (((ird >> 3) & 7) == 4) regs[0] = ird & 7;
            break;

        case 0x0: case 0x5: case 0xB: case 0xE:

            if (((ird >> 3) & 7) == 4) regs[0] = ird & 7;
            break;

        default:
            break;
    }

    for (int i = 0; i < 2; i++) {

        int r = regs[i];
        if (r < 0 || reg.a[r] != addr || saved.a[r] == reg.a[r]) continue;

        // Both operands may use the same register
        bool both = regs[0] == regs[1];
        u32 delta = saved.a[r] - reg.a[r];
        reg.a[r] += (both && size) ? (r == 7 && size == 1 ? 2 : size) : delta;
        break;
    }
}

void
Moira::restoreRegisters(const Registers &saved)
{
    u8 ipl = reg.ipl;
    reg = saved;
    reg.ipl = ipl;

    // Bring the dependent flags in sync with the restored status register
    flags &= ~(State::TRACING | State::TRACE_EXC);
    if (reg.sr.t1) flags |= State::TRACING;
    flags |= State::CHECK_IRQ;
}

void
Moira::executeUntil(i64 cycle)
{
    while (clock < cycle) { execute(); }
}

void
Moira::processException(const std::exception &exc, const Registers *saved)
{
    switch (cpuModel) {

        case CPUModel::M68000: processException<Core::C68000>(exc, saved); break;
        case CPUModel::M68010: processException<Core::C68010>(exc, saved); break;
        
        default:
            processException<Core::C68020>(exc, saved);
    }
}

template <Core C> void
Moira::processException(const std::exception &exc, const Registers *saved)
{
    // A fault aborts the instruction and terminates loop mode
    flags &= ~State::LOOPING;

    try {

        if (auto ae = dynamic_cast<const AddressError *>(&exc); ae) {

            execAddressError<C>(ae->stackFrame);
            return;
        }

        if (auto be = dynamic_cast<const BusError *>(&exc); be) {

            // Abort the current instruction. It will be restarted by RTE.
            // The 68020 in UAE mode keeps the registers as they are and rolls
            // them back when the handler returns (see Restart).
            bool deferred = compat == Compatibility::UAE && cpuModel <= CPUModel::M68020;
            if (saved && !deferred) restoreRegisters(*saved);

            // UAE decrements an address register after the access has succeeded
            if (saved && deferred) undoPredecrement(be->stackFrame.addr, *saved);

            // UAE sets the condition codes of a MOVE before it writes the result
            if (saved && deferred && busWriteAccess && (instrIrd >> 12) >= 1 && (instrIrd >> 12) <= 3) {

                int bits = (instrIrd >> 12) == 1 ? 8 : (instrIrd >> 12) == 3 ? 16 : 32;
                u32 mask = bits == 32 ? 0xFFFFFFFF : (1u << bits) - 1;
                reg.sr.n = (busWriteData >> (bits - 1)) & 1;
                reg.sr.z = (busWriteData & mask) == 0;
                reg.sr.v = 0;
                reg.sr.c = 0;
            }

            // A bus error while processing a bus error is a double bus fault
            try { execBusError<C>(be->stackFrame, 0, saved); } catch (const BusError &) { halt(); }
            return;
        }

        if (auto df = dynamic_cast<const DoubleFault *>(&exc); df) {

            throw *df;
        }

    } catch (const DoubleFault &) {

        halt();
        return;
    }

    throw exc;
}

bool
Moira::checkForIrq()
{
    // Level 7 is edge triggered. The interrupt can be taken again once the polled level has dropped.
    if (reg.ipl < 7) nmiTaken = false;

    if (reg.ipl > reg.sr.ipl || (reg.ipl == 7 && !nmiTaken)) {

        // Exit loop mode
        if (flags & State::LOOPING) flags &= ~State::LOOPING;

        // Trigger interrupt
        execInterrupt(reg.ipl);

        return true;

    } else {

        // If the polled IPL is up to date, we disable interrupt checking for
        // the time being, because no interrupt can occur as long as the
        // external IPL or the IPL mask inside the status register keep the
        // same. If one of these variables changes, we reenable interrupt
        // checking.
        if (reg.ipl == ipl) flags &= ~State::CHECK_IRQ;

        return false;
    }
}

void
Moira::halt()
{
    // Halt the CPU
    flags |= State::HALTED;
    reg.pc = reg.pc0 + 2;

    // Inform the delegate
    cpuDidHalt();
}

u8
Moira::getCCR() const
{
    auto result =
    reg.sr.c << 0 |
    reg.sr.v << 1 |
    reg.sr.z << 2 |
    reg.sr.n << 3 |
    reg.sr.x << 4 ;

    return u8(result);
}

void
Moira::setCCR(u8 val)
{
    reg.sr.c = (val >> 0) & 1;
    reg.sr.v = (val >> 1) & 1;
    reg.sr.z = (val >> 2) & 1;
    reg.sr.n = (val >> 3) & 1;
    reg.sr.x = (val >> 4) & 1;
}

u16
Moira::getSR() const
{
    auto flags =
    reg.sr.t1  << 15 |
    reg.sr.t0  << 14 |
    reg.sr.s   << 13 |
    reg.sr.m   << 12 |
    reg.sr.ipl <<  8 ;

    return u16(flags | getCCR());
}

void
Moira::setSR(u16 val)
{
    bool t1  = (val >> 15) & 1;
    bool s   = (val >> 13) & 1;
    u8   ipl = (val >>  8) & 7;

    reg.sr.ipl = ipl;
    flags |= State::CHECK_IRQ;
    t1 ? setTraceFlag() : clearTraceFlag();

    setCCR(u8(val));
    setSupervisorMode(s);

    if (cpuModel > CPUModel::M68010) {

        bool t0 = (val >> 14) & 1;
        bool m = (val >> 12) & 1;

        t0 ? setTrace0Flag() : clearTrace0Flag();
        setMasterMode(m);
    }
}

void
Moira::setCACR(u32 val)
{
    // Setting the clear bits invalidates (parts of) the caches
    iCache.cacrDidChange(val);
    dCache.cacrDidChange(val);

    reg.cacr = val & cacrMask();
    dCacheOn = dCache.active();
    didChangeCACR(val);
}

void
Moira::setCAAR(u32 val)
{
    reg.caar = val;
    iCache.caarDidChange(val);
    dCache.caarDidChange(val);
    didChangeCAAR(val);
}

void
Moira::setSupervisorMode(bool s)
{
    if (s != reg.sr.s) setSupervisorFlags(s, reg.sr.m);
}

void
Moira::setMasterMode(bool m)
{
    if (m != reg.sr.m) setSupervisorFlags(reg.sr.s, m);
}

void
Moira::setSupervisorFlags(bool s, bool m)
{
    bool uspWasVisible = !reg.sr.s;
    bool ispWasVisible =  reg.sr.s && !reg.sr.m;
    bool mspWasVisible =  reg.sr.s &&  reg.sr.m;

    if (uspWasVisible) reg.usp = reg.sp;
    if (ispWasVisible) reg.isp = reg.sp;
    if (mspWasVisible) reg.msp = reg.sp;

    reg.sr.s = s;
    reg.sr.m = m;

    bool uspIsVisible  = !reg.sr.s;
    bool ispIsVisible  =  reg.sr.s && !reg.sr.m;
    bool mspIsVisible  =  reg.sr.s &&  reg.sr.m;

    if (uspIsVisible)  reg.sp = reg.usp;
    if (ispIsVisible)  reg.sp = reg.isp;
    if (mspIsVisible)  reg.sp = reg.msp;
}

u16
Moira::availabilityMask(Instr I) const
{

    switch (I) {

        case Instr::BKPT: case Instr::MOVEC: case Instr::MOVES: case Instr::MOVEFCCR: case Instr::RTD:

            return AV::M68010_UP;

        case Instr::CALLM: case Instr::RTM:

            return AV::M68020;

        case Instr::cpGEN: case Instr::cpRESTORE: case Instr::cpSAVE: case Instr::cpScc: case Instr::cpTRAPcc:

            return AV::M68020 | AV::M68030;

        case Instr::BFCHG: case Instr::BFCLR: case Instr::BFEXTS: case Instr::BFEXTU: case Instr::BFFFO:
        case Instr::BFINS: case Instr::BFSET: case Instr::BFTST: case Instr::CAS: case Instr::CAS2:
        case Instr::CHK2: case Instr::CMP2: case Instr::DIVL: case Instr::EXTB: case Instr::MULL:
        case Instr::PACK: case Instr::TRAPCC: case Instr::TRAPCS: case Instr::TRAPEQ: case Instr::TRAPGE:
        case Instr::TRAPGT: case Instr::TRAPHI: case Instr::TRAPLE: case Instr::TRAPLS: case Instr::TRAPLT:
        case Instr::TRAPMI: case Instr::TRAPNE: case Instr::TRAPPL: case Instr::TRAPVC: case Instr::TRAPVS:
        case Instr::TRAPF: case Instr::TRAPT: case Instr::UNPK:

            return AV::M68020_UP;

        case Instr::CINV: case Instr::CPUSH: case Instr::MOVE16:

            return AV::M68040;

        case Instr::PFLUSH: case Instr::PFLUSHA: case Instr::PFLUSHAN: case Instr::PFLUSHN: case Instr::PLOAD:
        case Instr::PMOVE: case Instr::PTEST:

            return AV::MMU;

        case Instr::FABS: case Instr::FADD: case Instr::FBcc: case Instr::FCMP: case Instr::FDBcc: case Instr::FDIV:
        case Instr::FMOVE: case Instr::FMOVEM: case Instr::FMUL: case Instr::FNEG: case Instr::FNOP:
        case Instr::FRESTORE: case Instr::FSAVE: case Instr::FScc: case Instr::FSQRT: case Instr::FSUB:
        case Instr::FTRAPcc: case Instr::FTST:

        case Instr::FSABS: case Instr::FDABS: case Instr::FSADD: case Instr::FDADD: case Instr::FSDIV: case Instr::FDDIV:
        case Instr::FSMOVE: case Instr::FDMOVE: case Instr::FSMUL: case Instr::FDMUL: case Instr::FSNEG:
        case Instr::FDNEG: case Instr::FSSQRT: case Instr::FDSQRT: case Instr::FSSUB: case Instr::FDSUB:

            return AV::FPU;

        case Instr::FACOS: case Instr::FASIN: case Instr::FATAN: case Instr::FATANH: case Instr::FCOS: case Instr::FCOSH:
        case Instr::FETOX: case Instr::FETOXM1: case Instr::FGETEXP: case Instr::FGETMAN: case Instr::FINT:
        case Instr::FINTRZ: case Instr::FLOG10: case Instr::FLOG2: case Instr::FLOGN: case Instr::FLOGNP1:
        case Instr::FMOD: case Instr::FMOVECR: case Instr::FREM: case Instr::FSCAL: case Instr::FSGLDIV:
        case Instr::FSGLMUL: case Instr::FSIN: case Instr::FSINCOS: case Instr::FSINH: case Instr::FTAN:
        case Instr::FTANH: case Instr::FTENTOX: case Instr::FTWOTOX:

            return 0; // M6888x only

        default:

            return AV::M68000_UP;
    }
}

u16
Moira::availabilityMask(Instr I, Mode M, Size S) const
{
    u16 mask = availabilityMask(I);

    switch (I) {

        case Instr::CMPI:

            if (isPrgMode(M)) mask &= AV::M68010_UP;
            break;

        case Instr::CHK: case Instr::LINK: case Instr::BRA: case Instr::BHI: case Instr::BLS: case Instr::BCC: case Instr::BCS:
        case Instr::BNE: case Instr::BEQ: case Instr::BVC: case Instr::BVS: case Instr::BPL: case Instr::BMI: case Instr::BGE:
        case Instr::BLT: case Instr::BGT: case Instr::BLE: case Instr::BSR:

            if (S == Long) mask &= AV::M68020_UP;
            break;

        case Instr::TST:

            if (M == Mode(1) || M >= Mode(9)) mask &= AV::M68020_UP;
            break;

        default:

            break;
    }

    return mask;
}

u16 Moira::availabilityMask(Instr I, Mode M, Size S, u16 ext) const
{
    u16 mask = availabilityMask(I);

    switch (I) {

        case Instr::MOVEC:

            switch (ext & 0x0FFF) {

                case 0x000:
                case 0x001:
                case 0x800:
                case 0x801: mask &= AV::M68010_UP; break;
                case 0x002:
                case 0x803:
                case 0x804: mask &=  AV::M68020_UP; break;
                case 0x802: mask &=  AV::M68020 | AV::M68030; break;
                case 0x003:
                case 0x004:
                case 0x005:
                case 0x006:
                case 0x007:
                case 0x805:
                case 0x806:
                case 0x807: mask &= AV::M68040; break;

                default:
                    break;
            }
            break;

        case Instr::MOVES:

            if (ext & 0x7FF) mask = 0;
            break;

        default:
            break;
    }

    return mask;
}

bool
Moira::isAvailable(CPUModel model, Instr I) const
{
    return availabilityMask(I) & (1 << (int)model);
}

bool
Moira::isAvailable(CPUModel model, Instr I, Mode M, Size S) const
{
    return availabilityMask(I, M, S) & (1 << (int)model);
}

bool
Moira::isAvailable(CPUModel model, Instr I, Mode M, Size S, u16 ext) const
{
    return availabilityMask(I, M, S, ext) & (1 << (int)model);
}

const char *
Moira::availabilityString(Instr I, Mode M, Size S, u16 ext)
{
    switch (availabilityMask(I, M, S, ext)) {

        case AV::M68010_UP:           return "(1+)";
        case AV::M68020:              return "(2)";
        case AV::M68020 | AV::M68030: return "(2-3)";
        case AV::M68020_UP:           return "(2+)";
        case AV::M68040:              return "(4+)";

        default:
            return "(?)";
    }
}

bool
Moira::isValidExt(Instr I, Mode M, u16 op, u32 ext) const
{
    switch (I) {

        case Instr::BFCHG:     return (ext & 0xF000) == 0;
        case Instr::BFCLR:     return (ext & 0xF000) == 0;
        case Instr::BFEXTS:    return (ext & 0x8000) == 0;
        case Instr::BFEXTU:    return (ext & 0x8000) == 0;
        case Instr::BFFFO:     return (ext & 0x8000) == 0;
        case Instr::BFINS:     return (ext & 0x8000) == 0;
        case Instr::BFSET:     return (ext & 0xF000) == 0;
        case Instr::BFTST:     return (ext & 0xF000) == 0;
        case Instr::CAS:       return (ext & 0xFE38) == 0;
        case Instr::CAS2:      return (ext & 0x0E380E38) == 0;
        case Instr::CHK2:      return (ext & 0x07FF) == 0;
        case Instr::CMP2:      return (ext & 0x0FFF) == 0;
        case Instr::MULL:      return (ext & 0x83F8) == 0;
        case Instr::DIVL:      return (ext & 0x83F8) == 0;

        default:
            fatalError;
    }
}

u8
Moira::readFC() const
{
    switch (fcSource) {

        case 0: return u8((reg.sr.s ? 4 : 0) | fcl);
        case 1: return u8(reg.sfc);
        case 2: return u8(reg.dfc);
        case 3: return FC::CPU_SPACE;

        default:
            fatalError;
    }
}

void
Moira::setFC(u8 value)
{
    if constexpr (MOIRA_EMULATE_FC) {
        
        fcl = (u8)value;
    }
}

void
Moira::setIPL(u8 val)
{
    if (ipl != val) {
        
        ipl = val;
        flags |= State::CHECK_IRQ;
    }
}

void
Moira::reportCpuSpace(CpuSpaceType type, u8 arg)
{
    u8 oldSource = fcSource;
    fcSource = 3;
    cpuSpaceCycle(type, arg);
    fcSource = oldSource;
}

void
Moira::reportCoprocessorCycle(u16 opcode, u8 cir)
{
    // Coprocessor ID (opcode bits 11-9) on A15-A13, register select on A4-A0
    reportCpuSpace(CpuSpaceType::COPROCESSOR, u8(((opcode >> 9) & 7) << 5 | cir));
}

u16
Moira::getIrqVector(u8 level) {

    assert(level < 8);

    // The interrupt acknowledge cycle is a CPU space cycle (FC = 7)
    u8 oldSource = fcSource;
    fcSource = 3;
    cpuSpaceCycle(CpuSpaceType::IRQ_ACK, level);

    u16 vector = 0;
    switch (irqMode) {

        case IrqMode::AUTO:          vector = 24 + level; break;
        case IrqMode::USER:          vector = readIrqUserVector(level) & 0xFF; break;
        case IrqMode::SPURIOUS:      vector = 24; break;
        case IrqMode::UNINITIALIZED: vector = 15; break;

        default:
            fatalError;
    }

    fcSource = oldSource;
    return vector;
}

InstrInfo
Moira::getInstrInfo(u16 op) const
{
    if constexpr (MOIRA_ENABLE_DASM) {

        return info[op];

    } else {

        throw std::runtime_error("This feature requires MOIRA_ENABLE_DASM = true\n");
    }
}

template u32 Moira::readD <Long> (int n) const;
template u32 Moira::readA <Long> (int n) const;
template void Moira::writeD <Long> (int n, u32 v);
template void Moira::writeA <Long> (int n, u32 v);

}
