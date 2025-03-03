# About

Moira is a Motorola 68000, 68010, 680EC20, and 68020 emulator compatible with macOS, Linux, and Windows. The core emulator's source code is written in C++ and published under the MIT license. Moira serves as the heart of [vAmiga](https://dirkwhoffmann.github.io/vAmiga), a high-precision emulator for classic Commodore Amiga models.

![Screenshot](../images/disassembler.png "Moira screenshot")

## Design Goals 

Moira was developed with three primary goals:

- **Accuracy**

  Moira delivers high emulation accuracy, both functionally and temporally. Designed specifically for Amiga emulation, it ensures not only precise cycle counts but also that memory accesses occur at the correct DMA cycles.
  
- **Efficiency**

  Moira is optimized for high emulation speed. To achieve this goal, it is written entirely in C++ and makes extensive use of templates. It surpasses the performance of Musashi, one of the fastest M68k emulators ever developed.
  
- **Portability**

  The source code is compatible with all major C++ compilers, including Clang, GCC, and MSVC. It compiles seamlessly on macOS, Linux, and Windows. Additionally, Moira supports WebAssembly, making it well-suited for web-based applications.
  
## History

Before starting the vAmiga project in early 2019, I researched existing Motorola 68k emulators. My goal was to find a well-tested emulator with high performance and clean code. I discovered Musashi, written by Karl Stenerud, which had been successfully used in the MAME project for many years.

I integrated Musashi into vAmiga and used it for about a year before realizing it lacked a crucial feature: precise inter-instruction timing. To emulate the Amiga’s memory bus with high accuracy, I needed detailed timing information — specifically, the exact number of cycles elapsed between key events such as instruction execution and memory access. Like most 68k emulators, Musashi only provides the total execution time per instruction, which was insufficient for my needs. Since cycle-exact bus timing is critical for Amiga emulation, I concluded that more sophisticated timing control was necessary to elevate vAmiga to the next level.

My search for a new CPU core led me to Portable68000 and its successor, Denise m680x0, both written by "PiCiJi." These emulators provided inter-instruction timing, precisely what I needed to advance the vAmiga project. However, at the time, Denise was still in its early stages. While architecturally superior to Portable68000, it had not yet undergone extensive testing.

At this point, I decided to implement my own CPU emulator, drawing inspiration from the three cores I had studied. In many ways, Musashi, Portable68000, and Denise m680x0 can be considered the intellectual ancestors of Moira. Externally, early versions of Moira closely resembled Musashi, even replicating its disassembler output down to the last character. Internally, however, Moira shares many design principles with Denise m680x0. For example, it implements the same synchronization mechanism used in Portable68000 and Denise to track cycle timing between memory accesses.

## Credits

*Nanos gigantum humeris insidentes.* I am grateful to
- Karl Stenerud for writing [Musahi](https://github.com/kstenerud/Musashi), 
- PiCiJi for writing Denise [m680x0](https://bitbucket.org/piciji/denise/src/master/emulation/processor) and [portable68000](https://sourceforge.net/projects/portable68000), and
- Toni Wilen for writing [cputester](https://eab.abime.net/showthread.php?t=98747).
