# Tests

This folder contains everything that is used to verify Moira. All programs are
built by CMake (`cmake -B build && cmake --build build`) and by the Xcode project.
`ctest` runs the automated ones.

| Folder | Programs | What it checks |
|--------|----------|----------------|
| `Unit/` | `unit` | All unit tests in one [doctest](https://github.com/doctest/doctest) executable: FPU arithmetic and formats, FPU and MMU instruction level tests, bus error frames of the 68000, and a comparison of the FPU with UAE's softfloat code. |
| `Smoke/` | `smoke` | Runs the 68000 against the [SingleStepTests](https://github.com/SingleStepTests/m68000) suite (about 260,000 tests recorded from MAME). Checks the final state, the cycle count and every bus access. Needs no arguments and takes a few seconds. |
| `SST/` | `sstmame`, `sstmusashi`, `sstuae`, `exceptionmame`, `exceptionuae` | Runs the instructions of the SingleStepTests suite on Moira and on a real reference core (MAME, Musashi or UAE) and compares the results. The prerecorded results are ignored, so every compatibility mode can be checked against its own reference. Compares registers, memory, cycle counts and (if the reference reports them) bus cycles, including the cases that end in an address error. Takes a few seconds. `sstmame` has to give the same result as `smoke`. All three tools reach 100%. `sstuae` does not compare the I/N bit (bit 3) of the address error frame code word, which UAE sets by rules Moira does not model. **Exception tests** (`exceptionmame`, `exceptionuae`, built from the same sources): Differential exception test. Takes the SingleStepTests instructions and compares Moira with a reference CPU when an exception hits in the middle of an instruction: bus errors on every bus access, address errors provoked by perturbing the initial state (odd A7, address registers, extension words, return addresses, ...), and double faults (halted CPU). `exceptionmame` runs Moira in MAME mode against MAME's microcoded 68000, `exceptionuae` runs Moira in UAE mode against WinUAE's cycle-exact 68000. Compared are the stacked frame, registers, status register, cycle count, bus log and written memory. Options: `--only <name>`, `--limit <n>`, `-v <n>`, `--dedupe <n>` (at most n cases per instruction form), `--coverage` (list instruction forms without a case), `--check` (run the reference alone against the SingleStepTests, which verifies the MAME reference CPU), `--ignore ccr,cycles,nbit` (skip condition codes, timing, or the I/N bit of bus error frames), `--tsv <file>`. `exceptionuae` skips cases that fault while a group 1/2 exception is being processed, because UAE and MAME disagree on those frames. `exceptionuae --cpu 68020` runs the same cases against UAE's 68020 core (no timing, bus errors on data accesses only, one per operand access). Of the 68020 frames only the SR, PC, format word and fault address are compared, because UAE leaves the internal words empty and sets the SSW only partially. Single frame fields can be skipped with `--ignore frame.addr,frame.stagec,...`. |
| `Sequence/` | `sequenceuae` | Differential test of the interrupt machinery. Generates random programs (`--programs <n>`, `--seed <n>`) that mix ordinary instructions with `stop`, `rte`, `trap`, `chk`, trace mode and changing interrupt levels, and runs them on Moira and on WinUAE's 68000/68010 core in lockstep. After every instruction the registers, SR, stack pointers, stopped state, pending exceptions and written memory are compared. Covers IPL sampling, level 7 edge triggering, trace, STOP wake-up and RTE sequencing. Programs that end in an address error are not compared (UAE's 68010 frames are unreliable). Options: `--cpu 68000|68010`, `--min <percent>` (required match rate, used by ctest), `--program <n>`, `-v <n>`. Takes a few seconds. |
| `Random/` | `randomuae`, `randommusashi`, `randommame`, `dasmmot`, `dasmmit` | Runs randomized instructions on Moira and on a reference core (UAE, Musashi or MAME; MAME covers the 68000 only and skips address errors, which `exceptionmame` covers) and compares registers, memory and cycles. A complete run takes about an hour per reference. The reference cores sit behind the `Reference` facade (`Random/Reference.h`), which also holds the known differences that are tolerated per reference. The seed is derived from the date unless `--seed <n>` is given. **Disassembler tests** (`dasmmot`, `dasmmit`, built from the same sources, same options): Disassembler tests. Every opcode is disassembled by Moira in several syntaxes and compared with reference disassemblers. `dasmmot` (Motorola syntax) compares `Syntax::GNU` with Binutils, `Syntax::MUSASHI` with Musashi and `Syntax::UAE` with UAE. `dasmmit` (MIT syntax) compares `Syntax::GNU_MIT` with a Binutils that is compiled without `MOTOROLA`. Moira's own syntaxes (`MOIRA`, `MOIRA_MIT`) have no reference and are only checked for plausible output. |
| `Common/` | (sources) | Code shared by all tools (`Common.h/.cpp`: banner, command line parser, progress dots, random numbers) and the SingleStepTests file reader. |
| `CpuTester/` | (none) | Test images (ADF and HDF) that have been created with Toni Wilen's cputester. They run inside an Amiga emulator and are not part of the build. |
| `ThirdParty/` | (libraries) | Code and data from other projects that is only used by the tests: UAE, Musashi, MAME's 68000, Binutils, the glue code, and the SingleStepTests data. See the license files in the individual folders. |

## Running the tests

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
./build/Tests/Smoke/smoke
./build/Tests/Random/randomuae --cpu 68000
ctest --test-dir build
ctest --test-dir build -L smoke      # only the tests with a label (unit, smoke, sst, random, exception, dasm)
```

The random runners, the disassembler tests and the exception tests accept `--help`. The automated random and disassembler runs use a fixed seed (`--seed 1`) so that they are reproducible.

`unit` accepts the usual doctest options, e.g. `-ts="MMU"` runs a single
test suite, `-ltc` lists all test cases, and `-tce="*UAE*"` skips the
comparison with UAE. The comparison can be tuned with the environment variables
`FPUREF_ITERATIONS`, `FPUREF_SEED`, `FPUREF_CLEAN` and `FPUREF_MAXPRINT`.

`smoke` is linked against a special build of Moira that mimics MAME
(`moiraMAME`), because MAME's expected results differ from the real hardware in
a few places. The random runners are linked against builds that mimic UAE
and Musashi, respectively.
