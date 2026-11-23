# The Timing Model

A CPU emulator for a single-chip computer only needs to know how long each instruction takes. A computer like the Amiga needs much more: The custom chips share the memory bus with the CPU, so it matters at which cycle the CPU accesses memory, not only how many cycles an instruction takes in total. This article explains how Moira keeps track of time and how your application stays in sync with it.

## The Clock

Moira counts time in CPU cycles. The current value is returned by `getClock()` and can be changed with `setClock()`. The clock is not reset by `reset()`.

Whenever time passes, the CPU calls `sync(cycles)`. The default implementation advances the clock. Applications override it to let other components catch up with the CPU (see [Integrating the CPU Core](/guides/howto/HowToUseTheCpu)):

```c++
void MyCPU::sync(int cycles)
{
    Moira::sync(cycles);            // Advance the clock
    chips.executeUntil(getClock()); // Let the rest of the computer catch up
}
```

`execute()` runs a single instruction and returns the number of elapsed cycles. `execute(cycles)` and `executeUntil(cycle)` run instructions until a cycle budget is used up or the clock has reached a certain value. Because an instruction can't be interrupted, they may overshoot by the duration of the last instruction.

## The 68000 and 68010

The 68000 and 68010 are emulated cycle-exact. Moira knows the number of cycles of each instruction and the position of each bus access inside it. How much of this information reaches your application depends on `MOIRA_PRECISE_TIMING`:

- **Disabled (default).** `sync()` is called once at the end of each instruction with the total number of cycles. This is the fastest mode. It is sufficient if no other component needs to observe the CPU's bus accesses at the exact cycle.

- **Enabled.** `sync()` is called before each bus access with the number of cycles since the previous one. When `read16()` is called, for example, the clock shows the cycle at which the real CPU starts the bus cycle. Other components can be emulated up to this point before the access is carried out. This is the mode vAmiga uses to interleave the CPU's accesses with the DMA cycles of the custom chips.

In both modes, an instruction takes the same number of cycles. Only the granularity of the `sync()` calls differs.

The timing of the 68000 is checked against MAME's microcoded core and UAE, which compare the cycle count and the sequence of bus accesses of every instruction, including the exception sequences. The 68010 is checked against UAE.

## Interrupt Sampling

The real 68000 doesn't check the interrupt lines at an arbitrary point in time. It samples the interrupt priority level (IPL) during the last bus cycle of an instruction. An interrupt that is requested after this point is taken one instruction later.

Moira emulates this. The value passed to `setIPL()` is stored on the pins, and the CPU latches it at the sampling point of the instruction. With `MOIRA_PRECISE_TIMING` enabled, a component that raises an interrupt inside `sync()` therefore sees the same latency as on the real machine.

## The 68020 and Later

The 68020 has an instruction cache and a pipeline that overlaps the execution of consecutive instructions. Its timing depends on the cache contents and on how well the instructions fit into the pipeline. Motorola's manuals only give estimates for the best, the cache and the worst case. Therefore, Moira doesn't emulate the 68020 cycle-exact. It computes an approximation:

- Each instruction has a base number of cycles.
- A **penalty** is added for effective address calculations, for instruction fetches that miss the cache, and for accesses to devices with a port narrower than 32 bits (see `dsack()`).
- The cycles are booked on a **cycle budget**. The budget is spent in whole bus cycles (two clock cycles). If an instruction completes faster than that, the remainder is carried over to the next one. After a few instructions that didn't advance the clock, a bus cycle is borrowed from the budget, so that other components keep running.

`MOIRA_PRECISE_TIMING` has no effect on these models: `sync()` is called when the budget is spent, not before each bus access.

The 68030 and 68040 run on the 68020 core and use the same model. The data cache of the 68030 is emulated, too. The caches of the 68040 are not emulated, and FPU and MMU operations are not cycle-exact.

## Changing the Speed

The clock counts CPU cycles. To emulate a CPU that runs at a different frequency than the rest of the computer, the application converts between the two time bases in `sync()`. [How to Implement Overclocking](/guides/howto/HowToOverclockTheCpu) shows how vAmiga does this.
