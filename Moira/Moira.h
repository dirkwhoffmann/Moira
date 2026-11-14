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

inline constexpr int major = 5;
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
    CopModel copModel = CopModel::NONE;

    // Start address of the most recently executed instruction (stacked by the 68020 trace frame)
    u32 pct = 0;

    /* Restart information of a faulted instruction (68020 in UAE mode). The handler
     * sees the registers as they were when the fault occurred (as on the real CPU,
     * which continues from internal state). If it returns without touching the frame,
     * RTE rolls the instruction back, so that it is executed again from its start.
     */
    struct Restart {

        bool valid = false;
        Registers pre;          // Registers at the start of the instruction
        Registers fault;        // Registers at the time of the fault
        u16 preSr = 0;          // Status register at the start of the instruction
        u32 pc = 0;             // Start of the instruction
        u32 frameSp = 0;        // Location of the stacked frame
        u16 frameSr = 0;        // Stacked status register
        u32 framePC = 0;        // Stacked program counter
        u32 entrySsp = 0;       // Supervisor stack pointer before the frame was stacked
        u32 entryUsp = 0;       // User stack pointer (while in supervisor mode)
    } restart;

    // The value of the most recent write access (reported in bus fault frames)
    u32 busWriteData = 0;
    u8 splitHalf = 0;               // Set while the second half of a split long word access is performed (used by the tests)
    bool busWriteAccess = false;    // Is the current bus access a write? (68020 bus error frames)
    
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
    
    /// Debugger handling breakpoints, watchpoints, catchpoints, and instruction tracing
    Debugger debugger = Debugger(*this);

    
    
    //
    // Internals
    //
    
protected:
    
    // Number of elapsed cycles since power-up
    i64 clock {};
    
    // The CPU's register set
    Registers reg {};

    // The memory management unit
    MMU mmu = MMU(*this);

    // The floating-point unit
    FPU fpu {};
    
    // Prefetch queue for fetching instructions
    PrefetchQueue queue {};

    // Instruction cache (68020 and up) and data cache (68030 and up)
    ICache iCache = ICache(*this);
    DCache dCache = DCache(*this);

    // Indicates that the data cache is enabled
    bool dCacheOn = false;

    // Speed trackers (68020 only)
    CycleBudget budget {};

    // Interrupt mode
    IrqMode irqMode {IrqMode::AUTO};
    
    // Current value on the IPL (Interrupt Priority Level) pins
    u8 ipl {};

    // Level 7 is edge triggered. This flag is set when the level 7 interrupt has been taken and is cleared when the pins leave level 7.
    bool nmiTaken {};
    
    // Value on the lower two function code pins (FC1|FC0)
    u8 fcl {2};
    
    // Source of the function code pins
    u8 fcSource {};
    
    // Remembers the vector number of the most recent exception
    int exception {};

    // Controls exact timing of instructions running in loop mode (68010 only)
    int loopModeDelay {2};
    
    // Read buffer (appears in 68010 exception frames)
    u16 readBuffer {};
    
    // Write buffer (appears in 68010 exception frames)
    u16 writeBuffer {};
    
    // State flags used internally
    int flags {};
    
    
    //
    // Lookup tables
    //
    
private:
    
    // Jump table holding the instruction handlers
    typedef void (Moira::*ExecPtr)(u16);
    ExecPtr *exec = nullptr;
    
    // Jump table holding the loop mode instruction handlers (68010 only)
    ExecPtr *loop = nullptr;
    
    /* Jump table holding the disassembler handlers.
     *
     * Since the handlers are not templated, the instruction, addressing mode
     * and size attributes that used to be template arguments are passed in at
     * call time. They are read from the InstrInfo table, which is built
     * alongside this one.
     */
    typedef void (Moira::*DasmPtr)(StrWriter&, u32&, u16, Instr, Mode, Size) const;
    DasmPtr *dasm = nullptr;
    
    // Table holding instruction information
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

    // Rolls the registers back to the state saved at the start of the instruction
    void restoreRegisters(const Registers &saved);

    // Takes back the predecrement of the operand that caused a bus error
    void undoPredecrement(u32 addr, const Registers &saved, u16 ird);

    // Reads a program word without raising a bus error (returns 0 if the word is unreachable)
    u16 peekProgramWord(u32 addr, bool supervisor);

    // Determines the effective FPU and MMU from the CPU model and the selection
    void updateUnits();
    void createJumpTable(CPUModel model) { createJumpTable(model, model); }
    
