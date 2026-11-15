// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* The interface between the CPU core and the application
 *
 * This file is included inside the class body of Moira. It declares the
 * functions the application provides: The memory access functions and the
 * clock synchronization function, which are mandatory, and a set of optional
 * delegates that inform the application about events.
 *
 * If MOIRA_VIRTUAL_API is enabled, all functions are virtual and the
 * application overrides them in a subclass. Otherwise, the functions are
 * ordinary member functions that the application has to define at compile time.
 * The second variant avoids the cost of virtual calls. Both variants declare the
 * same set of functions. Keep them in sync.
 */

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

    /// Answers an interrupt acknowledge cycle
    /// @param level   The acknowledged interrupt level.
    /// @param vector  Receives the vector number if the answer is IrqMode::USER.
    /// @return        How the interrupt is answered. AUTO (default) selects the
    ///                autovector 24 + level.
    virtual IrqMode readIrqVector(u8 level, u8 *vector) const { return IrqMode::AUTO; }

    
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
    
    /// Called before the bus is used to read a data operand
    virtual void willAccessBus(u32 addr, Size size, bool write) { }

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

    /// Called before a CPU space cycle (FC = 7) is executed
    /// @param type  The cycle type
    /// @param arg   The interrupt level (IRQ_ACK), the breakpoint number
    ///              (BREAKPOINT), or the coprocessor ID in bits 7-5 and the
    ///              register select (CIR::XXX) in bits 4-0 (COPROCESSOR). This
    ///              is the same value that appears on the address lines
    ///              A15-A13 and A4-A0.
    /// @note  The function code pins still show the value of the previous
    ///        cycle (readFC()). The cycle type implies FC = 7. For interrupt
    ///        acknowledge cycles, the notification precedes readIrqVector().
    virtual void willExecute(CpuSpaceType type, u8 arg) { }

    /// Called after a CPU space cycle has been executed
    /// @note  For interrupt acknowledge cycles, readIrqVector() has been
    ///        called at this point.
    virtual void didExecute(CpuSpaceType type, u8 arg) { }

    
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

    // Called before and after a CPU space cycle (FC = 7)
    void willExecute(CpuSpaceType type, u8 arg);
    void didExecute(CpuSpaceType type, u8 arg);
    
    
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
