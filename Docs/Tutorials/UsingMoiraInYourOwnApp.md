# Using Moira in Your Own App

This tutorial guides you through integrating Moira into your application. It covers the essential steps using vAmiga, the Amiga emulator Moira was originally developed for.

## Configuring Moira

We begin by examining the compile-time options configured in vAmiga. The following options are selected in `MoiraConfig.h`:

- `#define MOIRA_PRECISE_TIMING true`

With this option enabled, Moira operates in precise timing mode, meaning a `sync` call is triggered before each memory access. This allows vAmiga to emulate the surrounding logic up to the exact moment the memory access occurs. This is an essential feature for Amiga emulation, as many applications depend on accurate bus timing.
In fact, the absence of this feature in Musashi was the primary reason for developing Moira. However, note that precise timing mode is only available for the M68000 and M68010. M68020 emulation always runs in simple cycle mode, regardless of this preprocessor setting.

- `#define MOIRA_VIRTUAL_API false`

In vAmiga, all Moira class functions that must be implemented by the client are declared as non-virtual. This ensures they are statically linked, providing a slight performance advantage over virtual functions.

- `#define MOIRA_EMULATE_ADDRESS_ERROR true`

The M68000 and M68010 generate an address error when a word or longword access is attempted at an odd address. Since some Amiga titles depend on this behavior, vAmiga enables address error emulation to ensure accurate compatibility.

- `#define MOIRA_EMULATE_FC true`

All M680x0 CPUs have three function code (FC) pins that indicate the type of memory access being performed. While the Amiga itself does not use these pins, vAmiga enables their emulation to accurately replicate M68010 behavior. Unlike the M68000, the M68010 includes the FC pin values in certain stack frames, making this feature essential for precise emulation.

- `#define MOIRA_ENABLE_DASM true`

Enabling this option instructs Moira to generate a lookup table for all disassembler handlers. This table is required for using the disassembler. However, if disassembly is not needed, it is recommended to set this option to `false`, as the table consumes a significant amount of memory.

- `#define MOIRA_BUILD_INSTR_INFO_TABLE false`

During initialization, Moira can generate an optional info table containing details about the instruction, addressing mode, and size attribute for each of the 65536 opcode words. In vAmiga, this table is omitted since the information is not required, reducing memory usage.

- `#define MOIRA_MIMIC_MUSASHI false`

This option disables Musashi compatibility mode, allowing Moira to prioritize higher accuracy over Musashi-like behavior.

## Subclassing Moira

Having explored Moira's compile-time options, let's dive deeper into its integration within vAmiga. The core class in vAmiga is the `Amiga` class, which has the following structure:

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

The `CPU` class in vAmiga is a subclass of the Moira class, designed to extend Moira's functionality. For example, it provides features for serializing and deserializing the CPU state, which is crucial for saving and loading emulator snapshots. Additionally, the CPU class adds overclocking support to Moira. If you're interested in how overclocking is implemented in vAmiga, please refer to the How-to section.

In `CPU.cpp`, you'll find the implementations of all Moira functions that interact with the environment. Since the API is declared as non-virtual, none of these functions have a default implementation. As a result, each function must be explicitly implemented, even if there's no specific action required. This leads to many functions being defined with empty bodies, such as:

```c++
void
Moira::cpuDidReset()
{

}
```

This function is one of the delegation methods provided by Moira. It is called at the end of the reset handler, allowing the client to perform any special actions required.

Other functions, however, have non-empty implementations, such as those for memory access. For example, here’s how `read8` and `read16` are implemented in vAmiga:

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

At first glance, the code seems self-explanatory. Moira’s memory request is passed to the `mem` object, which is an instance of vAmiga’s Memory class. However, upon closer inspection, a question arises: How does Moira know about the various Amiga components, such as memory? These objects aren’t part of the Moira class itself. The answer is that vAmiga utilizes a slightly modified version of the Moira class. In fact, the class is declared as follows:

```c++
class Moira : public SubComponent {
    ...
}
```

As you can see, Moira inherits from the `SubComponent` class, which serves as the base class for most Amiga components. One key role of the SubComponent class is to provide convenient access to the other components of the virtual Amiga. This is achieved by declaring several references that are initialized in the constructor:

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

As you can see, the `mem` object is inherited from the `SubComponent` class, enabling any Moira function to access Amiga memory.
Side note: If you notice a potential violation of the loose coupling principle here, keep in mind that we are dealing with an emulator, a unique type of software. The Amiga itself is a tightly coupled system, and attempting to emulate it using a loosely coupled software architecture wouldn’t serve us well.

## Creating a Run Loop

Lastly, let's take a closer look at where vAmiga calls Moira's execution function. This happens inside function `computeFrame`, which, as the name already suggests, emulates the Amiga for a single video frame. 

This function is one of the most prominent, since it contains the emulator's run loop. It is structured as follows:

```c++
void
Amiga::computeFrame()
{
    while (1) {

        // Emulate the next CPU instruction
        cpu.execute();

        // Check if special action needs to be taken
        if (flags) {

            ...
            // Did we reach a breakpoint?
            if (flags & RL::BREAKPOINT_REACHED) {

                auto addr = cpu.debugger.breakpoints.hit->addr;
                msgQueue.put(Msg::BREAKPOINT_REACHED, CpuMsg { addr, 0 });
                action = pause;
            }
            ...
        }
    }
}
```

We’ve reached the critical code section. At the beginning of the while loop’s body, Moira’s execution function is called. The flags variable is used to trigger special actions within the run loop. For example, when Moira reaches a breakpoint, a specific bit in the flags variable is set. These bits are checked inside the run loop, and if a bit is set, it triggers the corresponding action.
In the case of the breakpoint flag, vAmiga first performs some necessary actions (which are not relevant here). Then, it notifies the GUI by sending a `BREAKPOINT_REACHED` message. Finally, it requests the subsequent code to pause the thread and exit the run loop.
