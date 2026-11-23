# The Main Execution Function

This article explains what happens when `Moira::execute()` is called. The function runs a single instruction, processes a pending exception, or lets time pass if the CPU is halted or stopped. It returns the number of elapsed cycles.

The function is the hot spot of the emulator. It is called millions of times per second, so it is designed to do as little as possible in the common case. Its structure looks as follows:

```c++
i64
Moira::execute()
{
    const auto start = clock;

    // Take the fast path or the slow path
    if (!flags) {

        //
        // Fast path
        //

        try {
            (this->*exec[queue.ird])(queue.ird);
        } catch (const std::exception &exc) {
            processException(exc);
        }

        advancePC0();

    } else {

        //
        // Slow path
        //

        ...
    }

    return clock - start;
}
```

## The State Flags

The decision between the two paths is based on a single variable, `flags`. Each bit stands for a condition that requires special treatment:

| Flag | Meaning |
|:-----|:--------|
| `HALTED` | The CPU has halted after a double fault. Only a reset leaves this state. |
| `STOPPED` | The CPU has executed a `STOP` instruction and waits for an interrupt. |
| `LOOPING` | The 68010 is in loop mode. |
| `LOGGING` | The debugger records the instructions. |
| `TRACING` | The T1 bit of the status register is set. |
| `TRACE_EXC` | A trace exception is due. |
| `CHECK_IRQ` | An interrupt may be pending. |
| `CHECK_BP`, `CHECK_WP`, `CHECK_CP` | The debugger has breakpoints, watchpoints or catchpoints. |
| `SAVE_REGS` | The registers have to be saved, so that a faulted instruction can be restarted. |
| `MMU` | Address translation is active (which also requires the registers to be saved). |

The flags mirror information that is stored elsewhere in a form that is cheap to test. For example, `TRACING` reflects the T1 bit of the status register, and `CHECK_IRQ` is only set if the interrupt level on the pins is higher than the interrupt mask. In most cases, no flag is set, and `execute()` takes the fast path.

## The Fast Path

On the fast path, Moira calls the handler of the next instruction. The opcode is already in `queue.ird`, because the CPU has prefetched it. The handler is looked up in the jump table `exec`, which has one entry for each of the 65536 opcodes (see [Instruction Handlers](/guides/concepts/InstructionHandlers)).

The call is enclosed in a try-catch block, because exceptions such as address errors or bus errors are signaled with C++ exceptions. If the handler throws, for example an `AddressError` or a `BusError`, `processException()` writes the stack frame and jumps to the exception handler.

Afterwards, `advancePC0()` updates the address of the current instruction (`reg.pc0`). It now points to the instruction that will be executed next, and the address of the previous one is kept in `reg.prev_pc0`.

## The Slow Path

If at least one flag is set, Moira takes the slow path. It checks the flags one by one, in the order in which the real CPU processes the corresponding conditions:

```c++
        // A halted CPU only lets time pass
        if (flags & HALTED) { sync(2); return clock - start; }

        // Process a pending trace exception
        if (flags & TRACE_EXC) { execException(M68kException::TRACE); goto done; }

        // Request a trace exception for the next instruction
        if ((flags & TRACING) && !(flags & STOPPED)) flags |= TRACE_EXC;

        // Process a pending interrupt
        if (flags & CHECK_IRQ) { if (checkForIrq()) goto done; }

        // A stopped CPU only polls the interrupt lines
        if (flags & STOPPED) { POLL_IPL; sync(2); return clock - start; }

        // Record the instruction in the log
        if (flags & LOGGING) debugger.logInstruction();

        // Execute the instruction (see below)
        ...

    done:

        advancePC0();

        // Check the debugger's soft stops and breakpoints
        if (flags & CHECK_BP) {

            if (flags & TRACE_EXC) return clock - start;
            if (debugger.softstopMatches(reg.pc0)) didReachSoftstop(reg.pc0);
            if (debugger.breakpointMatches(reg.pc0)) didReachBreakpoint(reg.pc0);
        }
```

The instruction is executed like on the fast path, with two differences:

- **Loop mode.** If `LOOPING` is set, the handler is taken from the table `loop` instead of `exec`. Loop mode is a feature of the 68010: A loop that consists of a single instruction and a `DBcc` is executed without fetching the instructions again. The 68020 dropped loop mode, because its instruction cache makes it obsolete.

- **Saving the registers.** If `SAVE_REGS` or `MMU` is set, the registers are saved before the handler is called. If the instruction faults with a bus error, the saved state is passed to `processException()`, which needs it to restart the instruction after the exception handler returns (see [Emulating Bus Errors](/guides/howto/HowToEmulateBusErrors)). `SAVE_REGS` is set on the 68020 and later if `MOIRA_EMULATE_BUS_ERROR` is enabled, `MMU` while address translation is active. Because the registers are saved before every instruction, these models stay on the slow path in this configuration.

The breakpoints are checked after the instruction, against the address of the next one. Thus, the debugger delegates are called before the instruction at a breakpoint is executed. If a trace exception is due, the check is skipped, because the trace exception is processed first.

## Watchpoints and Catchpoints

Watchpoints and catchpoints are not checked in `execute()`. A watchpoint concerns a memory access, so it is checked by the memory access functions when `CHECK_WP` is set. A catchpoint concerns an exception, so it is checked when the CPU jumps to an exception vector.
