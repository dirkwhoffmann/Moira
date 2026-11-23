# Using the Memory Management Unit

The 68030 and 68040 have a built-in memory management unit (MMU). It translates the logical addresses that programs use into physical addresses, and it protects memory regions against unwanted access. Operating systems use it to implement virtual memory, to give each process its own address space, and to protect the kernel.

## Which Models Have an MMU

| CPU model | MMU |
|:----------|:----|
| 68000, 68010, 68EC020, 68020 | None |
| 68EC030 | None |
| 68030 | 68030 MMU |
| 68EC040 | None |
| 68LC040, 68040 | 68040 MMU |

The MMU is selected together with the CPU model. `hasMMU()` tells whether the configured CPU has one. On models without an MMU, the MMU instructions raise the line F exception.

The 68030 and 68040 MMUs are different designs. They have different registers, table formats and instructions. Moira emulates both, including the address translation caches (ATCs) and the transparent translation registers.

## What Changes for Your Application

Your application doesn't have to do anything to support the MMU. After a reset, the MMU is disabled, and logical and physical addresses are the same. The MMU is enabled by the emulated software, which sets up the translation tables and then sets the E bit in the translation control register (TC). Moira informs your application with the delegates `didEnableMMU()` and `didDisableMMU()`.

While the MMU is enabled, the memory access functions (`read8()` ... `write32()`) receive **physical** addresses. The table walks are performed through the same functions, because the translation tables are located in memory, too. Like the real MMU, Moira sets the used and modified bits in the page descriptors.

## Translation Faults

If an address can't be translated, for example because the page is not present or the access violates the write protection, the MMU signals a bus error. Moira writes the stack frame of the emulated model (format $A or $B on the 68030, format $7 on the 68040), and the operating system's handler takes over. Typically, it loads the missing page and returns with `RTE`, which executes the faulted instruction again.

Restarting the instruction requires Moira to save the registers before each instruction. While the MMU is enabled, this is always done, independently of `MOIRA_EMULATE_BUS_ERROR`. See [Emulating Bus Errors](/guides/howto/HowToEmulateBusErrors) for details about the stack frames and the restart.

## Accessing the MMU State

The MMU is a public member of the CPU, `cpu.mmu`. Its registers are accessible through the object of the emulated MMU type:

```c++
if (cpu.hasMMU() && cpu.mmu.getModel() == MMUModel::M68030) {

    auto &mmu = cpu.mmu.mmu030;
    printf("TC = %08x  TT0 = %08x  TT1 = %08x\n", mmu.tc, mmu.tt0, mmu.tt1);
    printf("CRP = %016llx  SRP = %016llx\n", mmu.crp, mmu.srp);
}
```

The 68040 MMU has the registers `tc`, `urp`, `srp`, `itt0`, `itt1`, `dtt0`, `dtt1` and `mmusr` (`cpu.mmu.mmu040`). To change a register that controls the translation, use the setter functions (`setTC()`, `setTT0()`, `setITT0()`, ...), because they also update the internal state of the MMU.

`mmu.isEnabled()` tells whether address translation is currently active.

## Translating Addresses

A debugger often needs to know the physical address that belongs to a logical one. `mmu.peek()` returns the address the CPU would access:

```c++
auto phys = cpu.mmu.peek(addr, FC::USER_DATA);

if (phys) {
    printf("%08x -> %08x\n", addr, *phys);
} else {
    printf("%08x is not mapped\n", addr);
}
```

The function code selects the address space (e.g. `FC::USER_DATA` or `FC::SUPERVISOR_PROG`). An optional third argument checks a write access instead of a read access, which also takes the write protection into account. If translation is disabled, the address is returned unchanged.

`peek()` has no side effects. It uses an entry of the ATC if there is one, as the CPU does, but doesn't change the ATC. If it has to walk the translation tables, it reads the descriptors with `read16Dasm()` and doesn't set the used and modified bits. This makes it safe to call at any time, e.g. while the emulation is paused.

`mmu.translate()`, which the CPU uses internally, is public, too. It has the side effects of a real translation: It loads the ATC, and it sets the used and modified bits in the page descriptors, which changes memory.

## Interaction with the Debugger and the Disassembler

- **Breakpoints** compare the program counter, which is a logical address.
- **Watchpoints** compare the address of the bus access, which is a physical address.
- **The disassembler** reads the instruction words with `read16Dasm()`, passing the address as it is. With the MMU enabled, this is a logical address. If you disassemble code of a program that runs with translation enabled, translate the address with `mmu.peek()` before you look it up.

## Limitations

- **Caches of the 68040.** The instruction and data caches of the 68040 are not emulated. The cache mode bits of the page descriptors and the transparent translation registers have no effect on timing.
- **Timing.** Table walks are not cycle-exact.
