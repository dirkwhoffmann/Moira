# Using Moira in your own app

This tutorial shows how Moira can be integrated in your own application. It explains the basic steps by showing how Moira is used inside vAmiga, the Amiga emulator Moira was originally developed for.

## Compile-time options 

We start by examining the compile-time option chosen by vAmiga. The following options are set in `MoiraConfig.h`:

- `#define PRECISE_TIMING true`

Moira is run in precise timing mode which means that a `sync` call is issued before each memory access. This gives vAmiga the opportunity to emulate the surrounding logic up to the point where the memory access happens. This is an essential feature in Amiga emulation, because a lot application rely on exact bus timing. In fact, the lack of this feature in Musashi was the reason why I implemented Moira in the first place. Note that this feature is only available for the M68000 and the M68010. M68020 emulation is always performed in simple cycle mode, no matter how this preprocessor variable is set. 

- `#define VIRTUAL_API false`

All functions of the Moira class that need to be implemented by the client (vAmiga) are declared as non-virtual. This means that these functions are linked statically which results in a small performance improvement compared to the tradition OOP approach which is based on virtual functions. 

- `#define EMULATE_ADDRESS_ERROR true`

The M68000 and M68010 signal an address error if a word or long word access is attempted at an odd address. Because a few Amiga title rely on this behaviour, address error emulation is turned on in vAmiga. 

- `#define EMULATE_FC true`

All M680x0 CPUs are equipped with three so called function code pins (FC pins). Whenever memory is accessed, the value on these pins allow external hardware to inspect the access type. Although the Amiga does not make use of the FC pins, emulation in Moira is enabled in order to emulate the M68010 CPU accurately. Unlike the M68000, the M68010 incoorprates the current value of the FC pins in some stack frames. 

- `#define ENABLE_DASM true`

This options tells Moira to create a lookup table with all disassemblers handlers. Building this table is mandatory for using the disassembler. If the disassembler is not needed, it is advices to set this option to `false` as the lookup table consumes a decent amount of memory. 

- `#define BUILD_INSTR_INFO_TABLE false`

During initialization, Moira is able to build an optional info table storing information about the instruction, the addressing mode, and the size attribute for each of the 65536 opcode words. Building this table is omitted in vAmiga because the information is not needed. 

- `#define MIMIC_MUSASHI false`

This option tells Moira to *not* run in Musashi compatibility mode which results in higher accuracy. 

## The CPU class 

After learning how Moira's compile-time options are configured in vAmiga, we turn our attention to the integration in vAmiga. The most prominent class in vAmiga is the `Amiga` class. It has the following structure: 

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

The `CPU` class serves as a wrapper around the Moira CPU. None of the other components interacts with the Moira CPU directly. Everything is routed through this class. But why do we need to wrap the Moira CPU? This is done to extend the Moira CPU with additional functionality. E.g., the CPU class provides capabilities for serializing and deserializing the CPU state which is important for saving and loading emulator states (snapshots). Futhermore, the CPU class enhances the Moira class with overclocking support. If you are interested in this topic, please refer to the How-to section. 

In `CPU.cpp` we find the implementations of all Moira function that interact with the enviroment. Because we've declared the API as non-virtual, none of these functions have a default implementation. Thus, all functions need to be implemented even if nothing has to be done. Hence, many functions are defined with an empty function body. E.g.:

```c++
void
Moira::didReset()
{

}
```

Other functions do have a custom implementation such as functions for accessing memory. E.g., this is how `read8` and `read16` are implemented in vAmiga:

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

At first glance, the code talks for itself. Moira's memory request is routed to the `mem` object which is an instrance of vAmiga's `Memory` class. Looking closer, it's no longer that easy. How can Moira know about the various Amiga components such as memory? Those objects are not part of the Moira class. Well, the answer is, they are, because in vAmiga, we use a slighlty modified version of the Moira class. In vAmiga, the class is declared as follows: 

```c++
class Moira : public SubComponent {
    ...
}
```

As you can see, Moira inherits from the `SubComponent` class which is the base class of most Amiga components. The purpose of `SubComponent` is to provide convinient access to all other components of the system. This is done by declaring references to the mostly used components: 

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

Here we go. The `mem` object is inherited from the `SubComponent` class and enables every Moira function to access the Amiga memory. 

Sidenote: In case you spot a violation of the loose-couple principle here, please be reminded that we're talking about an emulator. We are mimicing a hardware system that belongs to the tightliest coupled system we can think of. 
