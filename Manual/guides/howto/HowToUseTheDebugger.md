# Using the Debugger

Moira comes with a built-in debugger. It supports:

- Breakpoints
- Watchpoints
- Catchpoints
- Single-stepping
- Software traps
- An instruction log

The debugger is a public member of the CPU, `cpu.debugger`.

## How the Debugger Reports Events

The debugger never stops the CPU on its own. When an event occurs, it calls a delegate of the CPU class, and your application decides what to do. A typical debugger front end sets a flag in the delegate and leaves its run loop after the current instruction:

```c++
class CPU : public Moira {

public:

    bool stop = false;

protected:

    void didReachBreakpoint(u32 addr) override { stop = true; }
    void didReachSoftstop(u32 addr) override { stop = true; }
};

void Computer::run()
{
    cpu.stop = false;
    while (!cpu.stop) cpu.execute();
}
```

All delegates are optional. They have empty default implementations, so you only override the ones you need:

| Delegate | Called when... |
|:---------|:---------------|
| `didReachBreakpoint(addr)` | ...the next instruction to be executed is at a breakpoint address. |
| `didReachWatchpoint(addr)` | ...the CPU is about to access a watched address. |
| `didReachCatchpoint(vector)` | ...the CPU has taken an exception that is caught. |
| `didReachSoftstop(addr)` | ...a single step has completed. |
| `didReachSoftwareTrap(addr)` | ...the CPU has hit a software trap. |
| `didLogInstruction(reg)` | ...an instruction has been recorded in the log. |

## Breakpoints, Watchpoints and Catchpoints

The three kinds of guards are managed by three lists with the same interface:

- `cpu.debugger.breakpoints` observes the program counter.
- `cpu.debugger.watchpoints` observes memory accesses.
- `cpu.debugger.catchpoints` observes exceptions.

A guard is set at an address and is identified either by this address or by its position in the list:

```c++
auto &bp = cpu.debugger.breakpoints;

bp.setAt(0xFC00D2);         // Set a breakpoint
bp.disableAt(0xFC00D2);     // Keep it in the list, but don't trigger
bp.enableAt(0xFC00D2);      // Activate it again
bp.removeAt(0xFC00D2);      // Delete it
```

The most important functions are:

| Function | Description |
|:---------|:------------|
| `setAt(addr, ignores)` | Adds a guard. The optional second argument skips the first hits. |
| `removeAt(addr)`, `remove(nr)`, `removeAll()` | Deletes guards. |
| `enableAt(addr)`, `disableAt(addr)`, `toggleAt(addr)` | Activates or deactivates a guard. |
| `enableAll()`, `disableAll()` | Activates or deactivates all guards. |
| `ignore(nr, count)` | Lets a guard skip its next hits. |
| `isSetAt(addr)`, `isEnabledAt(addr)`, `isDisabledAt(addr)` | Queries the state of a guard. |
| `elements()`, `guardNr(nr)`, `guardAddr(nr)` | Iterates over the list, e.g. to display it. |

When a guard triggers, a copy of it is stored in the list's `hit` member. Empty lists cost no time during emulation, because the CPU only checks the lists that contain guards.

### Breakpoints

A breakpoint triggers when the CPU is about to execute the instruction at its address. `didReachBreakpoint()` is called at the end of `execute()`, after the previous instruction has completed. When your application calls `execute()` again, the instruction at the breakpoint is executed and the breakpoint doesn't trigger a second time.

### Watchpoints

A watchpoint triggers when the CPU accesses its address, before the access is carried out. A watchpoint at an address also catches word and long word accesses that cover it. If the MMU is enabled, watchpoints are compared with physical addresses (see [Using the Memory Management Unit](/guides/howto/HowToUseTheMmu)).

### Catchpoints

A catchpoint is set for an exception vector number, not for a memory address. It triggers when the CPU takes this exception, after it has stacked the frame and jumped to the handler. Catchpoints are useful for debugging error conditions, such as address errors (vector 3), bus errors (vector 2) or certain interrupts. `Debugger::vectorName()` returns a readable name for a vector number, e.g. for display in a user interface:

```c++
cpu.debugger.catchpoints.setAt(3);                  // Catch address errors
printf("%s\n", Debugger::vectorName(3).c_str());    // "Address error"
```

## Single-Stepping

Two functions implement single-stepping. Both set a *soft stop*, which is deleted automatically when it is reached. When it is reached, the CPU calls `didReachSoftstop()`.

- `void stepInto()`

  Stops before the next instruction is executed. If the current instruction is a subroutine call, the next stop is the first instruction of the subroutine.

- `void stepOver()`

  Stops at the instruction that follows the current one in memory. A subroutine call is executed as a whole. Note that the stop is never reached if the current instruction branches elsewhere and doesn't return.

## Software Traps

A software trap lets your application intercept the execution of code at a specific location, e.g. to hook a routine in ROM, without setting a breakpoint. The trap replaces an instruction with a special line A opcode (0xA000 - 0xAFFF) that Moira recognizes:

```c++
u16 original = read16(addr);
u16 trap = cpu.debugger.swTraps.create(original);   // Pick an unused trap opcode
write16(addr, trap);                                // Install it
```

When the CPU executes the trap opcode, it calls `didReachSoftwareTrap()` and then executes the original instruction. The trap stays in place and fires again the next time the code runs. To remove it, write the original instruction back. `swTraps.resolve(opcode)` returns the original instruction of a trap opcode, which is useful when displaying memory.

## The Instruction Log

The debugger can record the register state before each instruction is executed. The log is a ring buffer that holds the last 256 instructions:

| Function | Description |
|:---------|:------------|
| `enableLogging()`, `disableLogging()` | Turns logging on and off. |
| `loggedInstructions()` | Returns the number of entries (at most 256). |
| `logEntryRel(n)` | Returns an entry, counted from the newest one (0 is the most recent). |
| `logEntryAbs(n)` | Returns an entry, counted from the oldest one (0 is the oldest). |
| `clearLog()` | Empties the log. |
| `disassembleRecorded(fmt, n)` | Disassembles a logged instruction with the recorded register values. |
| `dumpLogBuffer(os, count)` | Prints the last `count` entries. |

For example, to see how a program ended up in an exception handler, enable logging, set a catchpoint, and print the log when the catchpoint triggers:

```c++
cpu.debugger.enableLogging();
cpu.debugger.catchpoints.setAt(3);

void CPU::didReachCatchpoint(u8 vector)
{
    debugger.dumpLogBuffer(std::cout, 10);      // The last 10 instructions
}
```

Each entry is a `Registers` structure with the program counter, the status register and all data and address registers at the start of the instruction. If `didLogInstruction()` is overridden, it receives each entry as it is recorded, e.g. to write a complete trace to a file.
