# Using the Delegation System

The Moira class defines various delegation functions that are triggered on specific events. These functions are declared as virtual and have empty default implementations. A subclassed CPU can override them to react to the corresponding events.

## State Delegates

The following functions are called when the CPU enters specific states:

- `void cpuDidReset()`

    Invoked at the end of the `reset()` routine. 

- `void cpuDidHalt()`

    Called when the CPU is halted, e.g., after a double-fault.

## Instruction Delegates

These functions are invoked at the start and end of selected instruction handlers:

- `void willExecute(const char *func, Instr I, Mode M, Size S, u16 opcode)`

    Called at the beginning of selected instruction handlers. The instructions are defined by the `WILL_EXECUTE` macro in `MoiraConfig.h`. The default implementation looks as follows:
    ````c++
    #define MOIRA_WILL_EXECUTE I == Instr::STOP || I == Instr::TAS || I == Instr::BKPT
    `````

- `void didExecute(const char *func, Instr I, Mode M, Size S, u16 opcode)`

    Called at the end of selected instruction handlers. The instructions are defined by the `DID_EXECUTE` macro in MoiraConfig.h. The default implementation looks as follows:
    ````c++
    #define MOIRA_DID_EXECUTE I == Instr::RESET
    `````

## Exception Delegates

- `void willExecute(ExceptionType exc, u16 vector)`

    Called at the start of exception processing. 

- `void didExecute(ExceptionType exc, u16 vector)`

    Called at the end of exception processing.

- `void willInterrupt(u8 level)`

    Called at the start of interrupt processing.

- `void didJumpToVector(int nr, u32 addr)`

    Called after the program counter jumps to the exception handler. 

## Cache Register Delegates (68020 Only)

These functions track changes to cache-related registers:

- `void didChangeCACR(u32 value)`

    Called when the CACR (Cache Control Register) is modified. 

- `void didChangeCAAR(u32 value)`

    Called when the CAAR (Cache Address Register) is modified.

## Debugger Delegates

These functions signal debug events:

- `void didReachSoftstop(u32 addr)`

   Called when a soft stop is reached.
    
- `void didReachBreakpoint(u32 addr)`

   Called when a breakpoint is hit.
           
- `void didReachWatchpoint(u32 addr)`
    
   Called when a watchpoint is hit.
        
- `void didReachCatchpoint(u8 vector)`
    
   Called when a catchpoint is hit.
        
- `void didReachSoftwareTrap(u32 addr)`

   Called when a software trap is hit.
