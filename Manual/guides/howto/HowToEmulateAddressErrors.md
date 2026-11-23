# Emulating Address Errors

The 68000 and 68010 can only access words and long words at even addresses. If a program tries to read or write a word or a long word at an odd address, the CPU doesn't perform the access. Instead, it aborts the instruction and processes an address error exception (vector 3). The same happens if the program counter becomes odd, e.g. after a jump or a return to an odd address.

Some programs rely on this behavior. vAmiga enables address error emulation, because some Amiga titles depend on it. Moira emulates address errors if the following option is enabled in `MoiraConfig.h`:

```c++
#define MOIRA_EMULATE_ADDRESS_ERROR true
```

The option is disabled by default, because the checks cost some speed. Unlike bus errors, address errors are detected by the CPU itself, so your application doesn't have to do anything else.

## Which Accesses Cause an Address Error

| Model | Odd data access (word or long word) | Odd program counter |
|:------|:-----------------------------------|:--------------------|
| 68000, 68010 | Address error | Address error |
| 68020 and later | Allowed | Address error |

The 68020 and later can access words and long words at any address, so only an odd program counter causes an address error. Byte accesses never cause one.

If the option is disabled, Moira performs all accesses as requested. In this case, `read16()`, `write16()` and the other memory access functions may receive odd addresses. On the 68020 and later, this is always the case for unaligned accesses, independently of the option.

## The Stack Frame

Moira writes the frame of the emulated model:

| Model | Frame | Content |
|:------|:------|:--------|
| 68000 | Group 0 frame (7 words) | Access information (read or write, instruction or data, function code), fault address, instruction register, status register, program counter |
| 68010 | Format $8 (29 words) | Special status word, fault address and the internal state of the CPU |
| 68020, 68030 | Format $B (46 words) | Special status word, fault address and the internal state of the CPU |
| 68040 | Format $2 (6 words) | Address of the instruction and the fault address |

The exact content of these frames depends on the instruction and on the bus cycle that would have been performed. For example, the program counter that the 68000 stacks is not always the address of the instruction. Depending on the instruction and the addressing mode, it may be up to a few words further. Moira reproduces these details. Many of them have been verified with recordings from real machines (the vAmigaTS test suite), e.g. for `MOVE.L`, `MOVEA`, `MOVEM`, `JSR` and `DBcc`. Where no recording exists, Moira follows MAME on the 68000 and UAE on the later models. The compatibility modes reproduce the deviations of these emulators (see [Compatibility Modes](/guides/concepts/CompatibilityModes)).

## Continuing After the Handler

On the 68000, the group 0 frame doesn't contain enough information to continue the instruction. The handler has to remove the frame and resume elsewhere. This is what operating systems usually do: They terminate the program that caused the error.

On the 68010 and later, `RTE` reads the frame back. However, an address error is usually a programming error, so most handlers don't return either.

## Double Faults

If an address error occurs while the CPU is processing an address error or a bus error, or if the supervisor stack pointer is odd when an exception is processed, the CPU can't write the stack frame. It enters the halted state, as it does after a double bus fault. Moira calls `didHaltCPU()`, and only `reset()` leaves this state.

## Observing Address Errors

Like all exceptions, address errors are reported to the exception delegates as `M68kException::ADDRESS_ERROR` (see [Using the Delegation System](/guides/howto/HowToUseDelegates)). A debugger can catch them with a catchpoint for vector 3 (see [Using the Debugger](/guides/howto/HowToUseTheDebugger)).
