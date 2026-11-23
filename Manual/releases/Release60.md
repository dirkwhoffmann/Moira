# Moira 6.0

The release has 417 commits since v3.0 (March 2025). The emulator now runs the 68030 and 68040 models instead of only disassembling them. Bus errors and the compatibility modes have been reworked, and every change is checked against reference emulators and real hardware.

## Highlights

- **68030 and 68040 execution.** In v3.0 these models were disassembler-only. Now they run on the 68020 core and add the parts each model needs:
  - a memory management unit (MMU030 or MMU040, including transparent translation);
  - instruction and data caches (a new `Cache` class for both the 68020 and the 68030/68040 layout);
  - the 68040's built-in FPU and its missing instructions.
- **Floating-point unit.** A new FPU emulates the 68881, the 68882 and the 68040's built-in FPU. It is built on its own 80-bit float type (`MFloat80`, `FpuEnv`) and no longer depends on an external softfloat library. Its results are compared with UAE's softfloat FPU.
- **Bus errors.** The new `MOIRA_EMULATE_BUS_ERROR` option emulates bus errors. Moira writes the bus error frames of all models: the 68000 and 68010 frames, the 68020/68030 short and long bus fault frames, and the 68040 access error frame. Instructions resume correctly after RTE on the 68010, the 68020 and the 68030/68040.
- **Compatibility modes.** `MOIRA_COMPATIBILITY` replaces `MOIRA_MIMIC_MUSASHI`. It selects one of the following modes:
  - `DEFAULT`: Moira's own behavior. Where real hardware can decide, it follows the hardware.
  - `MUSASHI`: Musashi's timing.
  - `UAE`: WinUAE/Amiberry behavior, including cycle counts and exception frames.
  - `MAME`: the MAME 68000 core, which produced the SingleStepTests reference data.
- **Accuracy verified on real hardware.** New vAmigaTS recordings (an A500 with a 68000 and with a 68010) settle cases where the reference emulators disagree or are wrong. They cover DBcc with an odd target, the address error frames of MOVEA, JSR (d16,An) and MOVEM (d8,An,Xn), the CHK overflow quirk, and the timing of RTE with a format-8 frame on the 68010.

## API changes (breaking)

The [migration notes](/releases/MigrationNotes) list the changes that require changes in client code.

### Model selection
- `Model` is renamed to `CPUModel`.
- `setModel(CPUModel cpu, CPUModel dasm)` selects the CPU model and the disassembler model separately. `setModel(CPUModel)` sets both, and `setDasmModel()` sets only the disassembler model.
- New `CPModel` and `setCPModel()` choose the coprocessor (none, 68881 or 68882). The coprocessor is only used on CPUs with a coprocessor interface. The 68040's FPU is part of the CPU model.

### Memory and bus callbacks
These are now declared through `MoiraInterface.h` (`MOIRA_REQUIRED`, `MOIRA_DEFAULT`, `MOIRA_DELEGATE`):
- `read32()` and `write32()` are new and required. Long word accesses go through them, and the CPU model decides whether to split them into word accesses.
- `readIrqUserVector(level)` is replaced by `IrqMode readIrqVector(u8 level, u8 *vector)`. `getIrqMode()` and `setIrqMode()` are new.
- `cpuDidReset()` and `cpuDidHalt()` are renamed to `didResetCPU()` and `didHaltCPU()`.
- New callbacks:
  - `dsack()` for the 68020+ dynamic bus sizing;
  - `willAccessBus()`;
  - `didEnableMMU()` and `didDisableMMU()`;
  - `willExecute()` and `didExecute()` for CPU space cycles (`CpuSpaceType`);
  - `didLogInstruction()`.

### Disassembler and data formatting
- The individual setters (`setDasmSyntax`, `setDasmLetterCase`, `setDasmNumberFormat`, `setDasmIndentation`, `setDumpNumberFormat`, `setDumpIndentation`, `setNumberFormat`) are replaced by `setDasmStyle(const DasmStyle &)` and `setDataStyle(...)`, with getters to match.
- New `Syntax::UAE`.
- New `instrLength()` and `disassembleInstr()`.

### Other
- New `version()` and `build()`.
- New `getModel()`, `getDasmModel()`, `getCPModel()`, `getFC()`, `getEDB()`, `getPrevPC0()`, `isStopped()` and `isValidMovecReg()`.
- `pc0` stays the same for the whole instruction.
- New `MOIRA_NAMESPACE`, which defaults to `moira`.

## New configuration options (`MoiraConfig.h`)

