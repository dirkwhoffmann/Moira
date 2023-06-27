# Using Moira in your own app

This tutorial shows how to integrate Moira into your own application. It explains the basic steps by showing how Moira is used in vAmiga, the Amiga emulator Moira was originally developed for.

## Configuring Moira

We start by examining the compile-time options chosen by vAmiga. The following options are set in `MoiraConfig.h`:

- `#define PRECISE_TIMING true`

Moira is run in precise timing mode, which means that a `sync` call is initiated before each memory access. This gives vAmiga the ability to emulate the surrounding logic up to the point in time when the memory access actually  happens. This is an essential feature in Amiga emulation, as many applications rely on accurate bus timing. In fact, the lack of this feature in Musashi was the reason I implemented Moira in the first place. Note that this feature is only available for the M68000 and M68010. M68020 emulation is always done in simple cycle mode, regardless of how this preprocessor variable is set. 

- `#define VIRTUAL_API false`

All functions of the Moira class that are required to be implemented by the client (vAmiga) are declared as non-virtual. This means these functions will be statically linked, resulting in a small performance boost compared to virtual functions. 

- `#define EMULATE_ADDRESS_ERROR true`

The M68000 and the M68010 signal an address error when a word or longword access to an odd address is attempted. Since some Amiga titles rely on this behavior, address error emulation is enabled in vAmiga. 

- `#define EMULATE_FC true`

All M680x0 CPUs are equipped with three so-called function code pins (FC pins). Whenever memory is accessed, the value on these pins allows external hardware to check the access type. Although the Amiga does not make use of those pins, emulation is enabled in Moira to accurately emulate the M68010 CPU. Unlike the M68000, the M68010 includes the current value of the FC pins in some stack frames. 

- `#define ENABLE_DASM true`

This option instructs Moira to create a lookup table with all disassembler handlers. The creation of this table is mandatory for using the disassembler. If the disassembler is not needed, it is advisable to set this option to `false`, since the lookup table consumes a decent amount of memory.

- `#define BUILD_INSTR_INFO_TABLE false`

During initialization, Moira is able to create an optional info table that stores information about the instruction, addressing mode, and size attribute for each of the 65536 opcode words. In vAmiga, the creation of this table is omitted because the information is not needed. 

- `#define MIMIC_MUSASHI false`

This option instructs Moira *not* to run in Musashi compatibility mode, resulting in higher accuracy. 

## Subclassing Moira

Having learned how Moira's compile-time options are configured, we take a deeper look on how Moira is integrated in vAmiga. The most prominent class in vAmiga is the `Amiga` class. It has the following structure: 

```c++
class Amiga : public Thread {

    ...
    // Core components
    CPU cpu = CPU(*this);
    CIAA ciaA = CIAA(*this);
    CIAB ciaB = CIAB(*this);
    Memory mem = Memory(*this);
    Agnus agnus = Agnus(*this);
    Denise denise = Denise(*this);
    Paula paula = Paula(*this);
    ...
};
```

vAmiga's `CPU` class is a subclass of the Moira class. It's main purpose is to add additional functionality to what Moira already offers. E.g., the CPU class provides capabilities for serializing and deserializing the CPU state which is important for saving and loading emulator states (snapshots). In addition, the CPU class enhances the Moira class with overclocking support. If you are interested in how overclocking is implemented in vAmiga, please refer to the How-to section. 

In `CPU.cpp` we find the implementations of all Moira functions that interact with the environment. Since we have declared the API as non-virtual, none of these functions have a default implementation. Therefore, all functions must be implemented even if there is nothing to do. Thus, many functions are defined with an empty function body. E.g.:

```c++
void
Moira::didReset()
{

}
```

This function is one of the delegation methods Moira provides. It is called at the end of the reset handler to allow the client to perform special actions.

Other functions do have a non-empty implementation such as functions for accessing memory. E.g., this is how `read8` and `read16` are implemented in vAmiga:

```c++
u8
Moira::read8(u32 addr) const
{
    return mem.peek8<ACCESSOR_CPU>(addr);
}

u16
Moira::read16(u32 addr) const
{
    return mem.peek16<ACCESSOR_CPU>(addr);
}
```

At first glance, the code seems to speak for itself. Moira's memory request is passed to the `mem` object, which is an instance of vAmiga's `Memory` class. When looking closer, however, this straightforward usage of vAmiga's memory class brings up the following question: How does Moira know about the various Amiga components such as the memory? These objects are not part of the Moira class. Well, the answer is they are, because in vAmiga we use a slightly modified Moira class. In fact, the class is declared as follows:

```c++
class Moira : public SubComponent {
    ...
}
```

As you can see, Moira inherits from the `SubComponent` class which is the base class of most Amiga components. One important purpose of the `SubComponent` class is to provide convinient access to all other components of the virtual Amiga. This is done by declaring a number of references that are initialized in the constructor: 

```c++
class SubComponent : public AmigaComponent {

protected:

    Agnus &agnus;
    Amiga &amiga;
    Blitter &blitter;
    ...
    Memory &mem;
    ...
};
``` 

Here we go. The `mem` object is inherited from the `SubComponent` class and allows any Moira function to access Amiga memory. 

Side note: If you spot a violation of the loose coupling principle here, please remember that we are talking about an emulator which a very special kind of software in various ways. The Amiga is a very tightly coupled system, and we wouldn't do ourselves any good if we tried to emulate it with a loosely coupled software system.

$$$

## Creating a run loop

Last but not least, let's take a closer look at where vAmiga executes the CPU, i.e. where it calls Moira's execution function. As you may have noticed, the Amiga class inherits from the Thread class, which provides capabilities for concurrent code execution. Whenever an instance of the Amiga class is created, an emulator thread is spawned automatically. 

```c++
Amiga::Amiga()
{
    ...
    // Start the thread and enter the main function
    thread = std::thread(&Thread::main, this);
    ...
}
```

Inside the function `main` one of the following functions is executed, depending on the operating mode of the thread: 
 
 - `execute<SyncMode::Periodic>();`
 - `execute<SyncMode::Pulsed>();`

Both functions call the `execute` function which is implemented inside the Amiga class. This function is one of the most prominent, since it implements the emulator's run loop. It is structured as follows:

```c++
void
Amiga::execute()
{    
    while (1) {
        
        // Emulate the next CPU instruction
        cpu.execute();

        // Check if special action needs to be taken
        if (flags) {
            ...
             if (flags & RL::BREAKPOINT_REACHED) {
                ...
                auto addr = isize(cpu.debugger.breakpoints.hit->addr);
                msgQueue.put(MSG_BREAKPOINT_REACHED, addr);
                newState = EXEC_PAUSED;
                break;
            }
            ...
        }
    }
}
```

We are finally there. Right at the beginning of the body of the while loop is the place where Moira's execution function is called. Variable `flags` is used to trigger special actions inside the run loop. E.g., when Moira reaches a breakpoint, a dedicated bit in variable `flags` is set. The bits are checked inside the run loop and  trigger special actions if it is set. In case of the breakpoint flag, vAmiga first performs some actions which are not important here. After that, it notifies the GUI by sending a `MSG_BREAKPOINT_REACHED` message. Finally, it puts the thread into paused state and exits the run loop. 
