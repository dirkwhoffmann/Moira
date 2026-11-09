# Tests

This folder contains everything that is used to verify Moira. All programs are
built by CMake (`cmake -B build && cmake --build build`) and by the Xcode project.
`ctest` runs the automated ones.

| Folder | Programs | What it checks |
|--------|----------|----------------|
| `Differential/` | `testRunnerUAE`, `testRunnerMusashi` | Runs randomized instructions on Moira and on a reference core (UAE or Musashi) and compares registers, memory and cycles. Also compares the disassembler output with Binutils. A complete run takes about an hour per reference. |
| `SingleStep/` | `singleStepRunner` | Runs the 68000 against the [SingleStepTests](https://github.com/SingleStepTests/m68000) suite (about 260,000 tests recorded from MAME). Checks the final state, the cycle count and every bus access. Needs no arguments and takes a few seconds. |
| `MameRef/` | `mameCheck` | Verifies the MAME reference CPU (MAME's microcoded 68000, see `ThirdParty/MAME`) against the SingleStepTests. It has to report 100%. The reference can trigger bus errors on any access, which no other reference CPU does faithfully. |
| `MameRef/` | `busErrorRunner` | Differential bus error test. Takes the SingleStepTests instructions, lets the n-th bus access of each instruction fail (for every n), and compares Moira (MAME mode) with the MAME reference: stacked frame (PC, IRD, address, code word, SR), registers, cycle count and bus log. Options: `--only <name>`, `--limit <n>`, `-v <n>`, `--kinds data\|fetch\|all`, `--flags` (also compare condition codes), `--exceptions` (include instructions that raise an exception), `--tsv <file>`. |
| `UnitTests/` | `unitTests` | All unit tests in one [doctest](https://github.com/doctest/doctest) executable: FPU arithmetic and formats, FPU and MMU instruction level tests, bus error frames of the 68000, and a comparison of the FPU with UAE's softfloat code. |
| `CpuTester/` | (none) | Test images (ADF and HDF) that have been created with Toni Wilen's cputester. They run inside an Amiga emulator and are not part of the build. |
| `ThirdParty/` | (libraries) | Code and data from other projects that is only used by the tests: UAE, Musashi, MAME's 68000, Binutils, the glue code, and the SingleStepTests data. See the license files in the individual folders. |

## Running the tests

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
./build/Tests/SingleStep/singleStepRunner
./build/Tests/Differential/testRunnerUAE --cpu 68000
ctest --test-dir build
```

Both test runners accept `--help`.

`unitTests` accepts the usual doctest options, e.g. `-ts="MMU"` runs a single
test suite, `-ltc` lists all test cases, and `-tce="*UAE*"` skips the
comparison with UAE. The comparison can be tuned with the environment variables
`FPUREF_ITERATIONS`, `FPUREF_SEED`, `FPUREF_CLEAN` and `FPUREF_MAXPRINT`.

`singleStepRunner` is linked against a special build of Moira that mimics MAME
(`moiraMAME`), because MAME's expected results differ from the real hardware in
a few places. The differential runners are linked against builds that mimic UAE
and Musashi, respectively.
