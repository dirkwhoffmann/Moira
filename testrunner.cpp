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

void setupTestCase(Setup &s, uint32_t pc, uint16_t opcode)
{
    s.pc = pc;
    s.opcode = opcode;
    s.sr = 0;

    for (int i = 0; i < 8; i++) s.d[i] = 0;
    for (int i = 0; i < 8; i++) s.a[i] = 0;

    for (unsigned i = 0; i < sizeof(s.mem); i++) {
        s.mem[i] = (uint8_t)((7 * i) & ~1);
    }
    s.mem[pc] = opcode >> 8;
    s.mem[pc + 1] = opcode & 0xFF;

    // REMOVE ASAP
    s.mem[pc + 2] = 0x80;
    s.mem[pc + 3] = 0x00;
    s.mem[pc + 4] = 0x80;
    s.mem[pc + 5] = 0x10;
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

void resetMusashi(Setup &s)
{
    m68ki_set_sr_noint(0);

    m68k_pulse_reset();

    memcpy(mem, s.mem, sizeof(mem));

    m68k_set_reg(M68K_REG_USP, 0);
    m68k_set_reg(M68K_REG_ISP, 0);
    m68k_set_reg(M68K_REG_MSP, 0);
    // m68ki_set_sr_noint(s.sr);

    for (int i = 0; i < 8; i++) {
        m68k_set_reg((m68k_register_t)(M68K_REG_D0 + i), s.d[0]);
        m68k_set_reg((m68k_register_t)(M68K_REG_A0 + i), s.a[0]);
    }
}

void resetMoira(Setup &s)
{
    moiracpu->reset();

    memcpy(mem, s.mem, sizeof(mem));

    // moiracpu->setSR(s.sr);

    for (int i = 0; i < 8; i++) {
        moiracpu->writeD(i,s.d[i]);
        moiracpu->writeA(i,s.a[i]);
    }
}

void run()
{
    const uint32_t pc = 0x1000;

    char moiraStr[128];
    int64_t moiraCycles;
    Setup  setup;
    Result mur, mor;

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
    for (int opcode = 0x0000; opcode < 65536; opcode++) {

        if ((opcode & 0xFFF) == 0) printf("Opcodes %xxxx\n", opcode >> 12);

        // Skip illegal instructions
        if (moiracpu->isIllegalInstr(opcode)) continue;
        if (moiracpu->isLineAInstr(opcode)) continue;
        if (moiracpu->isLineFInstr(opcode)) continue;

        for (int i = 32; i < 33; i++) {
            for (int j = 34; j < 35; j++) {

                moiracpu->sandbox.prepare();

                // Prepare the test case for Musashi
                setupTestCase(setup, pc, opcode);

                // Reset Musashi
                resetMusashi(setup);

                // Disassemble instruction
                // m68k_disassemble(musashiStr, pc, M68K_CPU_TYPE_68000);
                // printf("Instruction: %s (Musashi)\n", musashiStr);

                // Run Musashi
                mur.cycles = m68k_execute(1);

                mur.pc = m68k_get_reg(NULL, M68K_REG_PC);
                mur.sr = m68k_get_reg(NULL, M68K_REG_SR);
                mur.usp = m68k_get_reg(NULL, M68K_REG_USP);
                mur.ssp = m68k_get_reg(NULL, M68K_REG_ISP);
                for (int i = 0; i < 8; i++) {
                    mur.d[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + i));
                    mur.a[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_A0 + i));
                }

                // Skip NBCD, SBCD, ABCD for now
                if (isBcd(opcode)) {
                    continue;
                }

                // Reset Moira
                resetMoira(setup);
                moiraCycles = moiracpu->getClock();

                // Disassemble instruction
                moiracpu->disassemble(pc, moiraStr);
                // printf("Instruction $%4x: %s\n", opcode, moiraStr);

                // Run Moira
                moiracpu->process();
                mor.cycles = (int)(moiracpu->getClock() - moiraCycles);
                mor.pc = moiracpu->getPC();
                mor.sr = moiracpu->getSR();
                for (int i = 0; i < 8; i++) mor.d[i] = moiracpu->readD(i);
                for (int i = 0; i < 8; i++) mor.a[i] = moiracpu->readA(i);

                // Compare results
                bool error = false;
                bool regError = false;

                // printf("Cycles %d / %d\n", musashiCycles, moiraCycles);

                error |= (mur.pc != mor.pc);
                error |= (mur.sr != mor.sr);
                error |= !isMulOrDiv(opcode) && (mur.cycles != mor.cycles);
                for (int i = 0; i < 8; i++) regError |= mur.d[i] != mor.d[i];
                for (int i = 0; i < 8; i++) regError |= mur.a[i] != mor.a[i];

                if (error || regError) {
                    moiracpu->disassemble(pc, moiraStr);
                    printf("\nMISMATCH FOUND (opcode $%x out of $FFFF):\n\n", opcode);
                    printf("Instruction: %s\n\n", moiraStr);
                    printf("    Musashi: ");
                    printf("PC: %4x ", mur.pc);
                    printf("SR: %4x ", mur.sr);
                    printf("Elapsed cycles: %d\n" , mur.cycles);
                    printf("      Moira: ");
                    printf("PC: %4x ", mor.pc);
                    printf("SR: %4x ", mor.sr);
                    printf("Elapsed cycles: %d\n\n" , mor.cycles);
                    printf("\n");

                    if (regError) {
                        printf("Musashi: Dx: %8x %8x %8x %8x %8x %8x %8x %8x\n",
                               mur.d[0], mur.d[1], mur.d[2], mur.d[3],
                               mur.d[4], mur.d[5], mur.d[6], mur.d[7]);
                        printf("         Ax: %8x %8x %8x %8x %8x %8x %8x %8x\n\n",
                               mur.a[0], mur.a[1], mur.a[2], mur.a[3],
                               mur.a[4], mur.a[5], mur.a[6], mur.a[7]);
                        printf("  Moira: Dx: %8x %8x %8x %8x %8x %8x %8x %8x\n",
                               mor.d[0], mor.d[1], mor.d[2], mor.d[3],
                               mor.d[4], mor.d[5], mor.d[6], mor.d[7]);
                        printf("  Moira: Ax: %8x %8x %8x %8x %8x %8x %8x %8x\n",
                               mor.a[0], mor.a[1], mor.a[2], mor.a[3],
                               mor.a[4], mor.a[5], mor.a[6], mor.a[7]);
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

void dumpSetup(Setup &s)
{
    printf("SR: %2x \n", s.sr);
    printf("Dn: ");
    for (int i = 0; i < 8; i++) printf("%8x ", s.d[i]);
    printf("\n");
    printf("An: ");
    for (int i = 0; i < 8; i++) printf("%8x ", s.a[i]);
}

void dumpResult(Result &r)
{
    printf("\n");
    printf("PC: %4x ", r.pc);
    printf("SR: %2x ", r.sr);
    printf("Elapsed cycles: %d\n\n" , r.cycles);

    printf("Dn: ");
    for (int i = 0; i < 8; i++) printf("%8x ", r.d[i]);
    printf("\n");
    printf("An: ");
    for (int i = 0; i < 8; i++) printf("%8x ", r.a[i]);
    printf("\n");
}

void compare(Setup &s, Result &r1, Result &r2)
{
    bool error = false;
    char str[128];

    error |= (r1.pc != r2.pc);
    error |= (r1.sr != r2.sr);
    error |= !isMulOrDiv(s.opcode) && (r1.cycles != r2.cycles);
    for (int i = 0; i < 8; i++) error |= r1.d[i] != r2.d[i];
    for (int i = 0; i < 8; i++) error |= r1.a[i] != r2.a[i];

    if (error) {

        moiracpu->disassemble(s.pc, str);
        printf("\nMISMATCH FOUND (opcode $%x out of $FFFF):\n\n", s.opcode);
        printf("Instruction: %s\n\n", str);

        printf("Setup: ");
        dumpSetup(s);

        printf("Musashi: ");
        dumpResult(r1);

        printf("Moira: ");
        dumpResult(r2);
    }
}
