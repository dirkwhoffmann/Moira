# The Main Execution Function

This article provides an overview of the inner workings of the main execution function in vAmiga. Specifically, it explains what happens when `Moira::execute()` is called.
Below is the structure of the main execution function:

```c++
void
Moira::execute()
{
    ...

    // Take the fast path or the slow path
    if (!flags) {

        //
        // Fast path
        //

        reg.pc += 2;
        try {
            (this->*exec[queue.ird])(queue.ird);
        } catch (const std::exception &exc) {
            processException(exc);
        }

    } else {
    
        //
        // Slow path
        //

        ...
    }
    ...
}
```

## The Fast Path 

The first step in execution is a check on the `flags` variable. Flags are typically set only in exceptional circumstances, so in most cases, the variable will be 0. This allows Moira to execute instructions on the fast path, skipping time-consuming tasks. In this path, the program counter (`reg.pc`) is incremented by 2, and the appropriate instruction handler is called by looking up a function pointer from the `exec` table.
The call to the instruction handler is enclosed in a try-catch block, as exceptions may be thrown during instruction execution. For instance, if an address error occurs, the handler will throw an `AddressError` exception. This exception is caught, and `processException` handles it accordingly.

## The Slow Path

When at least one bit in flags is set, Moira enters the slow path, where it evaluates each flag individually and performs special actions as needed. Below is a detailed breakdown of the slow path code:

```c++
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
        
        ... 
        
        // Execute the instruction
        reg.pc += 2;

        if (flags & LOOPING) {

            assert(loop[queue.ird]);
            (this->*loop[queue.ird])(queue.ird);

        } else {

            try {
                (this->*exec[queue.ird])(queue.ird);
            } catch (const std::exception &exc) {
                processException(exc);
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
}
```

On the slow path, the instruction handler is called after checking various conditions related to the CPU state. The CPU_IS_LOOPING flag is specifically examined to determine if the CPU is in loop mode. If the flag is set, Moira uses a special loop-mode handler to optimize execution. Loop mode, which was introduced with the M68010, helps speed up the execution of specific loop constructs. However, it was removed in the M68020 due to the introduction of caches, making it obsolete.
After returning from the instruction handler, Moira checks if any breakpoints have been hit. If any are reached, it triggers the appropriate handling procedures. In the next section, we'll explore the details of a typical instruction handler.
