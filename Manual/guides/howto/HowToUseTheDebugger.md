# Using the Debugger

Moira comes with built-in support for the following debugging features: 
- Breakpoints
- Watchpoints
- Catchpoints
- Software traps
- Instructions log

All API calls related to debugging are handled through the debugger object, which is a public member of the Moira class.

## Breakpoints

When breakpoints are set, Moira monitors the program counter. If execution reaches one of the specified addresses, the function `didReachBreakpoint` is called. By default, this function has an empty implementation and must be overridden in a subclass.

All breakpoint-related API calls are managed through the `breakpoints` object, which is a public member of the `debugger` object. 

- `void setAt(u32 addr)`
    
    Sets a breakpoint at the specified address.

- `void removeAt(u32 addr)`

   Removes the breakpoint at the specified address.

- `void disableAt(u32 addr)`

    Disables the breakpoint at the specified address. A disabled breakpoint is kept in the breakpoint list, but will never trigger.

- `void enableAt(u32 addr)`

    Reenables the breakpoint at the specified address. 

- `void stepInto()`

   This function sets a breakpoint on the instruction that is going to be executed next. The breakpoint is *soft*, that is, it is deleted automatically when reached. 

- `void stepOver()`

This function sets a soft breakpoint on the instruction next to the currently executed instruction in memory. Note: If the current instruction jumps, this instruction might never be reached. 

## Watchpoints

Watchpoints allow monitoring of memory reads and writes. If a specified address is accessed, the function `didReachWatchpoint` is called. Like 'didReachBreakpoint`, this function is empty by default and must be overridden in a subclass.

All API calls related to watchpoints are managed by the `watchpoints` object which is a public member of the debugger object.

- `void setAt(u32 addr)`
    
    Sets a watchpoint at the specified address.

- `void removeAt(u32 addr)`

    Removes the watchpoint at the specified address.

- `void disableAt(u32 addr)`

    Disables the watchpoint at the specified address. A disabled watchpoint is kept in the watchpoint list, but never triggers.

- `void reableAt(u32 addr)`

    Reenables the watchpoint at the specified address. 

## Catchpoints

Catchpoints provide a way to intercept exceptions by monitoring the exception vector table. When a monitored exception executes, Moira calls `catchpointReached`, which must be implemented in a subclass. Catchpoints are a useful tool for debugging error conditions such as address errors, bus errors, or certain interrupts.
All catchpoint-related API calls are managed by the `catchpoints` object inside the debugger.

- `void setAt(u32 addr)`
    
    Sets a catchpoint at the specified address.

- `void removeAt(u32 addr)`

    Removes the catchpoint at the specified address.

- `void disableAt(u32 addr)`

    Disables the catchpoint at the specified address. A disabled catchpoint is kept in the catchpoint list, but never triggers.

- `void reableAt(u32 addr)`

    Reenables the catchpoint at the specified address. 

## Software Traps

Software traps are a flexible mechanism for interrupting execution in a relocatable manner. When a trap is set at a particular address, Moira replaces the instruction with a special LINEA instruction. If this instruction is executed, Moira signals that a software trap has occurred. Once triggered, the trap is automatically removed, restoring the original instruction at that address. This mechanism is particularly useful for setting temporary execution stops without modifying program logic permanently.

- `u16 create(u16 instr)`

    Places a software trap at the specified address.

## Logging

Moira includes an instruction logging system that records register states before each instruction executes. Logged entries are stored in a circular buffer with a fixed capacity of 256 entries. 
All logging-related API calls are handled through the `debugger` object. 

- `void enableLogging()`

    Turns logging on. 

- `void disableLogging()`

    Turns logging off. 
    
- `int loggedInstructions() const`

    Returns the number of log buffer entries. Once the circular buffer has filled up, the return value matches the log buffer capacity and does not change thereafter.

- `void clearLog()`

    Starts over with a clean log buffer.

- `const Registers logEntryAbs(n) const`

    Reads an entry from the log buffer where *n = 0* specifies the most recently recorded entry. 

- `debugger.logEntryAbs(n) const`

    Same as `logEntry(n)` with reversed enumeration order. Called with *n = 0*, the oldest record will be read from the buffer.