private:
    
    /* Core routines for creating jump tables
     *
     * Jump table creation is deliberately split into several parts.
     * As a single function it expands into several hundred kilobytes of
     * straight-line code per core, which drives the compiler's memory
     * consumption far beyond what some environments provide.
     */
    
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
     * setCopModel does not matter.
     */
    void setCopModel(CopModel model);

    /// Synchronizes internal state with the MMU registers. Needs to be called
    /// if the translation control register is modified without setTC.
    void updateMmuFlag();

    /// Returns the selected coprocessor (which may not be effective, see setCopModel)
    CopModel getCopModel() const { return copModel; }

    /// Checks if a model provides a coprocessor interface (to attach a 6888x)
    static bool hasCoproInterface(CPUModel model);

    /// Checks if the FPU is built into the CPU (68040)
    bool isInternalFpu() const;

    /// Checks if an external FPU (68881 or 68882) is attached. This requires a
    /// CPU with a coprocessor interface and a coprocessor model other than NONE.
    bool isExternalFpu() const;
    
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
    
    // Helper function
    void setNumberFormat(DasmStyle &style, const DasmNumberFormat &value);
    
    
    //
    // Querying CPU properties
    //
    
public:
    
    /// Checks if the emulated CPU model has a coprocessor interface
    bool hasCPI() const;
    
    /// Checks if the emulated CPU model has a memory management unit (MMU)
    bool hasMMU() const;
    /// Checks if a given CPU model has a memory management unit (MMU)
    static bool hasMMU(CPUModel model);
    
    /// Checks if the emulated system has a floating-point unit (built-in or external)
    bool hasFPU() const;
    
    /// Returns the cache register mask, indicating the accessible CACR bits
    u32 cacrMask() const;
    
    /// Returns the address bus mask, which defines the CPU's addressable memory range
    u32 addrMask() const;
    
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
    
    // Processes an exception that was caught during execution. If the exception
    // was raised inside an instruction whose start state has been saved, saved
    // points to that state (nullptr otherwise).
    void processException(const std::exception &exception, const Registers *saved = nullptr);
    
    // Processes an exception for a specific CPU core type
    template <Core C> void processException(const std::exception &exception, const Registers *saved = nullptr);
    
    // Performs a core-specific reset routine
    template <Core C> void reset();
    
    // Checks for a pending interrupt and handles it if necessary
    bool checkForIrq();
    
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
    // Advancing the clock
    //

protected:

    /* Advances the clock by the cycle count computed for a 68020 instruction.
     *
     * Instruction fetches that hit the cache are modelled as a discount (see
     * readInstructionWord), so the total can drop to zero or below, while the
     * environment can only be stepped in whole bus cycles. The time is
     * therefore accumulated and spent as soon as it amounts to at least one
     * bus cycle, which keeps the discount fully effective. Should that not
     * happen for CpStallMax instructions in a row, one bus cycle is borrowed:
     * the environment is stepped from here alone, so without it the emulation
     * would make no progress and its main loop would never terminate.
     */
    void syncCp(int cycles);


    //
    // Interfacing with other components
    //
    
protected:
    
