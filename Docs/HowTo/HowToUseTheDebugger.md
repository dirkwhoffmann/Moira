# How to use the debugger

Moira comes with built-in support for the following debugging features: 
- Breakpoints
- Watchpoints
- Catchpoints
- Software traps
- Instructions log

All API calls to the debugger are handled through the `debugger` object which is a public member of the Moira class.

## Managing Breakpoints

If one or more breakpoints are set, Moira starts monitoring the program counter. As soon as one of the monitored memory addresses is reached, function `breakpointReached` is invoked. This function has an empty function body by default and needs to be overwritten in your sub-class. 

All API calls related to breakpoints are handled through the `breakpoints` object which is a public member of the `debugger` object.

- `void addAt(u32 addr)`
    
    Adds a breakpoint at the specified address.

- `void addAt(u32 addr)`

   Removes the breakpoint at the specified address.

- `void disableAt(u32 addr)`

    Disables the breakpoint at the specified address. A disabled breakpoint is kept in the breakpoint list, but will never trigger.

- `void enableAt(u32 addr)`

    Reenables the breakpoint at the specified address. 

- `void stepInto()`

   This function sets a breakpoint on the instruction that is going to be executed next. The breakpoint is *soft* which means that it is deleted automatically when reached. 

- `void stepOver()`

This function sets a soft breakpoint on the instruction that is next to the currently executed instruction in memory. Note: If the current instruction performs a jump, this instruction might never be reached. 

## Watchpoints

If one or more watchpoints are set, Moira monitors all reads and writes from or to memory. As soon as one of the monitored addresses is accessed, function `watchpointReached` is invoked. This function has an empty function body by default and needs to be overwritten in your sub-class. 

All API calls related to watchpoints are handled through the `watchpoints` object which is a public member of the debugger object.

- `void addAt(u32 addr)`
    
    Adds a watchpoint at the specified address.

- `void removeAt(u32 addr)`

    Removes the watchpoint at the specified address.

- `void disableAt(u32 addr)`

    Disables the watchpoint at the specified address. A disabled watchpoint is kept in the watchpoint list, but can never trigger.

- `void reableAt(u32 addr)`

    Reenables the watchpoint at the specified address. 

## Catchpoints

If one or more catchpoints are set, Moira monitors the exception vector table. As soon as one of the monitored exceptions is thrown, function `catchpointReached` is invoked. This function has an empty function body by default and needs to be overwritten in your sub-class. Catchpoints are a handy debug tool. They can be used to intercept certain interrupts or certain error conditions such as address errors or bus errors. 

All API calls related to catchpoints are handled through the `catchpoints` object which is a public member of the debugger object.

- `void addAt(u32 addr)`
    
    Adds a catchpoint at the specified address.

- `void removeAt(u32 addr)`

    Removes the catchpoint at the specified address.

- `void disableAt(u32 addr)`

    Disables the catchpoint at the specified address. A disabled catchpoint is kept in the catchpoint list, but can never trigger.

- `void reableAt(u32 addr)`

    Reenables the catchpoint at the specified address. 

## Software traps

Software traps are a flexible mechanism to interrupt execution in a relocatable way. If a software trap is set at a specific address, Moira replaces the instruction by a special LINEA instruction. When this instruction is executed, which is independet of it's current address, Moira signals the occurance of a software trap. Once reached, the trap is removed by putting back the original instruction.

- `u16 create(u16 instr)`

    Places a software trap at the specified address.

## Logging

Moira provides a special log mode for recording the instruction stream. If logging is enabled, the contents of all registers is recorded prior to the execution of an instruction. The logged information is stored inside a *log buffer* which is a ring buffer whose capacity is currently hard-coded to 256 elements. 

All API calls of the logging facility are handled through the `debugger` object.

- `void enableLogging()`

    Turns logging on. 

- `void disableLogging()`

    Turns logging off. 
    
- `int loggedInstructions() const`

    Returns the number of records in the log buffer. If the emulator has run for a few cycles, the return value matches the log buffer capacity and does not change thereafter. 

- `void clearLog()`

    Start over with a clean log buffer.

- `const Registers logEntryAbs(n) const`

    Read an entry from the log buffer where *n = 0* specifies the most recently recorded entry. 

- `debugger.logEntryAbs(n) const`

    Same as `logEntry(n)` with reversed addressing. When called with *n = 0*, the oldest record will be read from the buffer.

