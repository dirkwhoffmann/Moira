# Installation

This page explains how to download and build Moira and how to run the tests.

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

This builds the CPU core as a static library, the example programs and the test tools. [Running the Examples](/overview/getting-started/Examples) shows how to run the examples.

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