#if MOIRA_VIRTUAL_API == true
    
    /// Advances the internal clock by the specified number of cycles
    virtual void sync(int cycles) { clock += cycles; }
    
    /// Reads a value from memory
    virtual u8 read8(u32 addr) const = 0;
    /// Reads a value from memory
    virtual u16 read16(u32 addr) const = 0;
    /// Reads a value from memory
    virtual u32 read32(u32 addr) const = 0;

    /// Reads a 16-bit value from memory during the reset routine
    virtual u16 read16OnReset(u32 addr) const { return read16(addr); }

    /// Reads a 16-bit value from memory for disassembly purposes
    virtual u16 read16Dasm(u32 addr) const { return read16(addr); }

    /// Writes a value into memory
    virtual void write8(u32 addr, u8 val) const = 0;
    /// Writes a value into memory
    virtual void write16(u32 addr, u16 val) const = 0;
    /// Writes a value into memory
    virtual void write32(u32 addr, u32 val) const = 0;

    /// Returns the DSACK bits for the addressed device (68020)
    virtual u8 dsack(u32 addr) const { return DSACK_16; }

    /// Provides the interrupt vector for a given interrupt level in USER mode
    virtual u16 readIrqUserVector(u8 level) const { return 0; }

    
    //
    // State delegates
    //
    
    /// Called when the CPU is reset
    virtual void cpuDidReset() { }
    
    /// Called when the CPU enters the HALT state
    virtual void cpuDidHalt() { }

    /// Called when the MMU is enabled or disabled
    virtual void mmuDidEnable() { }
    /// Called when the MMU is enabled or disabled
    virtual void mmuDidDisable() { }
    
    
    //
    // Instruction delegates
    //
    
    /// Called before an instruction is executed
    virtual void willExecute(const char *func, Instr I, Mode M, Size S, u16 opcode) { }
    
    /// Called after an instruction has been executed
    virtual void didExecute(const char *func, Instr I, Mode M, Size S, u16 opcode) { }
    
    
    //
    // Exception delegates
    //
    
    /// Called before an exception is executed
    virtual void willExecute(M68kException exc, u16 vector) { }
    
    /// Called after an exception has been executed
    virtual void didExecute(M68kException exc, u16 vector) { }
    
    /// Called when an interrupt is about to be processed
    virtual void willInterrupt(u8 level) { }
    
    /// Called when the CPU jumps to an exception vector
    virtual void didJumpToVector(int nr, u32 addr) { }

    /// Called when the CPU runs a CPU space cycle (FC = 7)
    /// @param type  The cycle type
    /// @param arg   The interrupt level (IRQ_ACK), the breakpoint number
    ///              (BREAKPOINT), or the coprocessor ID in bits 7-5 and the
    ///              register select (CIR::XXX) in bits 4-0 (COPROCESSOR). This
    ///              is the same value that appears on the address lines
    ///              A15-A13 and A4-A0.
    /// @note  readFC() returns 7 while this delegate is called. For interrupt
    ///        acknowledge cycles, the same holds for readIrqUserVector().
    virtual void cpuSpaceCycle(CpuSpaceType type, u8 arg) { }

    
    //
    // Cache register delegates
    //
    
    /// Called when the CACR register is modified
    virtual void didChangeCACR(u32 value) { }
    
    /// Called when the CAAR register is modified
    virtual void didChangeCAAR(u32 value) { }

    
    //
    // Debugger delegates
    //
    
    /// Called when a soft stop is reached
    virtual void didReachSoftstop(u32 addr) { }
    
    /// Called when a breakpoint is hit
    virtual void didReachBreakpoint(u32 addr) { }
    
    /// Called when a watchpoint is hit
    virtual void didReachWatchpoint(u32 addr) { }
    
    /// Called when a catchpoint is hit
    virtual void didReachCatchpoint(u8 vector) { }
    
    /// Called when a software trap is hit
    virtual void didReachSoftwareTrap(u32 addr) { }

    /// Called after an instruction has been appended to the log buffer
    virtual void didLogInstruction(const Registers &reg) { }

