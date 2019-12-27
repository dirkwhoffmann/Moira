// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "testrunner.h"

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

    return result;
}

extern "C" unsigned int m68k_read_memory_32(unsigned int addr)
{
    int hi = m68k_read_memory_16(addr);
    int lo = m68k_read_memory_16(addr + 2);
    int result = hi << 16 | lo;
    
    return result;
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
    m68k_write_memory_16(addr, (value >> 16) & 0xFFFF);
    m68k_write_memory_16(addr + 2, value & 0xFFFF);
}

extern "C" int interrupt_handler(int irqLevel)
{
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

void resetMusashi()
{
    m68k_set_reg(M68K_REG_D0, 0x10);
    m68k_set_reg(M68K_REG_D1, 0x20);
    m68k_set_reg(M68K_REG_D2, 0x30);
    m68k_set_reg(M68K_REG_D3, 0x40);
    m68k_set_reg(M68K_REG_D4, 0x50);
    m68k_set_reg(M68K_REG_D5, 0x60);
    m68k_set_reg(M68K_REG_D6, 0x70);
    m68k_set_reg(M68K_REG_D7, 0x80);
    m68k_set_reg(M68K_REG_A0, 0x90);
    m68k_set_reg(M68K_REG_A1, 0x10000);
    m68k_set_reg(M68K_REG_A2, 0x11011);
    m68k_set_reg(M68K_REG_A3, 0x12033);
    m68k_set_reg(M68K_REG_A4, 0x13000);
    m68k_set_reg(M68K_REG_A5, 0x14000000);
    m68k_set_reg(M68K_REG_A6, 0x80000000);
    m68k_set_reg(M68K_REG_USP, 0);
    m68k_set_reg(M68K_REG_ISP, 0);
    m68k_set_reg(M68K_REG_MSP, 0);
    m68k_set_reg(M68K_REG_SR, 0);

    m68k_pulse_reset();
}

void setupMemory(uint32_t addr, uint16_t val1, uint16_t val2, uint16_t val3)
{
    for (unsigned i = 0; i < sizeof(mem); i++) {
        mem[i] = (uint8_t)((7 * i) & ~1);
    }
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
                bool error = false;
                error |= (strcmp(musashiStr, moiraStr) != 0);
                error |= (musashiCnt != moiraCnt);

                if (error) {
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
    char moiraStr[128];
    int64_t musashiCycles, moiraCycles;
    int32_t musashiPC, moiraPC;
    uint16_t musashiSR, moiraSR;
    uint32_t musashiD[8], moiraD[8];
    uint32_t musashiA[8], moiraA[8];

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

        // Skip illegal instructions
        if (moiracpu->isIllegalInstr(opcode)) continue;
        if (moiracpu->isLineAInstr(opcode)) continue;
        if (moiracpu->isLineFInstr(opcode)) continue;

        /*
        for (int i = 0; i < 48; i++) {
            for (int j = 0; j < 48; j++) {
        */
        for (int i = 32; i < 33; i++) {
            for (int j = 34; j < 35; j++) {

                moiracpu->sandbox.prepare();
                
                // Setup memory for Musashi
                setupMemory(pc, opcode, ext[i], ext[j]);

                // Reset Musashi
                resetMusashi();

                // Disassemble instruction
                // m68k_disassemble(musashiStr, pc, M68K_CPU_TYPE_68000);
                // printf("Instruction: %s (Musashi)\n", musashiStr);
                
                // Run Musashi
                musashiCycles = m68k_execute(1);
                musashiPC = m68k_get_reg(NULL, M68K_REG_PC);
                musashiSR = m68k_get_reg(NULL, M68K_REG_SR);
                musashiD[0] = m68k_get_reg(NULL, M68K_REG_D0);
                musashiD[1] = m68k_get_reg(NULL, M68K_REG_D1);
                musashiD[2] = m68k_get_reg(NULL, M68K_REG_D2);
                musashiD[3] = m68k_get_reg(NULL, M68K_REG_D3);
                musashiD[4] = m68k_get_reg(NULL, M68K_REG_D4);
                musashiD[5] = m68k_get_reg(NULL, M68K_REG_D5);
                musashiD[6] = m68k_get_reg(NULL, M68K_REG_D6);
                musashiD[7] = m68k_get_reg(NULL, M68K_REG_D7);
                musashiA[0] = m68k_get_reg(NULL, M68K_REG_A0);
                musashiA[1] = m68k_get_reg(NULL, M68K_REG_A1);
                musashiA[2] = m68k_get_reg(NULL, M68K_REG_A2);
                musashiA[3] = m68k_get_reg(NULL, M68K_REG_A3);
                musashiA[4] = m68k_get_reg(NULL, M68K_REG_A4);
                musashiA[5] = m68k_get_reg(NULL, M68K_REG_A5);
                musashiA[6] = m68k_get_reg(NULL, M68K_REG_A6);
                musashiA[7] = m68k_get_reg(NULL, M68K_REG_A7);

                // Setup memory for Moira
                setupMemory(pc, opcode, ext[i], ext[j]);

                // Skip NBCD, SBCD, ABCD for now
                if (isBcd(opcode)) {
                    // moiracpu->disassemble(pc, moiraStr);
                    // printf("SKIPPING $%4x: %s\n", opcode, moiraStr);
                    continue;
                }

                // Reset Moira
                moiracpu->reset();
                moiraCycles = moiracpu->getClock();

                // Disassemble instruction
                moiracpu->disassemble(pc, moiraStr);
                // printf("Instruction $%4x: %s\n", opcode, moiraStr);

                // Run Moira
                moiracpu->process();
                moiraCycles = moiracpu->getClock() - moiraCycles;
                moiraPC = moiracpu->getPC();
                moiraSR = moiracpu->getSR();
                for (int i = 0; i < 8; i++) moiraD[i] = moiracpu->readD(i);
                for (int i = 0; i < 8; i++) moiraA[i] = moiracpu->readA(i);

                // Compare results
                bool error = false;
                bool regError = false;

                // printf("Cycles %d / %d\n", musashiCycles, moiraCycles);

                error |= (musashiPC != moiraPC);
                error |= (musashiSR != moiraSR);
                error |= !isMulOrDiv(opcode) && (musashiCycles != moiraCycles);
                for (int i = 0; i < 8; i++) regError |= musashiD[i] != moiraD[i];
                for (int i = 0; i < 8; i++) regError |= musashiA[i] != moiraA[i];

                if (error || regError) {
                    moiracpu->disassemble(pc, moiraStr);
                    printf("\nMISMATCH FOUND (opcode $%x out of $FFFF):\n\n", opcode);
                    printf("Instruction: %s\n\n", moiraStr);
                    printf("    Musashi: ");
                    printf("PC: %4x ", musashiPC);
                    printf("SR: %2x ", musashiSR);
                    printf("Elapsed cycles: %2lld\n" , musashiCycles);
                    printf("      Moira: ");
                    printf("PC: %4x ", moiraPC);
                    printf("SR: %4x ", moiraSR);
                    printf("Elapsed cycles: %2lld\n\n" , moiraCycles);
                    printf("\n");

                    if (regError) {
                        printf("Musashi: Dx: %8x %8x %8x %8x %8x %8x %8x %8x\n",
                               musashiD[0], musashiD[1], musashiD[2], musashiD[3],
                               musashiD[4], musashiD[5], musashiD[6], musashiD[7]);
                        printf("         Ax: %8x %8x %8x %8x %8x %8x %8x %8x\n\n",
                               musashiA[0], musashiA[1], musashiA[2], musashiA[3],
                               musashiA[4], musashiA[5], musashiA[6], musashiA[7]);
                        printf("  Moira: Dx: %8x %8x %8x %8x %8x %8x %8x %8x\n",
                               moiraD[0], moiraD[1], moiraD[2], moiraD[3],
                               moiraD[4], moiraD[5], moiraD[6], moiraD[7]);
                        printf("  Moira: Ax: %8x %8x %8x %8x %8x %8x %8x %8x\n",
                               moiraA[0], moiraA[1], moiraA[2], moiraA[3],
                               moiraA[4], moiraA[5], moiraA[6], moiraA[7]);
                    }
                    assert(false);
                }
            }
        }
    }

    clock_t now = clock();
    double elapsed = (double(now - start) / double(CLOCKS_PER_SEC));
    printf("PASSED (%f sec)\n", elapsed);
}
