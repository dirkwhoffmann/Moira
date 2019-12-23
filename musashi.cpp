//
//  musashi.cpp
//  Moira
//
//  Created by Dirk Hoffmann on 22.12.19.
//  Copyright © 2019 Dirk Hoffmann. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "musashi.h"
#include "Moira.h"
#include "globals.h"

extern "C" {
#include "m68k.h"
}

extern "C" unsigned int m68k_read_memory_8(unsigned int addr)
{
    int result = mem[addr & 0xFFFF];
    // moiracpu->sandbox.record(moira::PEEK8, addr, 0, result);
    return result;
}

extern "C" unsigned int m68k_read_memory_16(unsigned int addr)
{
    int hi = mem[addr & 0xFFFF];
    int lo = mem[(addr + 1) & 0xFFFF];
    int result = hi << 8 | lo;
    // moiracpu->sandbox.record(moira::PEEK16, addr, 0, result);
    return result;
}

extern "C" unsigned int m68k_read_memory_32(unsigned int addr)
{
    assert(false);
}

extern "C" unsigned int m68k_read_disassembler_16 (unsigned int addr)
{
    return m68k_read_memory_16(addr);
}

extern "C" unsigned int m68k_read_disassembler_32 (unsigned int addr)
{
    return m68k_read_memory_16(addr) << 16 | m68k_read_memory_16(addr + 2);
}

extern "C" void m68k_write_memory_8(unsigned int addr, unsigned int value)
{
    moiracpu->sandbox.record(moira::POKE8, addr, 0, value);
    mem[addr & 0xFFFF] = value;
}

extern "C" void m68k_write_memory_16(unsigned int addr, unsigned int value)
{
    mem[addr & 0xFFFF] = (value >> 8) & 0xFF;
    mem[(addr + 1) & 0xFFFF] = value & 0xFF;
    moiracpu->sandbox.record(moira::POKE16, addr, 0, value);
}

extern "C" void m68k_write_memory_32(unsigned int addr, unsigned int value)
{
     assert(false);
}

extern "C" int interrupt_handler(int irqLevel)
{
    printf("interrupt_handler\n");
    return M68K_INT_ACK_AUTOVECTOR;
}

extern "C" int read_sp_on_reset(void) { return 0x2000; }
extern "C" int read_pc_on_reset(void) { return 0x1000; }

void setupMusashi()
{
    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68k_set_int_ack_callback(interrupt_handler);
}

void setupMemory(uint32_t addr, uint16_t val1, uint16_t val2, uint16_t val3)
{
    memset(mem, 0, sizeof(mem));
    setMem16(addr, val1);
    setMem16(addr + 2, val2);
    setMem16(addr + 4, val3);
}

void setMem16(uint32_t addr, uint16_t val)
{
    mem[addr] = val >> 8;
    mem[addr + 1] = val & 0xFF;
}

void dasmTest()
{
    moira::Moira moiraCPU;
    char moiraStr[128], musashiStr[128];
    int moiraCnt, musashiCnt;
    
    // List of extension words used for testing
    uint16_t ext[] = {
        0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
        0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000,
        0x000F, 0x00F0, 0x0F00, 0xF000, 0x0007, 0x0070, 0x0700, 0x7000,
        0x00FF, 0xFF00, 0x0FF0, 0xF00F, 0xF0F0, 0x0F0F, 0xFFF0, 0x0FFF,
        0x8000, 0x8001, 0x8010, 0xF456, 0xAAAA, 0xF0F0, 0xF00F, 0xFFFF,
        0x0000, 0x0001, 0x0010, 0x7456, 0x2AAA, 0x70F0, 0x700F, 0x7FFF,
    };

    printf("Peforming disassembler tests... \n");
    setupMusashi();
    // moiraCPU.reset();

    clock_t start = clock();

    // Iterate through all opcodes
    for (uint32_t opcode = 0x0000; opcode < 65536; opcode++) {

        if ((opcode & 0xFFF) == 0) printf("Opcodes %xxxx\n", opcode >> 12);

        for (int i = 0; i < 48; i++) {
            for (int j = 0; j < 48; j++) {

                // Setup instruction
                const uint32_t pc = 0x1000;
                setMem16(pc + 0, opcode);
                setMem16(pc + 2, ext[i]);
                setMem16(pc + 4, ext[j]);

                // Ask Musashi to disassemble
                musashiCnt = m68k_disassemble(musashiStr, pc, M68K_CPU_TYPE_68000);

                // Ask Moira to disassemble
                moiraCnt = moiraCPU.disassemble(pc, moiraStr);

                // Compare
                if (strcmp(musashiStr, moiraStr) != 0 || musashiCnt != moiraCnt) {

                    printf("\n");
                    printf("Mismatch found: %x %x %x\n\n", opcode, ext[i], ext[j]);
                    printf("       Musashi: '%s'\n", musashiStr);
                    printf("         Moira: '%s'\n\n", moiraStr);
                    printf("         Sizes: %d Bytes / %d Bytes\n", musashiCnt, moiraCnt);
                    assert(false);
                }
            }
        }
    }

    clock_t now = clock();
    double elapsed = (double(now - start) / double(CLOCKS_PER_SEC));
    printf("PASSED (%f sec)\n", elapsed);
}

