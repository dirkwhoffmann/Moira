# Using the Floating-Point Unit

Moira emulates the floating-point units of the 68k family: the MC68881 and MC68882 coprocessors, which are connected to a 68020 or 68030, and the FPU that is built into the 68040.

## Selecting an FPU

Whether an FPU is present depends on the CPU model and on the coprocessor that is connected to it:

| CPU model | FPU |
|:----------|:----|
| 68000, 68010 | None |
| 68EC020, 68020, 68EC030, 68030 | Optional 68881 or 68882 coprocessor |
| 68EC040, 68LC040 | None |
| 68040 | Built in |

A coprocessor is connected with `setCPModel()`:

```c++
cpu.setModel(CPUModel::M68030);
cpu.setCPModel(CPModel::M68882);
```

The selection is independent of the CPU model, so `setModel()` and `setCPModel()` can be called in any order. On CPUs without a coprocessor interface, the selection is remembered but has no effect. Use `hasFPU()` to check whether the configured CPU has a working FPU, and `hasInternalFPU()` and `hasExternalFPU()` to tell the two kinds apart.

Without an FPU, all floating-point instructions raise the line F exception (vector 11). This allows the system software to emulate them, as it would on a real machine.

## Accessing the FPU State

The FPU is a public member of the CPU, `cpu.fpu`. It holds the eight data registers and the three control registers:

```c++
// Print the data registers
for (int i = 0; i < 8; i++) {
    printf("FP%d = %Lg\n", i, cpu.fpu.fpr[i].toLongDouble());
}

// Print the control registers
printf("FPCR = %08x FPSR = %08x FPIAR = %08x\n",
       cpu.fpu.getFPCR(), cpu.fpu.getFPSR(), cpu.fpu.getFPIAR());
```

The data registers have the type `MFloat80`, Moira's 80-bit extended precision type. `toLongDouble()` and `fromLongDouble()` convert from and to the host's `long double`. The conversion is exact only if the host's `long double` is an 80-bit type (e.g. on x86). For exact access on all hosts, use the raw bits (`signExp()` and `mantissa()`, or `MFloat80::fromBits()`).

The registers can be modified, too. A debugger, for example, can load a value into FP0 like this:

```c++
cpu.fpu.fpr[0] = MFloat80::fromLongDouble(3.14159265358979323846L);
```

## Accuracy

The FPU doesn't use the floating-point hardware of the host. All arithmetic is carried out by Moira's own 80-bit implementation (`MFloat80` and `FpuEnv`), which rounds like the real FPU in all rounding modes and precisions and sets the same status bits. Therefore, the results are identical on all platforms. The implementation is checked against the softfloat FPU of UAE (see the unit tests in `Tests/Unit/FpuRefTests.cpp`).

The FPU executes the instructions of the emulated model:

- The 68881 and 68882 implement the full instruction set, including the transcendental functions (`FSIN`, `FETOX`, `FLOGN`, ...) and the constant ROM (`FMOVECR`).
- The 68040 adds instructions that round to single or double precision (`FSADD`, `FDMUL`, ...). On the 68881 and 68882, these raise the line F exception.

## The 68040 FPU

To save chip space, the FPU of the 68040 implements only the most common instructions in hardware. Many others, such as the transcendental functions, `FMOD`, `FREM`, `FSCALE`, `FINT`, `FINTRZ` and `FMOVECR`, are left to software. Motorola provided a software package, the Floating-Point Software Package (FPSP), that the operating system installs to emulate them.

Moira emulates this behavior. An unimplemented instruction raises the unimplemented floating-point instruction exception (vector 11) with a stack frame of format 2. The frame holds the address of the next instruction and the effective address of the operand, which the FPSP needs to emulate the instruction. Before the exception is raised, the extension words and the source operand are read, so the address registers of the `(An)+` and `-(An)` modes are already updated, and FPIAR points to the instruction. Your application doesn't need to do anything special: Software written for the 68040 brings its own FPSP.

The exception is reported to the exception delegates as `M68kException::FPU_UNIMPLEMENTED` (see [Using the Delegation System](/guides/howto/HowToUseDelegates)).

## Saving and Restoring the FPU State

Operating systems save the internal state of the FPU with `FSAVE` and restore it with `FRESTORE`, e.g. when switching tasks. Moira writes a null frame if the FPU is in its reset state and an idle frame otherwise. The size of the idle frame depends on the model, as on the real hardware (68881: 28 bytes, 68882: 60 bytes, 68040: 4 bytes), so software that inspects the frame sees the values it expects. `FRESTORE` accepts the frames that `FSAVE` writes.

## Limitations

- **Arithmetic exceptions.** The exception bits in FPSR are set as on the real FPU, but enabling an exception in FPCR doesn't raise the corresponding trap (vectors 48 to 54). Software that relies on these traps doesn't work as expected.
- **Packed decimal on the 68040.** The 68040 doesn't support the packed decimal format in hardware. `FMOVE.P` should raise the unimplemented data type exception, but Moira executes it like a 68881 or 68882.
- **Timing.** FPU instructions are not cycle-exact.
