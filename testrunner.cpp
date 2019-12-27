// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "testrunner.h"


bool isMul(uint16_t opcode)
{
    bool result = false;

    if ((opcode & 0b1111000111000000) == 0b1100000111000000) result = true;
    if ((opcode & 0b1111000111000000) == 0b1100000011000000) result = true;

    return result;
}

bool isDiv(uint16_t opcode)
{
    if ((opcode & 0b1111000111000000) == 0b1000000111000000) return true;
    if ((opcode & 0b1111000111000000) == 0b1000000011000000) return true;
    return false;
}

bool isMulOrDiv(uint16_t opcode)
{
    return isDiv(opcode) || isMul(opcode);
}

bool isAbcd(uint16_t opcode)
{
    if ((opcode & 0b1111000111111000) == 0b1000000100000000) return true;
    if ((opcode & 0b1111000111111000) == 0b1000000100001000) return true;
    return false;
}

bool isSbcd(uint16_t opcode)
{
    if ((opcode & 0b1111000111111000) == 0b1100000100000000) return true;
    if ((opcode & 0b1111000111111000) == 0b1100000100001000) return true;
    return false;
}

bool isNbcd(uint16_t opcode)
{
    if ((opcode & 0b1111111111000000) == 0b0100100000000000) return true;
    return false;
}

bool isBcd(uint16_t opcode)
{
    return isAbcd(opcode) || isSbcd(opcode) || isNbcd(opcode);
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

void setRegsMusashi(u32 dn[8], u32 an[8])
{
    m68k_set_reg(M68K_REG_D0, dn[0]);
    m68k_set_reg(M68K_REG_D1, dn[1]);
    m68k_set_reg(M68K_REG_D2, dn[2]);
    m68k_set_reg(M68K_REG_D3, dn[3]);
    m68k_set_reg(M68K_REG_D4, dn[4]);
    m68k_set_reg(M68K_REG_D5, dn[5]);
    m68k_set_reg(M68K_REG_D6, dn[6]);
    m68k_set_reg(M68K_REG_D7, dn[7]);

    m68k_set_reg(M68K_REG_A0, an[0]);
    m68k_set_reg(M68K_REG_A1, an[1]);
    m68k_set_reg(M68K_REG_A2, an[2]);
    m68k_set_reg(M68K_REG_A3, an[3]);
    m68k_set_reg(M68K_REG_A4, an[4]);
    m68k_set_reg(M68K_REG_A5, an[5]);
    m68k_set_reg(M68K_REG_A6, an[6]);
    m68k_set_reg(M68K_REG_A7, an[7]);
}

void setRegsMoira(u32 dn[8], u32 an[8])
{
    for (int n = 0; n < 8; n++) moiracpu->writeD(n, dn[n]);
    for (int n = 0; n < 8; n++) moiracpu->writeA(n, an[n]);
}

void setupMusashi()
{
    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    // m68k_set_int_ack_callback(interrupt_handler);
}

void setupMoira()
{
}

void resetMusashi()
{
    uint32_t dn[8] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80 };
    uint32_t an[8] = { 0x90, 0x10000, 0x11011, 0x12033, 0x13000, 0x14000000, 0x80000000 };

    setRegsMusashi(dn, an);
    m68k_set_reg(M68K_REG_USP, 0);
    m68k_set_reg(M68K_REG_ISP, 0);
    m68k_set_reg(M68K_REG_MSP, 0);
    m68k_set_reg(M68K_REG_SR, 0);

    m68k_pulse_reset();
}

void resetMoira()
{
    moiracpu->reset();
}

void dasmTest()
{
    int iteration = 0;
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
    setupMoira();

    // We run the tests in an infinite loop
    while (1) {

        printf("Iteration %d:", ++iteration);

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
}

void run()
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

    printf("Peforming tests... \n");
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