void execTest()
{
    char moiraStr[128], musashiStr[128];
    int64_t moiraCnt;
    int musashiCnt;
    const uint32_t pc = 0x1000;

    // List of extension words used for testing
    uint16_t ext[] = {
        0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
        0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000,
        0x000F, 0x00F0, 0x0F00, 0xF000, 0x0007, 0x0070, 0x0700, 0x7000,
        0x00FF, 0xFF00, 0x0FF0, 0xF00F, 0xF0F0, 0x0F0F, 0xFFF0, 0x0FFF,
        0x8000, 0x8001, 0x8010, 0xF456, 0xAAAA, 0xF0F0, 0xF00F, 0xFFFF,
        0x0000, 0x0001, 0x0010, 0x7456, 0x2AAA, 0x70F0, 0x700F, 0x7FFF,
    };

    printf("Peforming execution tests... \n");
    setupMusashi();

    clock_t start = clock();

    // Iterate through all opcodes
    for (uint32_t opcode = 0x0000; opcode < 65536; opcode++) {

        if ((opcode & 0xFFF) == 0) printf("Opcodes %xxxx\n", opcode >> 12);

        /*
        for (int i = 0; i < 48; i++) {
            for (int j = 0; j < 48; j++) {
        */
        for (int i = 32; i < 33; i++) {
            for (int j = 34; j < 35; j++) {

                // Setup memory for Musashi
                setupMemory(pc, opcode, ext[i], ext[j]);

                // Reset Musashi
                m68k_pulse_reset();
                printf("Musashi PC = %x\n", m68k_get_reg(NULL, M68K_REG_PC));

                // Disassemble instruction
                m68k_disassemble(musashiStr, pc, M68K_CPU_TYPE_68000);
                printf("Instruction: %s (Musashi)\n", musashiStr);
                
                // Run Musashi
                musashiCnt = m68k_execute(1);
                printf("Musashi PC = %x\n", m68k_get_reg(NULL, M68K_REG_PC));

                // Setup memory for Moira
                setupMemory(pc, opcode, ext[i], ext[j]);

                // Reset Moira
                moiracpu->reset();

                moiraCnt = moiracpu->getClock();
                printf("Moira PC = %x\n", moiracpu->getPC());

                // Disassemble instruction
                moiracpu->disassemble(pc, moiraStr);
                printf("Instruction: %s (Moira)\n", moiraStr);

                // Run Moira
                moiracpu->process();
                moiraCnt = moiracpu->getClock() - moiraCnt;
                printf("Moira PC = %x\n", moiracpu->getPC());

                printf("Cycles: %d / %lld\n", musashiCnt, moiraCnt);
                // if (musashiCnt != moiraCnt) assert(false); 
            }
        }
    }

    clock_t now = clock();
    double elapsed = (double(now - start) / double(CLOCKS_PER_SEC));
    printf("PASSED (%f sec)\n", elapsed);
}
