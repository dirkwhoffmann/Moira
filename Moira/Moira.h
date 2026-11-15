// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "MoiraConfig.h"
#include "MoiraTypes.h"
#include "MoiraDebugger.h"
#include "Cache/Cache.h"
#include "FPU/FPU.h"
#include "MMU/MMU.h"

namespace MOIRA_NAMESPACE {

/// Version of this release
namespace Version {

inline constexpr int major = 6;
inline constexpr int minor = 0;
inline constexpr int patch = 0;
inline constexpr int beta  = 1;

}

/** The Motorola 68k CPU emulator
 *
 * %Moira emulates the 68000, 68010, 68020, 68030 and 68040 (including the
 * 68EC and 68LC variants) together with an optional FPU and MMU. The class is
 * abstract. To use it, derive your own CPU class and implement the memory
 * access functions (see the protected functions below):
 *
 * ```cpp
 * class CPU : public moira::Moira {
 *
 *     u8 read8(u32 addr) const override;
 *     u16 read16(u32 addr) const override;
 *     u32 read32(u32 addr) const override;
 *     void write8(u32 addr, u8 val) const override;
 *     void write16(u32 addr, u16 val) const override;
 *     void write32(u32 addr, u32 val) const override;
 * };
 * ```
 *
 * The remaining virtual functions are optional delegates that inform the
 * application about events such as breakpoints, exceptions or cache register
 * changes.
 *
 * The interface functions are only virtual if the configuration option
 * MOIRA_VIRTUAL_API is enabled (the default). Otherwise, they are expected to
 * be provided by the application at compile time.
 */
class Moira {
    
    friend class Debugger;
    friend class Breakpoints;
    friend class Watchpoints;
    friend class Catchpoints;
    friend class MMU030;
    friend class MMU040;
    friend class Cache;
    friend class ICache;
    friend class DCache;
    
    
    //
    // Configuration
    //
    
protected:
    
    // The emulated CPU model
    CPUModel cpuModel = CPUModel::M68000;

    // The emulated coprocessor (if any)
    CPModel cpModel = CPModel::NONE;

    // Instruction set used by the disassembler
    CPUModel dasmModel = CPUModel::M68000;
    
    // Visual style for disassembled instructions
    DasmStyle instrStyle;
    
    // Visual style for data dumps
    DasmStyle dataStyle;
    
    
    //
    // Subcomponents
    //
    
public:
    
    // The memory management unit
    MMU mmu = MMU(*this);

    // The floating-point unit
    FPU fpu = FPU();

    // Debugger (breakpoints, watchpoints, catchpoints, tracing)
    Debugger debugger = Debugger(*this);

protected:

    // Instruction cache (68020+)
    ICache iCache = ICache(*this);

    // Data cache (68030+)
    DCache dCache = DCache(*this);

    
    //
    // Internals
    //
    
protected:
    
    // Number of elapsed cycles since power-up
    i64 clock {};
    
    // The CPU's register set
    Registers reg {};
    
    // Prefetch queue for fetching instructions
    PrefetchQueue queue {};

    // Speed trackers (68020 only)
    CycleBudget budget {};
    
    // Current value on the IPL (Interrupt Priority Level) pins
    u8 ipl {};

    // Set when a level 7 interrupt is taken; cleared when the pins leave level 7
    bool nmiTaken {};
    
    // Value on the lower two function code pins (FC1|FC0)
    u8 fcl {2};
    
    // Source of the function code pins
    u8 fcSource {};
    
    // Controls exact timing of instructions running in loop mode (68010 only)
    int loopModeDelay {2};
    
    // Read buffer (appears in 68010 exception frames)
    u16 readBuffer {};
    
    // Write buffer (appears in 68010 exception frames)
    u16 writeBuffer {};
    
    // Internal state flags
    int flags {};
    
    // Restart information of a faulted instruction (68020+)
    Restart restart {};

    
    //
    // Lookup tables
    //
    
private:
    
    // Function pointer types
    typedef void (Moira::*ExecPtr)(u16);
    typedef void (Moira::*DasmPtr)(StrWriter&, u32&, u16, Instr, Mode, Size) const;

    // Jump table holding the instruction handlers
    ExecPtr *exec = nullptr;
    
    // Jump table holding the loop mode instruction handlers (68010 only)
    ExecPtr *loop = nullptr;
    
    // Jump table holding the disassembler handlers
    DasmPtr *dasm = nullptr;
    
