//
//  musashi.cpp
//  Moira
//
//  Created by Dirk Hoffmann on 22.12.19.
//  Copyright © 2019 Dirk Hoffmann. All rights reserved.
//

#include <stdio.h>
#include <string.h>

#include "musashi.h"
#include "Moira.h"

extern "C" {
#include "m68k.h"
}

uint8_t mem[0x10000];

extern "C" unsigned int m68k_read_memory_8(unsigned int addr)
{
    return mem[addr & 0xFFFF];
}

extern "C" unsigned int m68k_read_memory_16(unsigned int addr)
{
    int hi = mem[addr & 0xFFFF];
    int lo = mem[(addr + 2) & 0xFFFF];
    return hi << 16 | lo;
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
    assert(false);
}

extern "C" void m68k_write_memory_8(unsigned int addr, unsigned int value)
{
    mem[addr & 0xFFFF] = value;
}

extern "C" void m68k_write_memory_16(unsigned int addr, unsigned int value)
{
    mem[addr & 0xFFFF] = (value >> 8) & 0xFF;
    mem[(addr + 2) & 0xFFFF] = value & 0xFF;
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
    memset(mem, 0, sizeof(mem));

    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68k_set_int_ack_callback(interrupt_handler);
}

void setMem16(uint32_t addr, uint16_t val)
{
    mem[addr] = val >> 8;
    mem[addr + 1] = val & 0xFF;
}

void dasmTest()
{
    Moira moira;
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

    printf("Peforming disassembler test... \n");
    setupMusashi();
    moira.reset();

    // Iterate through all opcodes
    for (uint32_t opcode = 0x0000; opcode < 65536; opcode++) {

        for (int i = 0; i < 48; i++) {
            for (int j = 0; j < 48; j++) {

                // Setup instruction
                const u32 pc = 0x1000;
                setMem16(pc + 0, opcode);
                setMem16(pc + 2, ext[i]);
                setMem16(pc + 4, ext[j]);

                // Ask Musashi to disassemble
                musashiCnt = m68k_disassemble(musashiStr, pc, M68K_CPU_TYPE_68000);

                // Ask Moira to disassemble
                moiraCnt = moira.disassemble(pc, moiraStr);

                printf("Checking %x %x %x\n", opcode, ext[i], ext[j]);

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
    printf("PASSED\n");
}
