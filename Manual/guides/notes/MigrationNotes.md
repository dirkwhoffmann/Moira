# Migration Notes

This page lists the API changes that require changes in client code. It is meant for applications that update an older copy of Moira.

## Renamed

| Old | New |
|:----|:----|
| `readFC()` | `getFC()` |
| `hasInternalFpu()`, `hasExternalFpu()` | `hasInternalFPU()`, `hasExternalFPU()` |
| `cpuDidReset()`, `cpuDidHalt()` | `didResetCPU()`, `didHaltCPU()` |
| `mmuDidEnable()`, `mmuDidDisable()` | `didEnableMMU()`, `didDisableMMU()` |
| Coprocessor API (`Cop…`) | `CPModel`, `setCPModel()`, `getCPModel()` |
| Static `Moira::hasCPI(model)`, `hasMMU(model)`, `hasFPU(model)` | Free functions `moira::hasCPI(model)`, `moira::hasMMU(model)`, `moira::hasFPU(model)` |

## Removed

| Removed | Use instead |
|:--------|:------------|
| `dump8()`, `dump16()`, `dump24()`, `dump32()` | `disassemble(fmt, addr)` with `%p`, `%b`, `%Nd` or `%Na` |
| `disassembleSR()` | `disassemble(fmt, addr)` with `%f` |
| `setIRC()`, `setIRD()` | Set up the prefetch queue with `Debugger::jump()`, or from a subclass via the `queue` member |
| `setIrqMode()`, `getIrqMode()` | Implement the delegate `readIrqVector(level, &vector)` |
| `setDasmSyntax()`, `setDasmNumberFormat()`, `setDasmLetterCase()`, `setDasmIndentation()` | `getDasmStyle()` and `setDasmStyle()` |
| `setDumpNumberFormat()`, `setDumpIndentation()` | `getDataStyle()` and `setDataStyle()` |
| `peekProgramWord()` | `read16Dasm()` |
| `updateMmuFlag()` | Program the MMU via `mmu.mmu030.setTC()` or `mmu.mmu040.setTC()` |
| `availabilityMask()` (now private) | `isAvailable()` |

## Changed

- **`pc0`.** `getPC0()` returns the start of the instruction that is currently executing and the next instruction between instructions. `getPrevPC0()` returns the instruction executed before.
- **Interrupt acknowledge and CPU space cycles.** Cycles with FC = 7 are announced with `willExecute(CpuSpaceType, u8)` and `didExecute(CpuSpaceType, u8)`. While the delegate runs, `getFC()` still returns the value of the previous cycle.
- **Bus operands.** The optional delegate `willAccessBus(addr, size, write)` reports data operand accesses. It is only called if `MOIRA_REPORT_BUS_ACCESS` is enabled.
- **Register types.** `getSFC()`, `getDFC()`, `setSFC()` and `setDFC()` use `u8`.
- **Caches.** `iCache` and `dCache` are public.
- **New functions.** `getModel()`, `getDasmModel()`, `getEDB()`, `getDasmStyle()`, `setDasmStyle()`, `getDataStyle()`, `setDataStyle()`, `disassemble(fmt, addr, &len)`, `Debugger::disassembleRecorded()`, `Debugger::dumpLogBuffer()` and `Debugger::disassembleRange()`.
- **Non-virtual API.** If you build with `MOIRA_VIRTUAL_API = false`, you must define all functions of the interface (see `MoiraInterface.h`), including `readIrqVector` and `willAccessBus`.
- **New source file.** `MoiraInterface.h` has to be part of your project.
