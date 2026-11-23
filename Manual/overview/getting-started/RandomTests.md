# Running the Test Runners

Moira is tested by comparing it with other emulators. The most comprehensive tests are the random test runners: They execute every opcode with random register and memory contents on Moira and on a reference emulator and compare the results. This page shows how to run them. It assumes that you have built Moira as described in [Installation](/overview/Installation).

## The Runners

| Runner | Reference | Models |
|:-------|:----------|:-------|
| `randomuae` | UAE (the CPU core of WinUAE and Amiberry) | All models, including the FPU and the MMU |
| `randommusashi` | Musashi | All models, without the FPU and the MMU translation |
| `randommame` | MAME's microcoded 68000 core | 68000 |
| `dasmmot` | The disassemblers of binutils, Musashi and UAE (Motorola syntax) | All models |
| `dasmmit` | The disassembler of binutils (MIT syntax) | All models |

Each runner links a Moira library that is built in the matching compatibility mode, e.g. `randomuae` uses Moira in UAE mode (see [Compatibility Modes](/guides/concepts/CompatibilityModes)).

## Running a Test

The runners are located in `build/Tests/Random`. Without arguments, they test all models:

```bash
./build/Tests/Random/randomuae
```

The output lists the test stages for each model:

```text
Round 1:

      68040 CPU : CORE ................................ PASSED  (Moira: 0.34s  UAE: 0.34s)
      68040 MMU : CORE ................................ PASSED  (Moira: 0.00s  UAE: 0.00s)
      68040 MMU : XLAT ................................ PASSED  (Moira: 0.00s  UAE: 0.00s)
                      23016 translations, 75015 faults
      68040 FPU : CORE ................................ PASSED  (Moira: 3.61s  UAE: 4.22s)

All tests completed
```

The stages cover different parts of the CPU:

- **CPU : CORE** executes all opcodes (except the FPU opcodes) four times with different random setups.
- **MMU : CORE** executes the MMU instructions (68030 and 68040 models).
- **MMU : XLAT** sets up random translation tables and compares the address translation of both MMUs.
- **FPU : CORE** executes the FPU instructions with all extension words and a pool of interesting floating-point values. It runs once for each FPU that can be combined with the CPU model (68881, 68882, or the FPU of the 68040).

After each instruction, the runner compares the registers, the status register, the stack pointers, the FPU and MMU registers, the written memory and, where the reference is reliable, the number of cycles.

## Options

| Option | Description |
|:-------|:------------|
| `--cpu <model>` | Tests the given CPU model only. Can be repeated. Models: `68000`, `68010`, `EC020`, `68020`, `EC030`, `68030`, `EC040`, `LC040`, `68040` |
| `--fpu <model>` | Tests the given FPU only in the FPU stage. Can be repeated. Models: `68881`, `68882`, `builtin` |
| `--seed <n>` | Seed of the random number generator. By default, the seed is derived from the date, so each day tests different values. |
| `--rounds <n>` | Number of test rounds (default: 1). Each round uses new random values. |

For example, the following command tests the 68030 with a 68882 and a fixed seed:

```bash
./build/Tests/Random/randomuae --cpu 68030 --fpu 68882 --seed 42
```

A fixed seed makes a test reproducible. The automated tests (`ctest`) use the seed 1.

## Run Times

A round with all models takes about ten minutes for `randomuae`, about two minutes for `randommusashi` and a few seconds for `randommame`. The disassembler runners take considerably longer, because they disassemble every opcode in several syntaxes: `dasmmot` about half an hour, `dasmmit` about 20 minutes. To test a change quickly, restrict the run to a single model with `--cpu`.

## When a Test Fails

If Moira and the reference disagree, the runner stops and prints a report:

```text
FLOATING-POINT REGISTER MISMATCH FOUND

Instruction:      UAE: [6] 'fsincos.s (-$1000,a2),fp0,fp0'
                Moira: [6] 'fsincos.s (-$1000,a2),fp0,fp0'

Setup:   PC: 1000  Opcode: f22a  Ext1: 4430  Ext2: f000  Ext3: 0000 (SUPERVISOR MODE)
...
FP0: setup 0x3FEF:8000000100000000  UAE 0x7FFF:4A00B90000000000  Moira 0x3FEF:8000000100000000
...
```

The report shows the instruction, the initial state (the setup) and the final state on both emulators. Run the runner again with the same seed and model to reproduce the case.

Not every difference is a bug in Moira. The reference emulators have known flaws, and some behavior is undefined. These cases are listed in the reference classes (e.g. `Tests/Random/UAERef.cpp`), each with a comment that explains why it is skipped. If you find a difference that is not listed, please report it on [GitHub](https://github.com/dirkwhoffmann/Moira/issues).

## Other Tests

The random runners are part of a larger test suite, which also replays the [SingleStepTests](https://github.com/SingleStepTests/m68000), compares exception frames, interrupt sequences and bus error recovery with the reference emulators, and runs unit tests. `Tests/README.md` describes all tools. The whole suite is run with CTest (see [Installation](/overview/Installation)).
