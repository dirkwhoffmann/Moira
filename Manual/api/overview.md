---
title: "Overview"
---
# Overview

The whole public API hangs off a single object of class [moira::Moira](/api/moira-Moira). The class is abstract: to use it, derive your own CPU class and connect it to your memory by implementing the memory access functions. See the [guides](/guides/tutorials/UsingMoiraInYourOwnApp) for a walk-through.

All symbols live in the namespace `moira`. The name can be changed at compile time with the option `MOIRA_NAMESPACE`, which makes it possible to link several differently configured CPU cores into one program (see the [configuration options](/guides/reference/ConfigOptions)).

## Classes

| Class | Purpose |
|:------|:--------|
| [moira::Moira](/api/moira-Moira) | The CPU: configuration, execution, registers, disassembler. |
| [moira::Debugger](/api/moira-Debugger) | Breakpoints, watchpoints, catchpoints, software traps, single-stepping and instruction logging. Available as `Moira::debugger`. |
| [moira::Guards](/api/moira-Guards) | Base class of [Breakpoints](/api/moira-Breakpoints), [Watchpoints](/api/moira-Watchpoints) and [Catchpoints](/api/moira-Catchpoints). |
| [moira::SoftwareTraps](/api/moira-SoftwareTraps) | A table of software traps. |
| [moira::FPU](/api/moira-FPU) | The floating-point unit (68881, 68882 and the FPU of the 68040): registers and arithmetic. |
| [moira::FpuEnv](/api/moira-FpuEnv) | The floating-point environment: rounding mode, precision, exception flags and the rounded arithmetic. |
| [moira::MFloat80](/api/moira-MFloat80) | The 80-bit extended precision value type. |
| [moira::FpuFormats](/api/moira-FpuFormats) | Conversions between `MFloat80` and the memory formats of the FPU. |
| [moira::MMU](/api/moira-MMU) | The memory management unit. Dispatches to [MMU030](/api/moira-MMU030) or [MMU040](/api/moira-MMU040). |

## Delegates

The functions the CPU calls to communicate with your application are protected virtual functions of [moira::Moira](/api/moira-Moira):

- **Memory access.** `read8`, `read16`, `read32`, `write8`, `write16`, `write32` are required. The functions `read16OnReset`, `read16Dasm`, `dsack` and `readIrqVector` are optional refinements.
- **Notifications.** `cpuDidReset`, `cpuDidHalt`, `willExecute`, `didExecute`, `willInterrupt`, `didJumpToVector`, `didChangeCACR` and others inform you about what the CPU is doing. They do nothing by default.
- **Debugger events.** `didReachBreakpoint`, `didReachWatchpoint`, `didReachCatchpoint`, `didReachSoftstop` and `didReachSoftwareTrap` are called when the debugger stops the CPU. See [How to use the debugger](/guides/howto/HowToUseTheDebugger).

## Types

The data types of the API are collected on a single page, [Types and Enumerations](/api/moira). The most important ones are:

| Type | Purpose |
|:-----|:--------|
| [CPUModel](/api/enum-CPUModel) | The emulated CPU model. |
| [Registers](/api/moira-Registers) | The register set, as returned by the debugger's log. |
| [Syntax](/api/enum-Syntax), [LetterCase](/api/enum-LetterCase), [DasmStyle](/api/moira-DasmStyle) | Disassembler configuration. |
| [Instr](/api/enum-Instr), [Mode](/api/enum-Mode), [InstrInfo](/api/moira-InstrInfo) | Description of instructions. |
| [M68kException](/api/enum-M68kException) | The exceptions of the 68k family. |

## Conventions

| Pattern | Meaning |
|:--------|:--------|
| `getX()` / `setX(...)` | Read and change a register or a setting. |
| `xxxAt(addr)` | Variant of a function that identifies an object by its address instead of its index (guards). |
| `willXxx` / `didXxx` | Delegate that is called before or after an event. |
