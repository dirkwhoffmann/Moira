# Tests

This folder contains everything that is used to verify Moira. All programs are
built by CMake (`cmake -B build && cmake --build build`) and by the Xcode project.
`ctest` runs the automated ones.

| Folder | Programs | What it checks |
|--------|----------|----------------|
| `Differential/` | `testRunnerUAE`, `testRunnerMusashi` | Runs randomized instructions on Moira and on a reference core (UAE or Musashi) and compares registers, memory and cycles. Also compares the disassembler output with Binutils. A complete run takes about an hour per reference. |
| `SingleStep/` | `singleStepRunner` | Runs the 68000 against the [SingleStepTests](https://github.com/SingleStepTests/m68000) suite (about 260,000 tests recorded from MAME). Checks the final state, the cycle count and every bus access. Needs no arguments and takes a few seconds. |
| `FPU/` | `fpuTests`, `fpuExecTests`, `fpuRefTests` | Unit tests of the FPU, instruction level tests, and a comparison of the arithmetic with UAE's softfloat code. |
| `MMU/` | `mmuExecTests` | Instruction level tests of the 68030 and 68040 MMU. |
| `CpuTester/` | (none) | Test images (ADF and HDF) that have been created with Toni Wilen's cputester. They run inside an Amiga emulator and are not part of the build. |
| `ThirdParty/` | (libraries) | Code and data from other projects that is only used by the tests: UAE, Musashi, Binutils, the glue code, and the SingleStepTests data. See the license files in the individual folders. |

## Running the tests

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
./build/Tests/SingleStep/singleStepRunner
./build/Tests/Differential/testRunnerUAE --cpu 68000
ctest --test-dir build
```

Both test runners accept `--help`.

`singleStepRunner` is linked against a special build of Moira that mimics MAME
(`moiraMAME`), because MAME's expected results differ from the real hardware in
a few places. The differential runners are linked against builds that mimic UAE
and Musashi, respectively.
