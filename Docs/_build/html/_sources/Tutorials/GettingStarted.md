# Getting Started 

## Obtaining 

The recommended way to download Moira is by cloning the project directly from GitHub.

````bash
git clone https://github.com/dirkwhoffmann/Moira.git
cd Moira 
````

Older versions are available at [Moira's release page](https://github.com/dirkwhoffmann/Moira/releases).  

## Configuring 

All compile-time options are configured in a single file called `MoiraConfig.h`. For this tutorial, there is no need to change the default settings. However, you may want to take a look inside the file to get an overview of the available options.

````bash
cat Moira/MoiraConfig.h
````

All available options are explained in detail in a separate section.

## Compiling 

The easiest way to compile Moira is with `CMake`. Simply create a build directory, invoke `CMake` to set up the build chain, and then run `make`:

````bash
mkdir build
cd build
cmake ..
make
````

## Running 

If built as explained above, an application called `testRunner` is created. This application iterates over all opcodes, runs Moira and Musashi in parallel, and compares the results. If everything works as expected, it will terminate after a few minutes. If a discrepancy is detected, an error report is generated. In such cases, please file a bug on GitHub. Any help in identifying and resolving remaining issues is greatly appreciated.


````text
Moira CPU tester. (C) Dirk W. Hoffmann, 2019 - 2025

The test program runs Moira agains Musashi with randomly generated data.

    Test rounds : 1
    Random seed : 2084
     Exec range : (opcode >= 0x0000 && opcode <= 0xEFFF)
     Dasm range : (opcode >= 0x0000 && opcode <= 0xFFFF)

Round 1:

68000 CPU ................................ PASSED  (Moira: 0.50s  Musashi: 0.67s)
68010 CPU ................................ PASSED  (Moira: 1.00s  Musashi: 1.34s)
EC020 CPU ................................ PASSED  (Moira: 1.53s  Musashi: 2.03s)
68020 CPU ................................ PASSED  (Moira: 2.05s  Musashi: 2.73s)
EC030 CPU ................................ PASSED  (Moira: 2.59s  Musashi: 3.43s)
68030 CPU ................................ PASSED  (Moira: 3.17s  Musashi: 4.17s)
68030 MMU ................................ PASSED  (Moira: 3.17s  Musashi: 4.17s)
EC040 CPU ................................ PASSED  (Moira: 3.69s  Musashi: 4.86s)
LC040 CPU ................................ PASSED  (Moira: 4.21s  Musashi: 5.54s)
LC040 MMU ................................ PASSED  (Moira: 4.21s  Musashi: 5.54s)
68040 CPU ................................ PASSED  (Moira: 4.75s  Musashi: 6.25s)
68040 MMU ................................ PASSED  (Moira: 4.75s  Musashi: 6.25s)
68040 FPU ................................ PASSED  (Moira: 4.75s  Musashi: 6.25s)

All tests completed
````

Please note that not all test rounds execute instructions. This only occurs in the first four rounds when emulating a 68000, 68010, 68EC020, or 68020. For all other CPU models, only the disassembler is invoked.