    // Info table holding instruction descriptions
    InstrInfo *info = nullptr;
    
    
    //
    // Constructing
    //
    
public:
    
    /// Constructs and initializes a Moira instance
    Moira();
    
    /// Destroys the Moira instance
    virtual ~Moira();
    
    /// Returns a version string for this release
    static std::string version();

    /// Returns a build number string for this release
    static std::string build();
    
protected:
    
    // Creates or updates the jump tables for execution and disassembly
    void createJumpTable(CPUModel cpuModel, CPUModel dasmModel);
    
    // Creates the jump tables for execution and disassembly with the same model
    void createJumpTable(CPUModel model) { createJumpTable(model, model); }
    
private:
    
    // Core routines for creating jump tables    
    template <Core C> void createJumpTable(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable1(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable2(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable3(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable4(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable5(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable6(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable7(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable8(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable9(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable10(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable11(CPUModel model, bool registerDasm);
    template <Core C> void createJumpTable12(CPUModel model, bool registerDasm);
    
    
    //
    // Configuring
    //
    
public:
    
    /// Sets the emulated CPU model and the model used by the disassembler
    void setModel(CPUModel cpuModel, CPUModel dasmModel);
    
    /// Sets the emulated CPU model (the disassembler uses the same model)
    void setModel(CPUModel model) { setModel(model, model); }

    /** Selects the coprocessor that is connected to the coprocessor interface
     *
     * The built-in units are determined by the CPU model: The 68040 has an FPU
     * and an MMU, the 68030 and the 68LC040 have an MMU, and the EC variants
     * have neither. A coprocessor (68881 or 68882) can be connected to CPUs
     * with a coprocessor interface (68EC020, 68020, 68EC030, 68030). For other
     * CPU models, the selection is remembered but has no effect. It is not
     * changed when the CPU model changes, so the order of setModel and
     * setCPModel does not matter.
     */
    void setCPModel(CPModel model);

    /// Returns the selected coprocessor (which may not be effective, see setCPModel)
    CPModel getCPModel() const { return cpModel; }
    
    /// Configures the syntax style for disassembly output
    void setDasmSyntax(Syntax value);
    
    /// Sets the number format for disassembly output
    void setDasmNumberFormat(DasmNumberFormat value) { setNumberFormat(instrStyle, value); }
    
    /// Sets the letter case for disassembly output
    void setDasmLetterCase(LetterCase value);
    
    /// Sets the indentation for disassembly output
    void setDasmIndentation(int value) { instrStyle.tab = value; }
    
    /// Sets the number format for data dumps
    void setDumpNumberFormat(DasmNumberFormat value) { setNumberFormat(dataStyle, value); }
    
    /// Sets the indentation for data dumps
    void setDumpIndentation(int value) { dataStyle.tab = value; }
    
private:
    
    // Configures the FPU and MMU to match the emulated CPU configuration
    void configureUnits();

    // Helper function
    void setNumberFormat(DasmStyle &style, const DasmNumberFormat &value);
    
    
    //
    // Querying CPU properties
    //
    
public:
    
    /// Checks if the emulated CPU has a coprocessor interface (CPI)
    bool hasCPI() const;
    
    /// Checks if the emulated CPU has a memory management unit (MMU)
    bool hasMMU() const;
    
    /// Checks if the emulated system has a floating-point unit (built-in or external)
    bool hasFPU() const;
    
    /// Checks if the emulated CPU has a built-in floating-point unit
    bool hasInternalFpu() const;

    /// Checks if an external FPU (68881 or 68882) is attached
    bool hasExternalFpu() const;
        
    /// Returns the address bus mask, which defines the CPU's addressable memory range
    u32 addrMask() const;
    
    /// Returns the cache register mask, indicating the accessible CACR bits
    u32 cacrMask() const;

protected:
    
    // Returns the address bus mask for a specific CPU core type
    template <Core C> u32 addrMask() const;
    
    
    //
    // Running the CPU
    //
    
public:
    
    /// Performs a hard reset, simulating the native power-up sequence
    void reset();
    
    /// Executes a single instruction
    void execute();

    /// Executes instructions for the given number of cycles.
    ///
    /// Note: Since the emulator cannot stop in the middle of an instruction,
    /// the number of actually elapsed cycles may exceed the specified cycle
    /// budget.
    void execute(i64 cycles);
    
    /// Executes instructions until a specific cycle count is reached.
    void executeUntil(i64 cycle);
    
    /// Checks if the CPU is in a HALT state
    bool isHalted() const { return flags & State::HALTED; }
    
private:
    
    // Performs a core-specific reset routine
    template <Core C> void reset();
    
    // Puts the CPU into a HALT state, stopping execution
    void halt();
    
    
    //
    // Running the disassembler
    //
    
public:
    
    /// Disassembles an instruction and returns the instruction size in bytes
    int disassemble(char *str, u32 addr) const;
    
    /// Creates a textual representation of the status register
    void disassembleSR(char *str) const { disassembleSR(str, reg.sr); }
    
    /// Creates a textual representation of a given status register
    void disassembleSR(char *str, const StatusRegister &sr) const;
    
    /// Converts an integer value to a textual representation
    void dump8(char *str, u8 value) const;
    /// Converts an integer value to a textual representation
    void dump16(char *str, u16 value) const;
    /// Converts an integer value to a textual representation
    void dump24(char *str, u32 value) const;
    /// Converts an integer value to a textual representation
    void dump32(char *str, u32 value) const;
    
    /// Converts multiple 16-bit values to a textual representation
    void dump16(char *str, u16 values[], int cnt) const;
    /// Converts multiple 16-bit values to a textual representation
    void dump16(char *str, u32 addr, int cnt) const;
    
    /// Returns instruction metadata for a given opcode
    InstrInfo getInstrInfo(u16 op) const;

    
    //
    // Clock
    //
    
public:
    
    /// Returns the current CPU clock cycle count (elapsed cycles since power-up)
    i64 getClock() const { return clock; }
    
    /// Sets the CPU clock cycle count
    void setClock(i64 val) { clock = val; }


protected:

    /* Advances the clock by the cycle count computed for a 68020 instruction.
     *
     * Cache hits on instruction fetches are modelled as cycle discounts (see
     * readInstructionWord), so the resulting count may be zero or negative.
     * Since the environment can only advance in whole bus cycles, the time is
     * accumulated until at least one bus cycle is available. This preserves the
     * full cache discount.
     *
     * If no bus cycle is accumulated for maxStalled consecutive instructions,
     * one cycle is borrowed to ensure progress. Since the environment is stepped
     * only here, failing to do so would stall the emulation indefinitely.
     */
    void syncCyclePenalty(int cycles);


    //
    // Registers
    //
    
public:
    
    /// Gets the value of a data register (0 - 7)
    u32 getD(int n) const { return readD(n); }
    /// Sets the value of a data register (0 - 7)
    void setD(int n, u32 v) { writeD(n, v); }
    
    /// Gets the value of an address register (0 - 7)
    u32 getA(int n) const { return readA(n); }
    /// Sets the value of an address register (0 - 7)
    void setA(int n, u32 v) { writeA(n, v); }
    
    /** Gets the address of the next instruction (at an instruction boundary)
     *
     * Internally, the program counter follows the hardware: it always holds
     * the address of the word in IRC, which is two bytes ahead of the
     * instruction to be executed next. This function hides the offset.
     */
    u32 getPC() const { return reg.pc - 2; }
    /// Sets the address of the next instruction (see getPC)
    void setPC(u32 val) { reg.pc = val + 2; }

    /// Gets the address of the currently executed instruction
    u32 getPC0() const { return reg.pc0; }
    /// Gets the address of the previously executed instruction
    u32 getPrevPC0() const { return reg.prev_pc0; }
    /// Sets the address of the currently executed instruction
    void setPC0(u32 val) { reg.pc0 = val; }

    // Called when an instruction (or exception) has been processed: pc0 moves to the next instruction
    void advancePC0() { reg.prev_pc0 = reg.pc0; reg.pc0 = reg.pc - 2; }
    
    /// Gets the IRC register, which is part of the prefetch queue
    u16 getIRC() const { return queue.irc; }
    /// Sets the IRC register, which is part of the prefetch queue
    void setIRC(u16 val) { queue.irc = val; }
    
    /// Gets the IRD register, which is part of the prefetch queue
    u16 getIRD() const { return queue.ird; }
    /// Sets the IRD register, which is part of the prefetch queue
    void setIRD(u16 val) { queue.ird = val; }
    
    /// Gets the value of the Condition Code Register (CCR)
    u8 getCCR() const;
    /// Sets the value of the Condition Code Register (CCR)
    void setCCR(u8 val);
    
    /// Gets the value of the Status Register (SR)
    u16 getSR() const;
    /// Sets the value of the Status Register (SR)
    void setSR(u16 val);
    
    /// Gets the current value of the stack pointer (SP)
    u32 getSP() const { return reg.sp; }
    /// Sets the current value of the stack pointer (SP)
    void setSP(u32 val) { reg.sp = val; }
    
    /// Gets the User Stack Pointer (USP)
    u32 getUSP() const { return !reg.sr.s ? reg.sp : reg.usp; }
    /// Sets the User Stack Pointer (USP)
    void setUSP(u32 val) { if (!reg.sr.s) reg.sp = val; else reg.usp = val; }
    
    /// Gets the Interrupt Stack Pointer (ISP)
    u32 getISP() const { return (reg.sr.s && !reg.sr.m) ? reg.sp : reg.isp; }
    /// Sets the Interrupt Stack Pointer (ISP)
    void setISP(u32 val) { if (reg.sr.s && !reg.sr.m) reg.sp = val; else reg.isp = val; }
    
    /// Gets the Master Stack Pointer (MSP)
    u32 getMSP() const { return (reg.sr.s && reg.sr.m) ? reg.sp : reg.msp; }
    /// Sets the Master Stack Pointer (MSP)
    void setMSP(u32 val) { if (reg.sr.s && reg.sr.m) reg.sp = val; else reg.msp = val; }
    
    /// Gets the Vector Base Register (VBR)
    u32 getVBR() const { return reg.vbr; }
    /// Sets the Vector Base Register (VBR)
    void setVBR(u32 val) { reg.vbr = val; }
    
    /// Gets the Source Function Code (SFC)
    u32 getSFC() const { return reg.sfc; }
    /// Sets the Source Function Code (SFC)
    void setSFC(u32 val) { reg.sfc = val & 0b111; }
    
    /// Gets the Destination Function Code (DFC)
    u32 getDFC() const { return reg.dfc; }
    /// Sets the Destination Function Code (DFC)
    void setDFC(u32 val) { reg.dfc = val & 0b111; }
    
    /// Gets the Cache Control Register (CACR)
    u32 getCACR() const { return reg.cacr; }
    /// Sets the Cache Control Register (CACR)
    void setCACR(u32 val);
    
    /// Gets the Cache Address Register (CAAR)
    u32 getCAAR() const { return reg.caar; }
    /// Sets the Cache Address Register (CAAR)
    void setCAAR(u32 val);

    /// Sets or clears supervisor mode
    void setSupervisorMode(bool value);
    
    /// Sets or clears master mode
    void setMasterMode(bool value);
    
    /// Sets or clear the supervisor and master flags
    void setSupervisorFlags(bool s, bool m);

    
private:
    
    // Enables or disables trace mode (T1 flag)
    void setTraceFlag() { reg.sr.t1 = true; flags |= State::TRACING; }
    void clearTraceFlag() { reg.sr.t1 = false; flags &= ~State::TRACING; }

    /* Trace on change of flow (T1T0 = 01, 68020 and later). Instructions that change the
     * program counter or modify the status register call this function. A trace exception
     * follows the instruction. If T1 is set, every instruction is traced anyway.
     * STOP is not traced. RTE is traced if T0 was set before it started (UAE only does so
     * when the T0, S or M bit changes). */
    template <Core C> void changeOfFlow()
    {
        if constexpr (C == Core::C68020) {
            if (reg.sr.t0 && !reg.sr.t1) flags |= State::TRACE_EXC;
        }
    }
    
    // Enables or disables trace mode (T0 flag)
    void setTrace0Flag() { reg.sr.t0 = true; }
    void clearTrace0Flag() { reg.sr.t0 = false; }
    
    // Disables both trace flags (T0 and T1)
    void clearTraceFlags() { clearTraceFlag(); clearTrace0Flag(); }

    
protected:
    
    // Reads a value from a data register (D0, D1 ... D7)
    template<Size S = Long> u32 readD(int n) const;
    
    // Reads a value from an address register (A0, A1 ... A7)
    template <Size S = Long> u32 readA(int n) const;
    
    // Reads a value from a register (D0, D1 ... D7, A0, A1 ... A7)
    template <Size S = Long> u32 readR(int n) const;
    
    // Writes a value to a data register (D0, D1 ... D7)
    template <Size S = Long> void writeD(int n, u32 v);
    
    // Writes a value to an address register (A0, A1 ... A7)
    template <Size S = Long> void writeA(int n, u32 v);
    
    // Writes a value to a register (D0, D1 ... D7, A0, A1 ... A7)
    template <Size S = Long> void writeR(int n, u32 v);


    //
    // Interfacing with other components
    //
    
protected:
    
#include "MoiraInterface.h"


    //
    // Instruction Analysis
    //
    
public:
    
    /// Retrieves the availability mask for a given instruction
    u16 availabilityMask(Instr I) const;
    /// Retrieves the availability mask for a given instruction
    u16 availabilityMask(Instr I, Mode M, Size S) const;
    /// Retrieves the availability mask for a given instruction
    u16 availabilityMask(Instr I, Mode M, Size S, u16 ext) const;
    
    /// Checks if a given CPU model supports a specific instruction
    bool isAvailable(CPUModel model, Instr I) const;
    /// Checks if a given CPU model supports a specific instruction
    bool isAvailable(CPUModel model, Instr I, Mode M, Size S) const;
    /// Checks if a given CPU model supports a specific instruction
    bool isAvailable(CPUModel model, Instr I, Mode M, Size S, u16 ext) const;
    
    
    //
    // Extension Word Validation
    //

private:

    // Validates extension words for a given instruction
    bool isValidExt(Instr I, Mode M, u16 op, u32 ext) const;
    
    // Validates extension words for MMU-related instructions
    bool isValidExtMMU(Instr I, Mode M, u16 op, u32 ext) const;
    
    // Validates extension words for FPU-related instructions
    bool isValidExtFPU(Instr I, Mode M, u16 op, u32 ext, CPUModel model) const;
    
    
    //
    // Disassembler Support
    //
    
private:
    
    // Returns an availability string for a given instruction
    const char *availabilityString(Instr I, Mode M, Size S, u16 ext);

    
    //
    // Loop Mode Detection
    //
    
    // Checks if an instruction is a loop mode instruction
    template <Instr I> constexpr bool looping() {
        return I >= Instr::ABCD_LOOP && I <= Instr::TST_LOOP;
    }
    
    
    //
    // Managing the Function Code Pins
    //
    
public:
    
    /// Reads the current value of the function code pins
    u8 readFC() const;
    
private:
    
    // Sets the function code pins to a specific value
    void setFC(u8 value);
    
    // Sets the function code pins based on the provided addressing mode.
    template <Mode M> void setFC();

    
    //
    // Handling Interrupts
    //
    
public:
    
    /// Retrieves the value on the Interrupt Priority Level (IPL) pins
    u8 getIPL() const { return ipl; }
    
    /// Sets the value on the Interrupt Priority Level (IPL) pins
    void setIPL(u8 val);

private:
    
    // Checks for a pending interrupt and handles it if necessary
    bool checkForIrq();

    // Selects the IRQ vector based on the interrupt level
    u16 getIrqVector(u8 level);


    //
    // Running the coprocessor interface
    //

private:

    // Announces the first cycle of the coprocessor protocol of a line-F instruction
    void notifyCPCycle(u16 opcode, u8 cir);


    //
    // Running the MMU
    //

private:

    // Extracts the function code from the extension word of an MMU instruction
    u8 mmuFC(u16 ext) const;

    // Checks whether MOVEC accepts a control register number
    bool isValidMovecReg(u16 rc) const;

    // Translates a logical address to a physical address
    template <Core C, bool write, Size S> u32 translate(u32 addr, u8 fc, bool *ci = nullptr);


private:
    
#include "MoiraInit.h"
#include "MoiraALU.h"
#include "MoiraDataflow.h"
#include "MoiraExceptions.h"
#include "MoiraDasm.h"
};

/** The jump table builders are the only place where the instruction handler
 * templates are instantiated. Keeping each core in a translation unit of its
 * own splits the instantiation work three ways and keeps the compiler's peak
 * memory usage within bounds. The definitions live in MoiraCore680x0.cpp.
 */
extern template void Moira::createJumpTable<Core::C68000>(CPUModel, bool);
/** The jump table builders are the only place where the instruction handler
 * templates are instantiated. Keeping each core in a translation unit of its
 * own splits the instantiation work three ways and keeps the compiler's peak
 * memory usage within bounds. The definitions live in MoiraCore680x0.cpp.
 */
extern template void Moira::createJumpTable<Core::C68010>(CPUModel, bool);
/** The jump table builders are the only place where the instruction handler
 * templates are instantiated. Keeping each core in a translation unit of its
 * own splits the instantiation work three ways and keeps the compiler's peak
 * memory usage within bounds. The definitions live in MoiraCore680x0.cpp.
 */
extern template void Moira::createJumpTable<Core::C68020>(CPUModel, bool);


}
