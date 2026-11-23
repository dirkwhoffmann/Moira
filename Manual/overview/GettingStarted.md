# Getting Started

This page runs the two example programs that come with Moira. It assumes that you have built Moira as described in [Installation](/overview/Installation).

## Fibonacci

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

## Easy68K

The second example runs programs written for [Easy68K](http://www.easy68k.com), a 68000 development environment that is popular for teaching. Easy68K programs communicate with their environment through `TRAP #15`, which the example implements on the console. Without arguments, it runs a built-in Hello World program:

```bash
./build/Examples/Easy68k/easy68k
```

An assembled program (Motorola S-record format) is passed with `--file`:

```bash
./build/Examples/Easy68k/easy68k --file Examples/Easy68k/hello.s68
```

The option `--trace` prints every executed instruction, and `--model` selects the CPU model (e.g. `--model 68030`). Run `easy68k --help` for all options.

## Next Steps

[Integrating the CPU Core](/guides/howto/HowToUseTheCpu) explains how to integrate the CPU core into an application of your own.
