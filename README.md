![alt Logo](http://www.dirkwhoffmann.de/vAMIGA/pics/MoiraBanner1.png)

# Moira: A Motorola 68000 emulator

Moira is a Motorola 68000 emulator written in C++. The emulator was written with four goals in mind: 

- A high emulation accuracy, both functionally and temporally
- A high emulation speed
- The ability to easily port the code to other platforms.
- The ability to emulate multiple CPU instances simultaneously

Moira is the CPU being used in the Amiga emulator vAmiga. It is used there with great success so far. 

## Anchestors

Because many good CPU cores are already out there, Moira is not developed from scratch. It will imitate much of what other authors do in their own emulators with great success. The following CPU cores can be considered the intellectual ancestors of Moira: 

- Denise m680x0

  https://bitbucket.org/piciji/denise/src/master/emulation/processor/


  This CPU core is part of the Denise Retro emulator written by PiCiJi. Due to its remarkable software architecture, I decided to use it as a blueprint for Moira. I have taken many design decisions from this core. It also serves as a temporal reference implementation for Moira, since unlike most other cores it emulates bus timing with high precision.

  
- Portable68000

  https://sourceforge.net/projects/portable68000/

  This core is the predecessor of Denise m680x0, also authored by PiCiJi. It contains an extensive set of unit tests which I am going to use in my project.
  
- Musashi

  https://github.com/kstenerud/Musashi

  Musashi is a CPU core written in C by Karl Stenerud. It is well known for its high speed and remarkable software quality. It is being used in many emulators, including the current version of vAmiga. Due to its wide distribution, this core has achieved a high degree of functional accuracy and is therefore beeing uses as a functional reference implementation for Moira. 
  
 ## Goal
 
With Moira I want to implement a Motorola 68000 CPU core that is functionally equivalent to Musashi and temporally equivalent to Denise 680x0. 
 
## Status

Development has started in November 2019. There is no working prototype yet. 
   
## Where to go from here?

- [vAmiga](https://github.com/dirkwhoffmann/vAMIGA)
