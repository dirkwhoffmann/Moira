# How to use the CPU core

To integrate Moira into your own application, copy the contents of subfolder `Moira` to your application. All other files are only needed to create the test application.

## Creating a custom CPU

By default, Moira is configured to declare all functions that require a custom implementation as virtual. By doing so, Moira follows the basic C++ design principle which means that a custom CPU is created by sub-classing. A minimal custom CPU looks like this:

```c++
    #include "Moira.h"
    
    class CPU : public moira::Moira {

        u8 read8(u32 addr) override;
        u16 read16(u32 addr) override;
        void write8 (u32 addr, u8  val) override;
        void write16 (u32 addr, u16 val) override;
    };
```

`Moira` is an abstract class that requires the implementation of the four mentioned methods. These functions provide the basic functionality for transferring bytes or words between the CPU and memory. 

Alternatively, Moira can be configured to declare all interfacing functions as non-virtual to speed up emulation. This solution has been chosen by vAmiga to achieve maximum speed.

After instantiating an object of your custom CPU class, the CPU needs to be configured. The most important property is the emulated CPU type which can be selected by the following API functions:

- `void setModel(Model cpuModel)`

  Currently supported CPU models are: 

  - `M68000`
  - `M68010` 
  - `M68EC020` 
  - `M68020`

  This functions configures both the CPU core and the disassembler. 

- `void setModel(Model cpuModel, Model dasmModel)`

  This function configures the CPU core and the disassembler seperately. The function has been intentionally overloaded to let the disassembler support a broader range of cpu models. Whereas the emulated CPU core is restricted to the models mentioned above, the disassembler is capable of producing output for higher-end CPUs, too. These CPUs are specified by the following keys: 

  - `M68EC030`
  - `M68030`
  - `M68EC040`
  - `M68LC040`
  - `M68040`

## Running the custom CPU 

After the class has been instantiated, the CPU is ready to use. The following two API functions are the main entry points for running the device:  

- `void reset()`

  Initiates the CPU's internal reset procedure.

- `void execute()`

  Executes the CPU for one instruction.

Please make sure to reset the CPU before executing any instruction.

Internally, Moira holds a variable called `clock` which counts the number of CPU cycles that have elapsed since power-up. The following code utilizes the `clock` variable to run the CPU until a certain amount of cycles have been executed:

````c++
i64 now = cpu.getClock();
while (cpu.getClock() - now < 1000) {
  cpu.execute();
}
````

Keep in mind that Moira always executes a single instruction as a whole. This means that the above code is likely to execute slightly more than 1000 cycles in most cases.

## Querying CPU properties 

The `Moira` class provides a variety of functions for querying the CPU state. The following incomprehensive list contains the most important ones: 

- `u32 getD(int n) const`
    
  Reads the current value from a data register.

- `u32 getA(int n) const`

  Reads the current value from an address register.

- `u32 getPC() const`

  Reads the current value of the program counter. Keep in mind that the program counter is a moving target. It progresses while an instruction is executed. 

- `u32 getPC0() const`

  Returns the address of the currently executed instruction. Unlike the program counter, the instruction address always points to the start address of the current instruction. It matches the program counter when execution begins and remains unchanged until the next instruction is processed. In most scenarios, `getPC0` is the function you want to call, and not `getPC`. 

- `u8 getCCR() const`

  Returns the CCR part of the status register. 

- `u16 getSR() const`

  Returns the entire status register, including the CCR part. 

## Managing interrupts 

The Motorola 68000 CPU has three interrupt pins that are accessible from outside. The values of these pins encode the so-called Interrupt Priority Level (IPL) which ranges from 0 to 7 and is set to 0 on reset. The IPL pins can be accessed with the following API functions: 

- `u8 getIPL()`

  Returns the current interrupt level set on the IPL pins.

- `void setIPL(u8 level)`

  Sets the IPL pins to the specified value.

## Synchronizing the CPU

One of the key features of Moira is to provide information about the number of cycles that have passed between two bus accesses. This information is transmitted via the following delegation function:

- `void sync(int cycles)`

  This function is called each time the CPU accesses memory. The passed integer value indicates the number of cycles that have elapsed since the last call of this function. The default implementation simply adds up the passed cycle counts:

  ````c++
  virtual void sync(int cycles) 
  { 
      clock += cycles;  
  }
  ````

  In vAmiga this function is utilized to emulate bus sharing between the CPU and the custom chip set. As soon as the CPU tries to access memory, vAmiga intercepts the access in `sync()` and emulates the external hardware up to the point in time the CPU has already reached. When the actual memory access happens, vAmiga can determine exactly whether the bus is blocked or not. If it is, the CPU is virtually suspended by increasing variable `clock` by a certain number of wait states.

## Tweaking memory access

 Keep in mind that during the reset process the Motorola 68000 CPU reads the initial stack pointer and program counter from memory. This means that the memory must be fully initialized before calling reset(). However, you can easily work around this restriction by implementing the following function in your subclass:

- `u16 read16OnReset(u32 addr)`

  Once implemented, Moira accesses memory with this function during reset. Hence, the initial stack pointer and the program counter can be passed to the CPU even without memory being ready for operation.

The disassembler is subject to a similar issue. By default it calls `read16()` to read data words from memory. This default behaviour is not adequate, e.g., for emulating an Amiga, because reading certain registers causes side effects. In Moira this problem can be avoided by redirecting the disassembler to another read function. This is done by implementing

- `u16 read16Dasm(u32 addr)`

  If this function is implemented in your subclass, it should return the exact same values as the already implemented function `read16()` would. Only side effects which would occur on real hardware are not carried out.