# Integrating the CPU Core

This guide shows how to integrate Moira into an application. It builds a small computer step by step: a 68000 with RAM and a timer that raises an interrupt at a fixed rate. The same steps apply to any emulator, from a simple trainer board to a full Amiga.

## Adding Moira to Your Project

Moira is self-contained. Everything you need is in the `Moira/` folder. If your project uses CMake, add the folder as a subdirectory and link against the `moira` library:

```cmake
add_subdirectory(Moira)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE moira)
```

Linking against `moira` also adds the folder to the include path.

With other build systems, compile the source files that are listed in `Moira/CMakeLists.txt` and add the folder to the include path. The application only includes a single header:

```c++
#include "Moira.h"

using namespace moira;
```

## Choosing the Compile-Time Options

Moira is configured at compile time in `MoiraConfig.h`. The defaults are a good choice for most applications. The options you are most likely to change are:

| Option | Default | Change it if... |
|:-------|:--------|:----------------|
| `MOIRA_PRECISE_TIMING` | `false` | ...other hardware has to see the CPU's memory accesses at the right cycle (68000 and 68010). |
| `MOIRA_EMULATE_ADDRESS_ERROR` | `false` | ...software relies on the address error exception for odd word accesses (68000 and 68010). |
| `MOIRA_EMULATE_BUS_ERROR` | `false` | ...instructions on the 68020 and later have to be restarted after a bus error. |
| `MOIRA_VIRTUAL_API` | `true` | ...you want to avoid the cost of virtual function calls (see below). |
| `MOIRA_ENABLE_DASM` | `true` | ...you don't need the disassembler and want to save memory. |
| `MOIRA_COMPATIBILITY` | `DEFAULT` | ...you need the behavior of another emulator (Musashi, UAE or MAME), e.g. to compare results. |

Every option is guarded by `#ifndef`, so it can also be set from the build system instead of editing the file:

```cmake
target_compile_definitions(moira PUBLIC MOIRA_PRECISE_TIMING=true)
```

[Configuration Options](/api/ConfigOptions) describes all options. In our example, we keep the defaults.

## Connecting the CPU to Memory

Moira doesn't own any memory. Whenever the CPU needs the bus, it calls a function that your application provides. To connect the CPU to the rest of the computer, derive a class from `Moira` and implement the six memory access functions:

```c++
class Computer;

class CPU : public Moira {

    Computer &computer;

public:

    CPU(Computer &computer) : computer(computer) { }

protected:

    u8 read8(u32 addr) const override;
    u16 read16(u32 addr) const override;
    u32 read32(u32 addr) const override;
    void write8(u32 addr, u8 val) const override;
    void write16(u32 addr, u16 val) const override;
    void write32(u32 addr, u32 val) const override;

    void sync(int cycles) override;
};
```

The functions forward the requests to the computer, which owns the memory. Keep in mind that the 68k is a big-endian CPU: The high byte of a word is stored first.

```c++
u8 CPU::read8(u32 addr) const { return computer.peek8(addr); }
u16 CPU::read16(u32 addr) const { return u16(read8(addr) << 8 | read8(addr + 1)); }
u32 CPU::read32(u32 addr) const { return u32(read16(addr) << 16 | read16(addr + 2)); }

void CPU::write8(u32 addr, u8 val) const { computer.poke8(addr, val); }
void CPU::write16(u32 addr, u16 val) const { computer.poke16(addr, val); }
void CPU::write32(u32 addr, u32 val) const { computer.poke32(addr, val); }
```

Moira decides how a long word access is performed. On the 68000 and 68010, it is always split into two word accesses. The 68020 and later adapt their bus cycles to the port size of the addressed device, which the application reports with `dsack()`. By default, every device has a 16-bit port, and long words are also split into word accesses. `read32()` and `write32()` are called for aligned long word accesses to a device with a 32-bit port.

