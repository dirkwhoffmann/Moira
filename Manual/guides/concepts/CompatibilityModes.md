# Compatibility Modes

Moira is tested against other emulators: UAE (the CPU core of WinUAE and Amiberry), MAME's microcoded 68000, and Musashi. For normal program execution, all of them agree with Moira and with each other. In some corner cases, however, they differ: in the content of an exception stack frame, in the state of the registers after a fault, in the order of the bus accesses, or in the number of cycles an instruction takes. In many of these cases, it is unknown what the real CPU does, because the behavior can only be observed with special test hardware.

To compare Moira with a reference emulator in every detail, Moira can mimic its behavior. The compatibility mode is selected at compile time:

```c++
#define MOIRA_COMPATIBILITY DEFAULT
```

## The Modes

| Mode | Behavior |
|:-----|:---------|
| `DEFAULT` | Moira's own behavior. This is the mode for applications. |
| `UAE` | Mimics UAE's CPU core. |
| `MAME` | Mimics MAME's microcoded 68000 core, which produced the [SingleStepTests](https://github.com/SingleStepTests/m68000). |
| `MUSASHI` | Adopts Musashi's instruction timing. |

### DEFAULT

In the default mode, Moira follows the real hardware wherever it is known. Many corner cases have been settled with recordings from real machines (the vAmigaTS test suite, run on an Amiga 500 with a 68000 and with a 68010, and on an Amiga 1200 with a 68020). Where no recording exists, Moira makes the following choices:

- **68000:** Moira follows MAME. MAME's core is microcoded: it executes the same micro-instruction sequences as the real CPU, which makes it the most reliable reference for the 68000.
- **68010 and later:** Moira follows UAE, which is the most thoroughly tested emulator for these models.

### UAE

This mode reproduces UAE's behavior in the cases in which it deviates from MAME or from the real hardware. Examples:

- The program counter that is stacked when an address error or a bus error occurs (e.g., `DBcc` with an odd branch target, `JSR` when pushing the return address fails).
- The state of the address registers after a fault in the `(An)+` and `-(An)` modes, and the condition codes that `MOVE`, `CLR` and some other instructions have already set when a bus error occurs.
- The order of the accesses when `RTE` reads the stack frame.
- Whether `CHK` traps if the comparison overflows (the real 68000 does, UAE doesn't), and the condition codes that `DIVS` leaves on the 68010 in one case.
- The timing of `TRAPV` on the 68010 if the trap is taken: UAE doesn't read the next instruction word and takes 36 cycles instead of 40.

The automated tests use this mode to compare Moira with UAE, including the exception frames, cycle counts and bus accesses of the 68000.

The mode doesn't cover everything in which UAE differs from Moira:

- **Timing of the 68010 and later.** UAE's cycle counts for these models deviate in many places from Motorola's timing tables. Moira doesn't adopt them, and the tests don't compare them.
- **The FPU.** Known deviations of UAE's FPU, e.g. in the sign of some results with infinite operands, or the format 3 frame that UAE uses for an exception after `FMOVE` to memory on the 68881 and 68882, are not reproduced. The tests skip the affected cases instead.

### MAME

This mode reproduces the behavior of MAME's 68000 core in the few cases in which it differs from the real hardware, as recorded on an Amiga 500. MAME's core only emulates the 68000, so the mode is meant for this model. Some of its rules, e.g. for `CHK`, also apply to the other models, so don't use it to emulate them. Examples:

- After an address error in the `(An)+` mode, MAME has already incremented the address register. The real CPU leaves it unchanged.
- When `DBcc` branches to an odd address, MAME stacks the address behind the extension word. The real CPU stacks the address of the extension word.
- When an address error occurs in an indexed addressing mode, the real 68000 decrements the stacked program counter. MAME doesn't.

The SingleStepTests were recorded with MAME. The `smoke` test and the SingleStepTests runner compare Moira in this mode with them.

### MUSASHI

This mode adopts Musashi's timing for a few instructions (e.g., `BCLR`, `BCHG`, `DBcc` and `TRAPV` on the 68010) and for the stopped state. It is used to compare Moira with Musashi cycle by cycle. Musashi's functional flaws are not replicated. The test runner excludes the affected cases instead.

## Which Mode to Use

Use `DEFAULT`. The other modes exist for testing. They reproduce behavior that is known or suspected to differ from the real CPU, so they make the emulation less accurate, not more.

There is one exception: If your application has to produce exactly the same results as WinUAE, for example to compare traces with it, use `UAE`.

## Using Several Modes in One Program

The compatibility mode is a compile-time option, so a library is built for one mode. The test tools build one library per mode and link each into a separate program (see `Tests/CMakeLists.txt`).

To use several modes in the same program, give each library its own namespace with `MOIRA_NAMESPACE`:

```cmake
add_library(moiraUAE STATIC ${MOIRA_SOURCES})
target_compile_definitions(moiraUAE PUBLIC
    MOIRA_NAMESPACE=moiraUAE
    MOIRA_COMPATIBILITY=UAE)
```
