# Emulating Bus Errors

On a real system, external hardware terminates a bus cycle with a bus error if no device responds to the address or if the access is not allowed. The CPU then aborts the instruction and processes exception vector 2. Operating systems use this mechanism to detect missing hardware, to implement virtual memory and to protect memory regions.

Moira lets your application signal a bus error from any memory access function. The CPU writes the stack frame of the emulated model, runs the exception handler and, depending on the model, continues the faulted instruction after the handler returns with `RTE`.

## Signaling a Bus Error

A bus error is signaled by throwing a `BusError` from one of the memory access functions (`read8`, `read16`, `write8`, `write16`, and `read32` / `write32` on models with a 32-bit bus). In most cases, the exception only needs to carry the fault address:

```c++
u16 MyCPU::read16(u32 addr) const
{
    if (!isMapped(addr)) throw BusError(addr);
    return memory.read16(addr);
}

void MyCPU::write16(u32 addr, u16 val) const
{
    if (isReadOnly(addr)) throw BusError(addr);
    memory.write16(addr, val);
}
```

Moira catches the exception and completes the stack frame itself. This is the preferred way, because the content of the frame (the stacked program counter, the instruction register, the status register and the access information) depends on the executed instruction, its addressing mode and the bus cycle that failed. For example, a fault in a write cycle produces a different frame than a fault in a read cycle, and a fault in the second half of a long word access produces a different frame than a fault in the first half.

If you need full control, you can create the stack frame yourself and pass it to the constructor:

```c++
StackFrame frame = {};
frame.addr = addr;
frame.sr = mySR;
// ...
throw BusError(frame);
```

Moira never modifies a frame that has been supplied this way. This is mainly useful on the 68000, whose frame layout is simple. On the other models, let Moira create the frame.

## Configuring Bus Error Emulation

Whether bus errors need to be enabled depends on the CPU model:

- **68000 and 68010**

  A `BusError` is always processed. No configuration is needed and there is no performance penalty.

- **68020 and later**

  These models can restart the faulted instruction after the handler returns with `RTE`. To support this, Moira has to save the registers before each instruction, which forces the CPU onto the slower execution path. This is enabled by the following option in `MoiraConfig.h`:

  ```c++
  #define MOIRA_EMULATE_BUS_ERROR true
  ```

  If the option is disabled, a `BusError` that is thrown anyway is still processed, but the instruction cannot be restarted. While the MMU is enabled, the option has no effect: An address translation fault is a bus error that must be restartable, so Moira always saves the registers in this case.

Bus errors are independent of address errors, which are enabled with `MOIRA_EMULATE_ADDRESS_ERROR` (see [Emulating Address Errors](/guides/howto/HowToEmulateAddressErrors)).

## The Stack Frame

Moira writes the frame of the emulated model:

| Model | Frame | Content |
|:------|:------|:--------|
| 68000 | Group 0 frame (7 words) | Access information, fault address, instruction register, status register, program counter |
| 68010 | Format $8 (29 words) | Special status word, fault address and the internal state of the CPU |
| 68020 | Format $B (46 words) | Special status word, fault address, data output buffer and the internal state of the CPU |
| 68030 | Format $A (16 words) for a fault in a data write, format $B (46 words) otherwise | Special status word, fault address, data output buffer and pipe stages B and C |
| 68040 | Format $7 (30 words, access error frame) | Special status word, fault address and the write-back data of a faulting write |

The fault address is the logical address of the failed access. A handler reads the frame like the handler of a real system, so existing system software works without modifications.

## Continuing After the Handler

What happens when the handler returns with `RTE` depends on the model:

- **68000**

  The group 0 frame does not contain enough information to continue the instruction. The handler has to remove the frame and resume elsewhere, for example by terminating the faulting program.

- **68010**

  The format $8 frame holds the internal state of the CPU. `RTE` reads it back and continues the instruction where it was interrupted.

- **68020, 68030 and 68040**

  Moira uses a restart model: The registers are restored to their state at the start of the instruction, and `RTE` executes the instruction again. This covers the common case, in which the handler fixes the cause of the fault (e.g., maps the missing page) and returns. A handler that completes the faulted access itself and clears the corresponding bits in the special status word is not supported. In this case, the instruction is executed a second time.

## Double Bus Faults

If another bus error occurs while the CPU is processing a bus error, for example because the stack pointer points to unmapped memory, the CPU enters the halted state. Moira calls the `didHaltCPU()` delegate, and `isHalted()` returns true. While halted, `execute()` only advances the clock. Only `reset()` leaves this state.

## Observing Bus Errors

Like all exceptions, bus errors are reported to the exception delegates (see [Using the Delegation System](/guides/howto/HowToUseDelegates)):

```c++
void MyCPU::willExecute(M68kException exc, u16 vector)
{
    if (exc == M68kException::BUS_ERROR) {
        printf("Bus error at PC %08x\n", getPC0());
    }
}
```

`willExecute()` is called before the stack frame is written, `didExecute()` after the CPU has jumped to the handler.