#else
    
    // Advances the internal clock by the specified number of cycles
    void sync(int cycles);
    
    // Reads a value from memory
    u8 read8(u32 addr) const;
    u16 read16(u32 addr) const;
    u32 read32(u32 addr) const;

    // Reads a 16-bit value from memory during the reset routine
    u16 read16OnReset(u32 addr) const;

    // Reads a 16-bit value from memory for disassembly purposes
    u16 read16Dasm(u32 addr) const;

    // Writes a value into memory
    void write8(u32 addr, u8 val) const;
    void write16(u32 addr, u16 val) const;
    void write32(u32 addr, u32 val) const;

    // Returns the DSACK bits for the addressed device (68020)
    u8 dsack(u32 addr) const;

    // Provides the interrupt vector for a given interrupt level in USER mode
    u16 readIrqUserVector(u8 level) const;

    
    //
    // State delegates
    //
    
    // Called when the CPU is reset
    void cpuDidReset();
    
    // Called when the CPU enters the HALT state
    void cpuDidHalt();

    // Called when the MMU is enabled or disabled
    void mmuDidEnable();
    void mmuDidDisable();
    
    
    //
    // Instruction delegates
    //
    
    // Called before an instruction is executed
    void willExecute(const char *func, Instr I, Mode M, Size S, u16 opcode);
    
    // Called after an instruction has been executed
    void didExecute(const char *func, Instr I, Mode M, Size S, u16 opcode);
    
    
    //
    // Exception delegates
    //
    
    // Called before an exception is executed
    void willExecute(M68kException exc, u16 vector);
    
    // Called after an exception has been executed
    void didExecute(M68kException exc, u16 vector);
    
    // Called when an interrupt is about to be processed
    void willInterrupt(u8 level);
    
    // Called when the CPU jumps to an exception vector
    void didJumpToVector(int nr, u32 addr);

    // Called when the CPU runs a CPU space cycle (FC = 7)
    void cpuSpaceCycle(CpuSpaceType type, u8 arg);
    
    
    //
    // Cache register delegates
    //
    
    // Called when the CACR register is modified
    void didChangeCACR(u32 value);
    
    // Called when the CAAR register is modified
    void didChangeCAAR(u32 value);
    
    
    //
    // Debugger delegates
    //
    
    // Called when a soft stop is reached
    void didReachSoftstop(u32 addr);
    
    // Called when a breakpoint is hit
    void didReachBreakpoint(u32 addr);
    
    // Called when a watchpoint is hit
    void didReachWatchpoint(u32 addr);
    
    // Called when a catchpoint is hit
    void didReachCatchpoint(u8 vector);
    
    // Called when a software trap is hit
    void didReachSoftwareTrap(u32 addr);

    // Called after an instruction has been appended to the log buffer
    void didLogInstruction(const Registers &reg);
    
#endif
    
    //
    // Accessing the clock
    //
    
public:
    
    /// Returns the current CPU clock cycle count (elapsed cycles since power-up)
    i64 getClock() const { return clock; }
    
    /// Sets the CPU clock cycle count
    void setClock(i64 val) { clock = val; }
    
    
    //
    // Accessing registers
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
    /// Sets the address of the currently executed instruction
    void setPC0(u32 val) { reg.pc0 = val; }
    
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

    
    //
    // Supervisor mode
    //
    
public:
    
    /// Sets or clears supervisor mode
    void setSupervisorMode(bool value);
    
    /// Sets or clears master mode
    void setMasterMode(bool value);
    
    /// Sets or clear the supervisor and master flags
    void setSupervisorFlags(bool s, bool m);
    
    
    //
    // Trace Flags
    //
    
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
    
    
    //
    // Register Access
    //
    
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

    /// Retrieves the way the CPU obtains the vector number of an interrupt
    IrqMode getIrqMode() const { return irqMode; }

    /// Selects how the CPU obtains the vector number of an interrupt
    /// @param mode  AUTO uses the autovector 24 + level (default). USER asks
    ///              readIrqUserVector() for the vector number. SPURIOUS always
    ///              uses vector 24 and UNINITIALIZED vector 15.
    void setIrqMode(IrqMode mode) { irqMode = mode; }

private:
    
    // Selects the IRQ vector based on the interrupt level
    u16 getIrqVector(u8 level);

    // Reports a CPU space cycle (sets the function code pins to 7 meanwhile)
    void reportCpuSpace(CpuSpaceType type, u8 arg);

    // Reports the first cycle of the coprocessor protocol of a line-F instruction
    void reportCoprocessorCycle(u16 opcode, u8 cir);


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
