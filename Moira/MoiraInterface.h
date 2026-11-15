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
 * Each function is declared once, using one of the following macros:
 *
 *   MOIRA_REQUIRED(decl)        A function the application must implement.
 *   MOIRA_DELEGATE(decl)        An optional delegate that does nothing by default.
 *   MOIRA_DEFAULT(decl, body)   An optional function with a default implementation.
 *
 * If MOIRA_VIRTUAL_API is enabled, the macros declare virtual functions and
 * the application overrides them in a subclass. Otherwise, they declare
 * ordinary member functions that the application has to define at compile time
 * (all of them, including the optional ones). The second variant avoids the
 * cost of virtual calls.
 */

#if MOIRA_VIRTUAL_API == true

#define MOIRA_REQUIRED(decl)       virtual decl = 0;
#define MOIRA_DELEGATE(decl)       virtual decl { }
#define MOIRA_DEFAULT(decl, ...)   virtual decl { __VA_ARGS__ }

#else

#define MOIRA_REQUIRED(decl)       decl;
#define MOIRA_DELEGATE(decl)       decl;
#define MOIRA_DEFAULT(decl, ...)   decl;

#endif


//
// Clock and memory access
//

/// Advances the internal clock by the specified number of cycles
MOIRA_DEFAULT(void sync(int cycles), clock += cycles;)

/// Reads a value from memory
MOIRA_REQUIRED(u8 read8(u32 addr) const)
/// Reads a value from memory
MOIRA_REQUIRED(u16 read16(u32 addr) const)
/// Reads a value from memory
MOIRA_REQUIRED(u32 read32(u32 addr) const)

/// Reads a 16-bit value from memory during the reset routine
MOIRA_DEFAULT(u16 read16OnReset(u32 addr) const, return read16(addr);)

/// Reads a 16-bit value from memory for disassembly purposes
MOIRA_DEFAULT(u16 read16Dasm(u32 addr) const, return read16(addr);)

/// Writes a value into memory
MOIRA_REQUIRED(void write8(u32 addr, u8 val) const)
/// Writes a value into memory
MOIRA_REQUIRED(void write16(u32 addr, u16 val) const)
/// Writes a value into memory
MOIRA_REQUIRED(void write32(u32 addr, u32 val) const)

/// Returns the DSACK bits for the addressed device (68020)
MOIRA_DEFAULT(u8 dsack(u32 addr) const, return DSACK_16;)

/// Answers an interrupt acknowledge cycle
/// @param level   The acknowledged interrupt level.
/// @param vector  Receives the vector number if the answer is IrqMode::USER.
/// @return        How the interrupt is answered. AUTO (default) selects the
///                autovector 24 + level.
MOIRA_DEFAULT(IrqMode readIrqVector(u8 level, u8 *vector) const, return IrqMode::AUTO;)


//
// State delegates
//

/// Called when the CPU is reset
MOIRA_DELEGATE(void cpuDidReset())

/// Called when the CPU enters the HALT state
MOIRA_DELEGATE(void cpuDidHalt())

/// Called when the MMU is enabled
MOIRA_DELEGATE(void mmuDidEnable())

/// Called when the MMU is disabled
MOIRA_DELEGATE(void mmuDidDisable())


//
// Instruction delegates
//

/// Called before an instruction is executed
MOIRA_DELEGATE(void willExecute(const char *func, Instr I, Mode M, Size S, u16 opcode))

/// Called before the bus is used to read or write a data operand
/// @note  Only called if the option MOIRA_REPORT_BUS_ACCESS is enabled.
MOIRA_DELEGATE(void willAccessBus(u32 addr, Size size, bool write))

/// Called after an instruction has been executed
MOIRA_DELEGATE(void didExecute(const char *func, Instr I, Mode M, Size S, u16 opcode))


//
// Exception delegates
//

/// Called before an exception is executed
MOIRA_DELEGATE(void willExecute(M68kException exc, u16 vector))

/// Called after an exception has been executed
MOIRA_DELEGATE(void didExecute(M68kException exc, u16 vector))

/// Called when an interrupt is about to be processed
MOIRA_DELEGATE(void willInterrupt(u8 level))

/// Called when the CPU jumps to an exception vector
MOIRA_DELEGATE(void didJumpToVector(int nr, u32 addr))

/// Called before a CPU space cycle (FC = 7) is executed
/// @param type  The cycle type
/// @param arg   The interrupt level (IRQ_ACK), the breakpoint number
///              (BREAKPOINT), or the coprocessor ID in bits 7-5 and the
///              register select (CIR::XXX) in bits 4-0 (COPROCESSOR). This
///              is the same value that appears on the address lines
///              A15-A13 and A4-A0.
/// @note  The function code pins still show the value of the previous
///        cycle (getFC()). The cycle type implies FC = 7. For interrupt
///        acknowledge cycles, the notification precedes readIrqVector().
MOIRA_DELEGATE(void willExecute(CpuSpaceType type, u8 arg))

/// Called after a CPU space cycle has been executed
/// @note  For interrupt acknowledge cycles, readIrqVector() has been
///        called at this point.
MOIRA_DELEGATE(void didExecute(CpuSpaceType type, u8 arg))


//
// Cache register delegates
//

/// Called when the CACR register is modified
MOIRA_DELEGATE(void didChangeCACR(u32 value))

/// Called when the CAAR register is modified
MOIRA_DELEGATE(void didChangeCAAR(u32 value))


//
// Debugger delegates
//

/// Called when a soft stop is reached
MOIRA_DELEGATE(void didReachSoftstop(u32 addr))

/// Called when a breakpoint is hit
MOIRA_DELEGATE(void didReachBreakpoint(u32 addr))

/// Called when a watchpoint is hit
MOIRA_DELEGATE(void didReachWatchpoint(u32 addr))

/// Called when a catchpoint is hit
MOIRA_DELEGATE(void didReachCatchpoint(u8 vector))

/// Called when a software trap is hit
MOIRA_DELEGATE(void didReachSoftwareTrap(u32 addr))

/// Called after an instruction has been appended to the log buffer
MOIRA_DELEGATE(void didLogInstruction(const Registers &reg))

#undef MOIRA_REQUIRED
#undef MOIRA_DELEGATE
#undef MOIRA_DEFAULT
