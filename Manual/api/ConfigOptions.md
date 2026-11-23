# Configuration Options

Moira is configured at compile time. All options are defined in `Moira/MoiraConfig.h`. Each option is guarded by `#ifndef`, so it can also be set from the build system without editing the file:

```cmake
target_compile_definitions(moira PUBLIC MOIRA_PRECISE_TIMING=true)
```

## Overview

| Option | Default | Purpose |
|:-------|:--------|:--------|
| `MOIRA_NAMESPACE` | `moira` | Name of the namespace |
| `MOIRA_PRECISE_TIMING` | `false` | Calls `sync()` before each memory access (68000 and 68010) |
| `MOIRA_VIRTUAL_API` | `true` | Declares the interface functions as virtual |
| `MOIRA_EMULATE_ADDRESS_ERROR` | `false` | Emulates address errors |
| `MOIRA_EMULATE_BUS_ERROR` | `false` | Lets the 68020 and later restart an instruction after a bus error |
| `MOIRA_REPORT_BUS_ACCESS` | `false` | Calls `willAccessBus()` before each operand access |
| `MOIRA_EMULATE_FC` | `true` | Emulates the function code pins |
| `MOIRA_EMULATE_ICACHE` | `true` | Emulates the instruction cache (68020 and 68030) |
| `MOIRA_EMULATE_DCACHE` | `true` | Emulates the data cache (68030) |
| `MOIRA_ENABLE_DASM` | `true` | Enables the disassembler |
| `MOIRA_COMPATIBILITY` | `DEFAULT` | Selects the compatibility mode |
| `MOIRA_WILL_EXECUTE` | `STOP`, `TAS`, `BKPT` | Instructions that call `willExecute()` |
| `MOIRA_DID_EXECUTE` | `RESET` | Instructions that call `didExecute()` |

## Options

- `MOIRA_NAMESPACE`

  The name of the namespace that contains all of Moira. Build several differently configured libraries with different namespaces to link them into the same program, e.g. to use several compatibility modes at once.

- `MOIRA_PRECISE_TIMING`

  If disabled, Moira calls `sync()` at the end of each instruction, passing the number of elapsed cycles. If enabled, `sync()` is called before each memory access, so that the application can emulate the surrounding hardware up to the point where the access occurs. The option only affects the 68000 and 68010. See [The Timing Model](/guides/concepts/TimingModel).

  Enable to improve accuracy, disable to gain speed.

- `MOIRA_VIRTUAL_API`

  If enabled, the functions that the application provides (memory access, delegates) are virtual, and the application overrides them in a subclass. If disabled, they are ordinary member functions that the application defines directly, which avoids the cost of virtual calls. In this case, the application has to define all of them, including the optional ones. See [Integrating the CPU Core](/guides/howto/HowToUseTheCpu).

  Enable to follow the usual object-oriented design, disable to gain speed.

- `MOIRA_EMULATE_ADDRESS_ERROR`

  If enabled, the 68000 and 68010 raise an address error if a word or a long word is accessed at an odd address. On all models, an odd program counter raises an address error. See [Emulating Address Errors](/guides/howto/HowToEmulateAddressErrors).

  Enable to improve accuracy, disable to gain speed.

- `MOIRA_EMULATE_BUS_ERROR`

  The application signals a bus error by throwing a `BusError` from a memory access function. The 68000 and 68010 always process it at no extra cost. The 68020 and later can restart the faulted instruction after the handler returns with `RTE`. To support this, Moira saves the registers before each instruction, which forces these models onto the slow execution path. If the option is disabled, a `BusError` is still processed, but the instruction can't be restarted. While the MMU is enabled, the registers are always saved. See [Emulating Bus Errors](/guides/howto/HowToEmulateBusErrors).

  Enable to improve accuracy, disable to gain speed.

- `MOIRA_REPORT_BUS_ACCESS`

  If enabled, `willAccessBus()` is called before the bus cycles of each operand access. A long word access that is split into several bus cycles is reported once. The option is intended for test environments that trace the operand accesses.

  Disable to gain speed.

- `MOIRA_EMULATE_FC`

  If enabled, Moira emulates the function code pins FC0 - FC2, which tell external hardware the type of each memory access (user or supervisor, program or data). `getFC()` returns their current value.

  Enable to improve accuracy, disable to gain speed.

- `MOIRA_EMULATE_ICACHE`

  If enabled, the instruction cache of the 68020 and 68030 is emulated, including its effect on timing. If disabled, every instruction fetch goes to the bus. The caches of the 68040 are not emulated.

  Enable to improve accuracy, disable to gain speed.

- `MOIRA_EMULATE_DCACHE`

  If enabled, the data cache of the 68030 is emulated. If disabled, every data access goes to the bus.

  Enable to improve accuracy, disable to gain speed.

- `MOIRA_ENABLE_DASM`

  Enables the disassembler. It needs a jump table of about 1 MB, plus 0.25 MB for the instruction information returned by `getInstrInfo()`. If disabled, the disassembler functions throw `std::runtime_error`. See [Using the Disassembler](/guides/howto/HowToUseTheDisassembler).

  Disable to save memory.

- `MOIRA_COMPATIBILITY`

  Selects the compatibility mode: `DEFAULT`, `UAE`, `MAME` or `MUSASHI`. The modes other than `DEFAULT` mimic the behavior of other emulators and are meant for testing. See [Compatibility Modes](/guides/concepts/CompatibilityModes).

- `MOIRA_WILL_EXECUTE`

  A condition on the template parameters of the instruction handlers. For all instructions that satisfy it, `willExecute()` is called at the beginning of the handler. The condition is evaluated at compile time. See [Using the Delegation System](/guides/howto/HowToUseDelegates).

  Default: `#define MOIRA_WILL_EXECUTE I == Instr::STOP || I == Instr::TAS || I == Instr::BKPT`

- `MOIRA_DID_EXECUTE`

  Like `MOIRA_WILL_EXECUTE`, but for `didExecute()`, which is called at the end of the handler.

  Default: `#define MOIRA_DID_EXECUTE I == Instr::RESET`