Besides the memory functions, the `Moira` class declares a number of optional functions. All of them have default implementations, so you only override what you need. Most of them are delegates that inform the application about events, e.g. `didHaltCPU()` or `didReachBreakpoint()` (see [Using the Delegation System](/guides/howto/HowToUseDelegates)).

## Selecting the CPU Model

The CPU model is selected with `setModel()`. Our computer uses a 68000:

```c++
cpu.setModel(CPUModel::M68000);
```

Moira supports the 68000, 68010, 68EC020, 68020, 68EC030, 68030, 68EC040, 68LC040 and 68040. On the 68020 and 68030, a floating-point coprocessor can be added with `setCPModel(CPModel::M68881)` or `setCPModel(CPModel::M68882)`. The 68040 has a built-in FPU.

`setModel()` also selects the model of the disassembler, which decides which instructions are recognized. The disassembler model can be chosen independently with `setModel(cpuModel, dasmModel)` or `setDasmModel()`. This is useful, for example, to disassemble 68040 code on an emulated 68000.

## Keeping the Hardware in Sync

The CPU is only one component of a computer. Other components, such as timers or video hardware, have to advance in step with the CPU. For this purpose, Moira calls `sync()` whenever time passes. The default implementation advances the CPU's clock. An overriding implementation has to call it, too:

```c++
void CPU::sync(int cycles)
{
    // Advance the CPU's clock
    Moira::sync(cycles);

    // Let the timer catch up and connect its interrupt line to the CPU
    computer.timer.update(getClock());
    setIPL(computer.timer.pending ? 6 : 0);
}
```

By default, `sync()` is called once at the end of each instruction. If `MOIRA_PRECISE_TIMING` is enabled, the 68000 and 68010 call it before each memory access, so that other components see the access at the right cycle. vAmiga uses this to emulate the bus sharing between the CPU and the custom chips: In `sync()`, it emulates the chips up to the cycle the CPU has reached. If the bus is blocked when the CPU accesses memory, vAmiga advances the clock by the number of wait states. See [The Timing Model](/guides/concepts/TimingModel) for details.

Our timer requests an interrupt every 1000 cycles. The interrupt is acknowledged by a write to address `$F00000`:

```c++
class Timer {

public:

    i64 interval = 1000;    // Cycles between two interrupts
    i64 next = 1000;        // Cycle of the next interrupt
    bool pending = false;   // State of the interrupt line

    void update(i64 clock) { if (clock >= next) { pending = true; next += interval; } }
};
```

## Handling Interrupts

Interrupts are requested by setting the interrupt priority level pins with `setIPL()`, as shown above. The CPU checks the pins at the next instruction boundary and takes the interrupt if its level is above the interrupt mask in the status register. Level 7 is non-maskable.

By default, an interrupt is answered with an autovector (vector 24 + level). Devices that supply their own vector number are emulated by overriding `readIrqVector()` (declared in the class like the memory functions):

```c++
IrqMode CPU::readIrqVector(u8 level, u8 *vector) const
{
    if (level == 6) { *vector = 64; return IrqMode::USER; }
    return IrqMode::AUTO;
}
```

## Putting It All Together

The computer owns the memory, the timer and the CPU:

```c++
class Computer {

public:

    std::vector<u8> ram = std::vector<u8>(1 << 20);
    Timer timer;
    CPU cpu = CPU(*this);

    Computer() { cpu.setModel(CPUModel::M68000); }

    u8 peek8(u32 addr) const { return ram[addr & 0xFFFFF]; }

    void poke8(u32 addr, u8 val)
    {
        // A write to $F00000 acknowledges the timer interrupt
        if (addr == 0xF00000) { timer.pending = false; return; }
        ram[addr & 0xFFFFF] = val;
    }
    void poke16(u32 addr, u16 val) { poke8(addr, u8(val >> 8)); poke8(addr + 1, u8(val)); }
    void poke32(u32 addr, u32 val) { poke16(addr, u16(val >> 16)); poke16(addr + 2, u16(val)); }
};
```

