# Using the Delegation System

The Moira class informs your application about events, such as a reset, an exception or a debugger stop, by calling delegate functions. All delegates are optional: They have empty default implementations, and a CPU subclass only overrides the ones it needs.

```c++
class CPU : public Moira {

protected:

    void didResetCPU() override { printf("Reset\n"); }
    void willInterrupt(u8 level) override { printf("Level %d interrupt\n", level); }
};
```

All delegates are declared in `MoiraInterface.h`, together with the memory access functions. If `MOIRA_VIRTUAL_API` is disabled, the functions are not virtual. Your application then has to define all of them, including the delegates it doesn't need (see [Integrating the CPU Core](/guides/howto/HowToUseTheCpu)).

## State Delegates

These functions are called when the CPU changes its state:

| Delegate | Called... |
|:---------|:----------|
| `didResetCPU()` | ...at the end of `reset()`. |
| `didHaltCPU()` | ...when the CPU halts after an unrecoverable error, e.g. a double bus fault. Only `reset()` leaves this state. |
| `didEnableMMU()` | ...when the MMU is enabled (the E bit in the translation control register is set). |
| `didDisableMMU()` | ...when the MMU is disabled. |

## Instruction Delegates

These functions are called around selected instructions:

- `void willExecute(const char *func, Instr I, Mode M, Size S, u16 opcode)`

    Called at the beginning of an instruction. `func` is the name of the handler function, `I`, `M` and `S` describe the instruction, its addressing mode and its operand size.

- `void didExecute(const char *func, Instr I, Mode M, Size S, u16 opcode)`

    Called at the end of an instruction.

To keep the emulator fast, the two functions are only called for the instructions that are selected by the options `MOIRA_WILL_EXECUTE` and `MOIRA_DID_EXECUTE` in `MoiraConfig.h`. The options contain a condition on the template parameters of the instruction handlers, which is evaluated at compile time. The defaults are:

```c++
#define MOIRA_WILL_EXECUTE I == Instr::STOP || I == Instr::TAS || I == Instr::BKPT
#define MOIRA_DID_EXECUTE I == Instr::RESET
```

vAmiga, for example, uses `didExecute()` for `RESET` to reset the external hardware, as the RESET instruction does on a real machine.

- `void willAccessBus(u32 addr, Size size, bool write)`

    Called before a data operand is read or written, with the logical address and the size of the operand. A long word access that Moira splits into several bus cycles is reported only once. The delegate is only called if `MOIRA_REPORT_BUS_ACCESS` is enabled. It is intended for test environments that trace the operand accesses of the CPU.

## Exception Delegates

These functions are called when the CPU processes an exception:

| Delegate | Called... |
|:---------|:----------|
| `willExecute(M68kException exc, u16 vector)` | ...at the start of exception processing. |
| `didExecute(M68kException exc, u16 vector)` | ...at the end of exception processing. |
| `willInterrupt(u8 level)` | ...before an interrupt is acknowledged. |
| `didJumpToVector(int nr, u32 addr)` | ...after the stack frame has been written and the handler address has been loaded. `addr` is the address of the first instruction of the handler. |

`exc` tells which kind of exception occurs, e.g. `M68kException::BUS_ERROR`, `TRAP` or `FPU_ARITHMETIC`. `vector` is the vector number, which also distinguishes, for example, the 16 trap instructions.

## CPU Space Delegates

Some bus cycles don't access memory, but communicate with external hardware. These CPU space cycles (function code 7) are reported by the following functions:

- `void willExecute(CpuSpaceType type, u8 arg)`

    Called before a CPU space cycle is executed.

- `void didExecute(CpuSpaceType type, u8 arg)`

    Called after a CPU space cycle has been executed.

The cycle type is one of the following:

| Type | Cycle | `arg` |
|:-----|:------|:------|
| `IRQ_ACK` | Interrupt acknowledge (all models) | The interrupt level |
| `BREAKPOINT` | Breakpoint acknowledge (`BKPT` instruction, 68010 and later) | The breakpoint number |
| `COPROCESSOR` | First cycle of a coprocessor instruction (68020 and 68030 with a 68881 or 68882) | The coprocessor ID (bits 7-5) and the coprocessor interface register (bits 4-0) |

`arg` is the value the CPU places on the address lines A15-A13 and A4-A0. For interrupt acknowledge cycles, `willExecute()` is called before and `didExecute()` after `readIrqVector()`.

## Cache Delegates

These functions report changes to the cache registers (68020 and later):

| Delegate | Called... |
|:---------|:----------|
| `didChangeCACR(u32 value)` | ...when the cache control register is written. `value` is the value before it is masked with the bits the model supports. |
| `didChangeCAAR(u32 value)` | ...when the cache address register is written. |

## Debugger Delegates

These functions report debugger events. [Using the Debugger](/guides/howto/HowToUseTheDebugger) describes them in detail.

| Delegate | Called... |
|:---------|:----------|
| `didReachBreakpoint(u32 addr)` | ...when the next instruction is at a breakpoint address. |
| `didReachWatchpoint(u32 addr)` | ...when the CPU is about to access a watched address. |
| `didReachCatchpoint(u8 vector)` | ...when the CPU has taken a caught exception. |
| `didReachSoftstop(u32 addr)` | ...when a single step has completed. |
| `didReachSoftwareTrap(u32 addr)` | ...when the CPU has hit a software trap. |
| `didLogInstruction(const Registers &reg)` | ...when an instruction has been recorded in the instruction log. |
