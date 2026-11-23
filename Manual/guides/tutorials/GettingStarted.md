# Getting Started

This tutorial shows how to download and build Moira, run the example programs and run the tests.

## Requirements

Moira is written in C++20. It compiles with Clang, GCC and MSVC on macOS, Linux and Windows. The build is set up with CMake 3.16 or later. On macOS, the Xcode project `Moira.xcodeproj` can be used instead.

## Obtaining

The recommended way to download Moira is to clone the project from GitHub:

```bash
git clone https://github.com/dirkwhoffmann/Moira.git
cd Moira
```

Older versions are available on [Moira's release page](https://github.com/dirkwhoffmann/Moira/releases).

The repository is organized as follows:

| Folder | Content |
|:-------|:--------|
| `Moira/` | The CPU core. This is the only folder you need to use Moira in your own project. |
| `Examples/` | Small programs that show how to use Moira. |
| `Tests/` | The tests and the test tools. |
| `Manual/` | The sources of this manual. |

## Building

Create a build directory, let CMake set up the build chain, and compile:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

This builds the CPU core as a static library, the example programs and the test tools.

## Running the Examples

### Fibonacci

The first example runs a small 68000 program that computes the first ten Fibonacci numbers:

```bash
./build/Examples/Fibonacci/fibonacci
```

The program disassembles the test program, executes it instruction by instruction and prints the CPU state and the result table after each step:

```text
Test program:

000400: 41f8 2000        lea     $2000.w, A0
000404: 7001             moveq   #$1, D0
000406: 1080             move.b  D0, (A0)
...

Instruction trace:

    PC   SR XNZVC  D0       D1       A0       SP        Instruction                 $2000...
000400 2700 -----  00000000 00000000 00000000 00008000  lea     $2000.w, A0         0   0   0   0   0   0   0   0   0   0
000404 2700 -----  00000000 00000000 00002000 00008000  moveq   #$1, D0             0   0   0   0   0   0   0   0   0   0
...
000420 2704 --Z--  00000037 0000000A 00002000 00008000  bne     $40e                1   1   2   3   5   8  13  21  34  55

SUCCESS
```

The source code (`Examples/Fibonacci/main.cpp`) is short and well commented. It is a good starting point for your own experiments, because it covers the three things every application has to do: derive a class from `Moira` and connect it to memory, reset the CPU and execute instructions, and inspect the CPU state.

### Easy68K

The second example runs programs written for [Easy68K](http://www.easy68k.com), a 68000 development environment that is popular for teaching. Easy68K programs communicate with their environment through `TRAP #15`, which the example implements on the console. Without arguments, it runs a built-in Hello World program:

```bash
./build/Examples/Easy68k/easy68k
```

An assembled program (Motorola S-record format) is passed with `--file`:

```bash
./build/Examples/Easy68k/easy68k --file Examples/Easy68k/hello.s68
```

The option `--trace` prints every executed instruction, and `--model` selects the CPU (68000, 68010 or 68020). Run `easy68k --help` for all options.

## Running the Tests

The tests are run with CTest. The fast tests take a few seconds:

```bash
ctest --test-dir build -L "unit|smoke|example"
```

The complete suite compares Moira with several reference emulators (MAME, UAE and Musashi) and with reference disassemblers. Without the disassembler tests, it takes about ten minutes. The disassembler tests take about half an hour on their own:

```bash
ctest --test-dir build -j6 -E Dasm
```

The test tools can also be run directly. `Tests/README.md` describes each tool and its options.

## Configuring

All compile-time options are defined in `Moira/MoiraConfig.h`. Each option has a default value and is documented in the file. For this tutorial, there is no need to change anything. Since every option is guarded by `#ifndef`, it can also be set from the build system without editing the file, e.g. with CMake:

```cmake
target_compile_definitions(moira PUBLIC MOIRA_EMULATE_BUS_ERROR=true)
```

All options are described in [Configuration Options](/api/ConfigOptions).

## Next Steps

[Using Moira in Your Own App](/guides/tutorials/UsingMoiraInYourOwnApp) explains how to integrate the CPU core into an application of your own.
