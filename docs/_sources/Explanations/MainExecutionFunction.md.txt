# The main execution function

This article gives an insight into the inner workings of the main execute function. That is, it explains what happens after calling `Moira::execute()`. 

This is how the main execution functions looks like in v2.2:

```c++
void
Moira::execute()
{
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
}
```

## The fast path 

First and foremost, Moira checks if a bit is set in variable `flags`. Flags are only set in exceptional situations, which means the variable will be 0 most of the time. In this case Moira can execute the function on a fast path, since many time consuming tasks can be skipped. As can be seen above, very few statements are processed. First, Moira increments the program counter by 2. Then, it calls the handler of the currently executed instruction by looking up a function pointer in the `exec` table. 

The command handler call is embedded in a try-catch block because it may throw. For example, if an address error occurs within the command handler, an `AddressError` exception is thrown. The exception is caught and processed further in the `processException` function. 

## The slow path

If at least one bit is set in variable `flags`, Moira is forced to execute the slow path. On this path, Moira checks all flags one by one and triggers special actions if necessary. The following code skeleton shows the details:

```c++
        // Only continue if the CPU is not halted
        if (flags & CPU_IS_HALTED) {
            ...
            return;
        }

        // Process pending trace exception (if any)
        if (flags & CPU_TRACE_EXCEPTION) {
            ...
            goto done;
        }

        // Check if the T flag is set inside the status register
        if ((flags & CPU_TRACE_FLAG) && !(flags & CPU_IS_STOPPED)) {
            ...
        }

        // Process pending interrupt (if any)
        if (flags & CPU_CHECK_IRQ) {
            ...
        }

        // If the CPU is stopped, poll the IPL lines and return
        if (flags & CPU_IS_STOPPED) {
            ...
            return;
        }

        // If logging is enabled, record the executed instruction
        if (flags & CPU_LOG_INSTRUCTION) {
            ...
        }

        // Execute the instruction
        reg.pc += 2;

        if (flags & CPU_IS_LOOPING) {
            ...
        } else {

            try {
                (this->*exec[queue.ird])(queue.ird);
            } catch (const std::exception &exc) {
                processException(exc);
            }
        }

    done:

        // Check if a breakpoint has been reached
        if (flags & CPU_CHECK_BP) {

            // Don't break if the instruction won't be executed due to tracing
            if (flags & CPU_TRACE_EXCEPTION) return;

            // Check if a softstop has been reached
            if (debugger.softstopMatches(reg.pc0)) { ... }

            // Check if a breakpoint has been reached
            if (debugger.breakpointMatches(reg.pc0)) { ... }
        }
    }
}
```

On the slow path, Moira calls the instruction handler somewhere in the middle. Note that the code is embedded in a block that checks the `CPU_IS_LOOPING` flag. When the flag is cleared, Moira calls the instruction handler exactly as it would on the fast path. When the flag is set, a special loop-mode handler is called. Loop mode is a feature of the M68010 that allows the CPU to speed up the execution of certain loop constructs. In hindsight, this feature was short lived as it was no longer present in the M68020. It had become obsolete with the introduction of caches. 

After returning from the instruction handler, Moira checks for breakpoints and exits. In the next section we will learn what a typical instruction handler looks like.
