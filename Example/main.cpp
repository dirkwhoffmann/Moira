// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* A minimal example that shows how to use Moira.
 *
 * The program runs a tiny 68000 program that computes the first ten Fibonacci
 * numbers and verifies the result. It demonstrates the three things every
 * Moira user has to do:
 *
 *  1. Derive a class from Moira and implement the memory interface
 *  2. Reset the CPU and execute instructions
 *  3. Inspect the CPU state and disassemble instructions
 */

#include "Moira.h"
#include <cstdio>
#include <cstring>

using namespace moira;

// The computer's memory (64 KB)
static u8 ram[65536] = { };

// The test program (will be loaded at address $0400)
static const u16 prog[] =
{
    0x41F8, 0x2000,         //       LEA     $2000.w,A0
    0x7001,                 //       MOVEQ   #1,D0
    0x1080,                 //       MOVE.B  D0,(A0)
    0x1140, 0x0001,         //       MOVE.B  D0,1(A0)
    0x7202,                 //       MOVEQ   #2,D1
    0x1030, 0x10FE,         // loop: MOVE.B  -2(A0,D1.W),D0
    0xD030, 0x10FF,         //       ADD.B   -1(A0,D1.W),D0
    0x1180, 0x1000,         //       MOVE.B  D0,0(A0,D1.W)
    0x5241,                 //       ADDQ.W  #1,D1
    0x0C41, 0x000A,         //       CMPI.W  #10,D1
    0x66EC,                 //       BNE.B   loop
    0x4E72, 0x2700          //       STOP    #$2700
};

static const u32 progAddr = 0x0400;     // Start address of the program
static const u32 stackAddr = 0x8000;    // Initial stack pointer
static const u32 tableAddr = 0x2000;    // Location of the result table
static const u16 STOP = 0x4E72;         // Opcode of the STOP instruction

/* Step 1: Derive from the Moira class
 *
 * Moira doesn't own any memory. Whenever it needs to access the bus, it calls
 * one of the pure virtual functions below. Here, we map all requests to a plain
 * array. The 68000 is a big endian CPU, so the high byte of a word comes first.
 */
class CPU : public Moira {

public:

    u8 read8(u32 addr) const override { return ram[addr & 0xFFFF]; }
    u16 read16(u32 addr) const override { return u16(read8(addr) << 8 | read8(addr + 1)); }
    u32 read32(u32 addr) const override { return u32(read16(addr) << 16 | read16(addr + 2)); }

    void write8(u32 addr, u8 val) const override { ram[addr & 0xFFFF] = val; }
    void write16(u32 addr, u16 val) const override { write8(addr, u8(val >> 8)); write8(addr + 1, u8(val)); }
    void write32(u32 addr, u32 val) const override { write16(addr, u16(val >> 16)); write16(addr + 2, u16(val)); }

    CPU() { setModel(CPUModel::M68000); }

    // Prints the CPU state followed by the instruction that is about to be executed
    void dump()
    {
        char instr[64];
        (void)disassemble(instr, getPC0());

        // The five condition code flags in the order X N Z V C
        u16 sr = getSR();
        char flags[6];
        for (int i = 0; i < 5; i++) flags[i] = (sr >> (4 - i)) & 1 ? "XNZVC"[i] : '-';
        flags[5] = 0;

        printf("%06X %04X %s  %08X %08X %08X %08X  %-20s\n",
               getPC0(), sr, flags, getD(0), getD(1), getA(0), getSP(), instr);
    }
};

int main(int argc, const char *argv[])
{
    CPU cpu;

    printf("Moira - A Motorola 68k CPU emulator\n\n");
    printf("Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de\n");
    printf("Published under the terms of the MIT License\n\n");

    // Copy the test program into memory
    for (size_t i = 0; i < sizeof(prog) / sizeof(prog[0]); i++) {
        ram[progAddr + 2 * i] = u8(prog[i] >> 8);
        ram[progAddr + 2 * i + 1] = u8(prog[i]);
    }

    // Initialize the reset vectors (initial stack pointer and initial PC)
    for (int i = 0; i < 4; i++) {
        ram[i] = u8(stackAddr >> (24 - 8 * i));
        ram[4 + i] = u8(progAddr >> (24 - 8 * i));
    }

    // Step 2: Reset the CPU. This loads the initial SP and PC from the vector table.
    cpu.reset();

    // Disassemble the program
    printf("Test program:\n\n");

    char instr[64], bytes[64];
    for (u32 addr = progAddr; addr < progAddr + sizeof(prog); ) {

        int len = cpu.disassemble(instr, addr);
        cpu.dump16(bytes, addr, len / 2);

        printf("%06X: %-16s %s\n", addr, bytes, instr);
        addr += len;
    }

    printf("\nInstruction trace:\n\n");
    printf("    PC   SR XNZVC  D0       D1       A0       SP\n");

    // Execute instructions until the CPU reaches the STOP instruction
    while (cpu.read16(cpu.getPC0()) != STOP) {

        cpu.dump();
        cpu.execute();
    }

    // Step 3: Verify the result
    const u8 expected[] = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55 };
    for (size_t i = 0; i < sizeof(expected); i++) {

        u32 addr = tableAddr + u32(i);

        if (ram[addr] != expected[i]) {

            printf("\nERROR: ram[%04x] = %02x. Expected: %02x\n", addr, ram[addr], expected[i]);
            return 1;
        }
    }

    printf("\nSUCCESS\n");
    return 0;
}
