# How to Implement Overclocking

Moira counts time in CPU cycles and reports it to your application through `sync()`. It doesn't know how fast the CPU runs compared to the rest of the computer. This makes overclocking a matter of the application: It only has to convert the CPU cycles into the time base of the other components. This guide shows how vAmiga does this. A similar approach works for any emulator.

## Running the CPU at Native Speed

In an Amiga, all clocks are derived from a single master clock, which runs at 28.375160 MHz on PAL machines. The CPU clock is a quarter of it (7.093790 MHz). The memory bus is driven by the color clock, which is an eighth of it (3.546895 MHz). Hence, two CPU cycles make up one bus cycle, which is also called a DMA cycle, because the custom chips use these cycles to access memory.

vAmiga keeps the custom chips in sync with the CPU in `sync()`. It runs with `MOIRA_PRECISE_TIMING` enabled, so `sync()` is called before each memory access (see [The Timing Model](/guides/concepts/TimingModel)). At native speed, the function advances the CPU clock and lets Agnus, the chip that controls the bus, emulate the same amount of time:

```c++
void Moira::sync(int cycles)
{
    // Advance the CPU clock
    clock += cycles;

    // Emulate Agnus up to the same point in time (2 CPU cycles = 1 DMA cycle)
    agnus.execute(cycles / 2);
}
```

vAmiga builds Moira with `MOIRA_VIRTUAL_API` disabled, so it defines `Moira::sync()` directly. With the virtual API, override `sync()` in your subclass and call `Moira::sync(cycles)` instead of changing the clock yourself.

## Running the CPU Faster

An overclocked CPU executes more cycles in the same amount of time. If the CPU runs at twice the speed, four CPU cycles make up one DMA cycle instead of two. vAmiga lets the user choose an overclocking factor `n` and converts the cycles with a sub-cycle counter, `debt`, which collects CPU cycles until they amount to a full DMA cycle:

```c++
void Moira::sync(int cycles)
{
    // Number of CPU cycles that make up one DMA cycle
    auto cyclesPerDmaCycle = 2 * n;

    debt += cycles;

    while (debt >= cyclesPerDmaCycle) {

        // Advance the clock by one DMA cycle and let Agnus catch up
        clock += 2;
        agnus.execute();

        debt -= cyclesPerDmaCycle;
    }
}
```

Note that the clock is advanced by two cycles per DMA cycle, regardless of the overclocking factor. The clock no longer counts the cycles the CPU has executed, but the time that has passed, measured in native CPU cycles. This keeps all time stamps that the rest of the emulator derives from the CPU clock valid.

Any remainder stays in `debt` and is carried over to the next call.

## Accessing the Bus

A faster CPU doesn't speed up the memory bus. When the CPU accesses chip memory or a custom chip register, it has to wait for the bus like a CPU at native speed. vAmiga handles this in two steps when the CPU requests the bus:

1. **Resynchronizing.** If `debt` is not zero, the CPU is in the middle of a DMA cycle. Before the access, the clock is advanced to the start of the next DMA cycle, and `debt` is cleared:

   ```c++
   if (debt) {

       clock += 2;
       agnus.execute();
       debt = 0;
   }
   ```

2. **Slowing down the access.** vAmiga sets a counter, `slowCycles`, to 1. `sync()` counts that many CPU cycles as full DMA cycles before it continues at the faster rate:

   ```c++
   // Execute some cycles at native speed if required
   while (slowCycles && cycles) {

       debt += cyclesPerDmaCycle;
       cycles--;
       slowCycles--;
   }
   ```

   If the bus is blocked by DMA, vAmiga lets Agnus run until the bus is free and adds the elapsed time as wait states, as it does at native speed.

For accesses to the CIAs, vAmiga resynchronizes the CPU in the same way and then waits for the E clock, which drives the CIAs.

The effect is that an overclocked CPU runs faster on everything that doesn't need the bus, like internal operations and accesses to fast memory, but not on accesses to chip memory and the custom chips, similar to an accelerator card.

## The 68020 and Later

The same technique works for the 68020 and later models. Their timing is not cycle-exact, and `sync()` is called when the cycle budget is spent rather than before each memory access (see [The Timing Model](/guides/concepts/TimingModel)). The conversion in `sync()` is unchanged.
