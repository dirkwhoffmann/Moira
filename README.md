![alt Logo](http://www.dirkwhoffmann.de/vAMIGA/pics/MoiraBanner1.png)

# About

Moira is a Motorola 68000 emulator written in C++. The emulator was written with four goals in mind: 

- A high emulation accuracy, both functionally and temporally
- A high emulation speed
- The ability to easily port the code to other platforms.
- The ability to emulate multiple CPU instances simultaneously

Moira is the CPU being used in the Amiga emulator vAmiga. It is used there with great success so far. 

## History

Before I started the vAmiga project in early 2019, I informed myself about the existing Motorola 68k emulators at the time. What I was looking for was a throughly tested emulator with high emulation speed and good code quality. I did find such an emulator in the form of Musashi, written by Karl Stenerud, which is being used successfully in the Mame project for many years now. I had been using Musashi in vAmiga for roughly a year until I realized that it lacks one important feature: Precise inter-instruction timing information.

In order to emulate the memory bus of the Amiga with high precision, I needed to know the number of cycles that had elapsed, for example, between the beginning of a command execution and the first or second access of the memory bus. Like most 68k emulators, Musashi only provides the total number of CPU cycles that has been consumed by the execution of a single instruction. Because cycle-exact bus timing is a crucial aspect for Amiga emulation, I came to the conclusion that more sophisticated timing control was needed to bring vAmiga to the next level.

My search for a new CPU core had drawn my attention to the Portable68000 emulator and its successor Denise m680x0, both written by "PiCiJi". These emulators offer inter-instruction timing and thus exactly what I needed to take vAmiga to the next level. However, Denise wasn't thoroughly tested back then, but  already far superior to portable68000 from a software-architectural point of view.  At this point I decided to implement my own CPU based on the three CPU cores I already knew. In fact Musashi, Portable68000 and Denise m680x0 can be considered the intellectual ancestors of my CPU implementation: Moira imitates much of the concepts the three other emulators are based on. Externally, Moira might appear a bit like a clone of Musashi, because it mimics Musashi’s disassembler output down to the last character. Internally, Moira exhibits  many similarities with Denise m680x0. For example, it provides exactly the same sync function that is being used by Portable68000 and Denise to count the number elapsed cycles between bus accesses. 
  
## Status

   
## Where to go from here?

- [Denise m680x0 on BitBucket](https://bitbucket.org/piciji/denise/src/master/emulation/processor)
- [Portable68000 on SourceForge](https://sourceforge.net/projects/portable68000)
- [Musashi on GitHub](https://github.com/kstenerud/Musashi)
- [vAmiga on GitHub](https://github.com/dirkwhoffmann/vAMIGA)