After the program and the vector table have been loaded into memory, the CPU is reset. The reset reads the initial stack pointer and the initial program counter from addresses 0 and 4:

```c++
computer.cpu.reset();
```

## Creating a Run Loop

The emulator's run loop calls `execute()`, which runs a single instruction (or processes a pending exception) and returns the number of elapsed cycles. Emulators typically run the CPU for one video frame at a time:

```c++
void Computer::runFrame(i64 cycles)
{
    auto end = cpu.getClock() + cycles;
    while (cpu.getClock() < end) cpu.execute();
}
```

If no other action is needed between two instructions, `execute(cycles)` and `executeUntil(cycle)` do the same in a single call. Like the loop above, they may overshoot by the duration of the last instruction, because an instruction cannot be interrupted.

The run loop is also the place to react to events. A debugger front end, for example, sets a flag in `didReachBreakpoint()` and checks it after each instruction to pause the emulation (see [Using the Debugger](/guides/howto/HowToUseTheDebugger)).

## Inspecting the CPU State

The registers can be read and written at any time, e.g. by a debugger while the emulation is paused. The most important functions are:

| Function | Description |
|:---------|:------------|
| `getD(n)`, `setD(n, value)` | Data register Dn |
| `getA(n)`, `setA(n, value)` | Address register An |
| `getPC0()` | Address of the current instruction |
| `getPC()` | Program counter |
| `getSR()`, `setSR(value)` | Status register |
| `getCCR()`, `setCCR(value)` | Condition code register (the lower byte of the status register) |
| `getSP()`, `getUSP()` | Active and user stack pointer |
| `getIPL()` | Value on the interrupt priority level pins |

`getPC0()` is usually the function you want rather than `getPC()`. The program counter is a moving target: It advances several times while an instruction is executed, because the CPU prefetches the following words. `getPC0()` returns the start address of the instruction. It is set when the instruction starts and stays the same until the next one begins.

To continue execution at a different address, use `jump()`, which also refills the prefetch queue. `setPC()` only changes the register.

## Special Memory Accesses

Two optional functions let your application handle memory accesses that need special care:

- `u16 read16OnReset(u32 addr)`

  During a reset, the CPU reads the initial stack pointer and the initial program counter from addresses 0 to 7. Some systems map a different memory into this range while the reset is in progress, e.g. a ROM. By overriding this function, your application can supply these values without setting up the whole memory system. The default implementation calls `read16()`.

- `u16 read16Dasm(u32 addr)`

  The disassembler and the debugger fetch instruction words with this function. Reading memory-mapped I/O registers can have side effects, which must not happen when an instruction is merely disassembled. If your memory has such registers, override this function to return the same values as `read16()` without triggering the side effects. The default implementation calls `read16()`.

## Avoiding Virtual Function Calls

By default, the functions your application provides are virtual. Since the CPU calls them for every memory access, the cost of a virtual call can matter for a cycle-exact emulator. If `MOIRA_VIRTUAL_API` is set to `false`, they are declared as ordinary member functions instead. The application then doesn't derive a class, but defines the functions of the `Moira` class directly:

```c++
u8 Moira::read8(u32 addr) const
{
    return mem.peek8(addr);
}
```

In this mode, the application has to define all functions, including the optional delegates, because there are no default implementations. The functions also need a way to reach the rest of the emulator. vAmiga, the Amiga emulator Moira was written for, uses this mode: Its copy of Moira derives from the base class of all Amiga components, which gives every Moira function access to the memory and the other chips.

## Next Steps

- [Using the Disassembler](/guides/howto/HowToUseTheDisassembler) and [Using the Debugger](/guides/howto/HowToUseTheDebugger) show how to inspect a running program.
- The API reference describes all classes and functions, starting with [moira::Moira](/api/moira-Moira).