| Option | Default | Purpose |
|---|---|---|
| `MOIRA_COMPATIBILITY` | `DEFAULT` | Compatibility mode (see above) |
| `MOIRA_EMULATE_BUS_ERROR` | `false` | Bus error emulation |
| `MOIRA_REPORT_BUS_ACCESS` | `false` | Calls `willAccessBus()` before each bus access |
| `MOIRA_EMULATE_ICACHE` / `MOIRA_EMULATE_DCACHE` | `true` | Cache emulation on the 68020+ |
| `MOIRA_NAMESPACE` | `moira` | Namespace of the core |

`MOIRA_MIMIC_MUSASHI` and `MOIRA_BUILD_INSTR_INFO_TABLE` are removed.

## Accuracy fixes (selection)

- 68000:
  - The address error frames of MOVE.L, MOVEA, MOVEM, JSR and DBcc are now correct, and so are the read/write order of the bit instructions (BTST, BCHG, BCLR, BSET).
  - When the branch target of DBcc is odd, the stacked PC is the address of the extension word. The address error happens even when the branch isn't taken, and the counter isn't decremented. This was verified on real hardware.
  - DIVU and DIVS have the correct prefetch timing ([#38](https://github.com/dirkwhoffmann/Moira/issues/38)).
- 68010:
  - The long (format 8) and short frames are now correct for bus and address errors.
  - RTE now reads a long frame in the same order as the hardware, which fixes its cycle count.
  - Loop mode no longer has the bug it had before.
- 68020:
  - The bus fault frames are now correct, and instructions resume correctly after RTE.
  - Format 1001 is accepted when RTE pops a frame.
  - The cycle timing of the 68020 now uses a cycle budget.
- 68040:
  - The missing instructions are implemented, and the disassembler supports them.
  - RTE accepts the same frame formats as UAE.
  - An address error now writes a frame of format 2, as on the real 68040, instead of the 68030's format B frame.
  - FPU instructions the 68040 doesn't implement (the transcendentals, FMOD, FREM, FSCALE, FINT, FINTRZ and FMOVECR) now raise the unimplemented floating-point instruction exception. The frame has format 2 and holds the address of the next instruction and the effective address, so an FPSP can emulate the instruction.
- 68020 and later:
  - A bus error in a prefetch no longer halts the CPU. Before, reading the instruction words for the stack frame faulted a second time, which was treated as a double bus fault.
- 68030 and 68040:
  - A bus error that the application signals with `BusError(addr)` now gets a complete special status word (read or write, size, function code). Before, only faults detected by the MMU had one, and a failed write on the 68030 got the wrong frame format.
- FPU (all models):
  - FBcc, FDBcc, FScc and FTRAPcc no longer set accrued exception bits other than IOP when they raise BSUN.
  - Arithmetic exceptions that are enabled in FPCR now raise their exceptions (vectors 48 to 54). As on the real FPU, they are taken when the next FPU instruction starts. BSUN is taken by the conditional instruction, and the 68040 takes the exception of an FMOVE to memory right away (format 3 frame). An enabled SNAN, OPERR or DZ exception leaves the destination register unchanged.

Thanks to Arnaud (habib256) for reporting the 68040 and FPU issues ([discussion #36](https://github.com/dirkwhoffmann/Moira/discussions/36)).

## Tests and tools

- New test runners:
  - Moira against MAME, UAE and Musashi, using the SingleStepTests data and random instructions;
  - exception frames against MAME and UAE;
  - interrupt, trace, STOP and RTE sequences against UAE and MAME;
  - bus error retry on the 68030 and 68040;
  - the disassembler against Binutils, Musashi and UAE, in Motorola and MIT syntax.
- The unit tests use doctest and include tests for the debugger, the MMU and the stack frames. The stack frame tests use the recordings from real hardware.
- The CI builds run with ASan and UBSan.
- Two examples are included: `Fibonacci` and `Easy68k`, which runs Easy68K S-record programs.
- The manual has been rewritten (VitePress, with the Doxygen API reference).
- The compile time and the memory footprint are smaller: the jump table and the per-core translation units were reworked, and memory optimizations were merged back from vAmiga ([#37](https://github.com/dirkwhoffmann/Moira/issues/37)).

## Test results

All 26 ctest targets pass:
- unit: 2;
- examples: 3;
- smoke: 1;
- SingleStepTests against MAME, UAE and Musashi: 3;
- exception frames: 6, among them ExceptionMame with 1,261,453 of 1,261,453 cases matching and ExceptionUAE with 1,111,989 of 1,111,989;
- sequences: 6;
- random: 3;
- disassembler (Motorola and MIT syntax): 2.
