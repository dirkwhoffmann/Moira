# Integrating the CPU Core

This guide explains how to integrate Moira into your application, create a custom CPU, and manage execution, memory access, and interrupts.

To use Moira in your application, copy the contents of the Moira subfolder into your project. Other files are only needed for the test application.

## Creating a Custom CPU

By default, Moira declares all necessary interface functions as virtual, allowing for easy subclassing. A minimal custom CPU looks like this:

```c++
    #include "Moira.h"
    
    class CPU : public moira::Moira {

        u8 read8(u32 addr) override;
        u16 read16(u32 addr) override;
        void write8 (u32 addr, u8  val) override;
        void write16 (u32 addr, u16 val) override;
    };
```

The Moira class is abstract, requiring the implementation of the four mentioned methods. They handle memory access by reading and writing bytes and words.

Alternatively, for better performance, Moira can be configured to use non-virtual functions (as done in vAmiga).

## Configuring the CPU

After instantiating your CPU class, configure the CPU model:

- `void setModel(Model cpuModel)`

  The following models are supported: 

  - `M68000`
  - `M68010` 
  - `M68EC020` 
  - `M68020`

  This function configures both the CPU core and disassembler. To configure them separately: 

- `void setModel(Model cpuModel, Model dasmModel)`

  This function configures the CPU core and the disassembler seperately. The function has been intentionally overloaded to let the disassembler support a broader range of CPU models. Whereas the emulated CPU core is restricted to the models mentioned above, the disassembler supports higher-end CPUs. These CPUs are specified by the following keys: 

  - `M68EC030`
  - `M68030`
  - `M68EC040`
  - `M68LC040`
  - `M68040`

## Running the Custom CPU 

After instantiation, the CPU is ready to use. The two key functions are:

- `void reset()`

  Runs the CPU's internal reset procedure.

- `void execute()`

  Emulates the CPU for one instruction.

Make sure to reset the CPU before executing any instruction.

The following functions are convenient wrappers around `execute()`:

- `void execute(i64 cycles)`

  Runs the CPU for the given number of cycles. Be aware that the number of 
  actually elapsed cycles may exceed the specified cycle budget since the 
  emulator cannot stop in the middle of an instruction.
  
- `void executeUntil(i64 cycle)`

  Executes instructions until a specific cycle count is reached.

## Querying CPU Properties 

Moira provides various functions to inspect the CPU state. The following incomprehensive list contains the most important ones: 

- `u32 getD(int n) const`
    
  Reads the current value from a data register.

- `u32 getA(int n) const`

  Reads the current value from an address register.

- `u32 getPC() const`

  Reads the current value of the program counter. Keep in mind that the program counter is a moving target. It may progress multiple times during the execution of an instruction. 

- `u32 getPC0() const`

  Returns the address of the currently executed instruction. Unlike the program counter, the instruction address always points to the start address of the current instruction. It matches the program counter when execution begins and remains unchanged until the next instruction is processed. In most scenarios, `getPC0` is the function you want to call rather than `getPC`. 

- `u8 getCCR() const`

  Returns the CCR part of the status register. 

- `u16 getSR() const`

  Returns the entire status register, including the CCR part. 

## Managing Interrupts 

The Motorola 68000 CPU has three interrupt pins that define the Interrupt Priority Level (IPL), which ranges from 0 to 7 and is set to 0 on reset.

- `u8 getIPL()`

  Returns the current interrupt level encoded on the IPL pins.

- `void setIPL(u8 level)`

  Sets the IPL pins to the specified value.

## Synchronizing the CPU

Moira provides cycle-accurate timing via the following function:

- `void sync(int cycles)`

  The function is called each time the CPU accesses memory. The passed integer value indicates the number of cycles that have elapsed since the last call of this function. The default implementation simply adds up the passed cycle counts:

  ````c++
  virtual void sync(int cycles) 
  { 
      clock += cycles;  
  }
  ````

  In vAmiga this function is utilized to emulate bus sharing between the CPU and the custom chip set. As soon as the CPU tries to access memory, vAmiga intercepts the access in `sync()` and emulates the external hardware up to the point the CPU has already reached. Consequently, when the actual memory access happens, vAmiga can determine exactly whether the bus is blocked. If it is, the CPU is virtually suspended by increasing variable `clock` by a certain number of wait states.

## Tweaking Memory Access

During the reset process, the Motorola 68000 CPU reads the initial stack pointer from address 0x000000 and the program counter from address 0x000004. This means that memory must be fully initialized before calling `reset`, which can be inconvenient in certain situations. To work around this restriction, Moira provides a dedicated function that can be implemented in a subclass. By implementing the following function, it becomes possible to supply the required values without requiring the entire memory system to be set up:

- `u16 read16OnReset(u32 addr)`

A similar issue arises when the disassembler reads memory. By default, it uses `read16` to fetch instruction words, but this approach can be problematic when emulating systems like the Amiga. Some memory-mapped registers trigger side effects when accessed, and these side effects should not occur when simply disassembling instructions. To prevent unintended behavior, Moira allows the disassembler to use a separate function:

- `u16 read16Dasm(u32 addr)`

If your subclass implements this function, it should return the exact same values as the already implemented function `read16`. However, no side effects should be carried out.
