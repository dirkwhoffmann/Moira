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

        // Reset the sandbox which oberseves memory accesses
        moiracpu->sandbox.prepare();

        // Setup a test case
        setupTestCase(setup, pc, opcode);

        // Make Musashi ready to run the test
        resetMusashi(setup);

        // Disassemble instruction
        // m68k_disassemble(musashiStr, pc, M68K_CPU_TYPE_68000);
        // printf("Instruction: %s (Musashi)\n", musashiStr);

        // Run Musashi
        mur.cycles = m68k_execute(1);

        // Record the result
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
        compare(setup, mur, mor);
    }

    clock_t now = clock();
    double elapsed = (double(now - start) / double(CLOCKS_PER_SEC));
    printf("PASSED (%f sec)\n", elapsed);
}

void dumpSetup(Setup &s)
{
    printf("PC: %4x ", s.pc);
    printf("SR: %2x\n", s.sr);
    printf("         Dn: ");
    for (int i = 0; i < 8; i++) printf("%8x ", s.d[i]);
    printf("\n");
    printf("         An: ");
    for (int i = 0; i < 8; i++) printf("%8x ", s.a[i]);
    printf("\n\n");
}

void dumpResult(Result &r)
{
    printf("PC: %4x ", r.pc);
    printf("SR: %2x ", r.sr);
    printf("Elapsed cycles: %d\n" , r.cycles);

    printf("         Dn: ");
    for (int i = 0; i < 8; i++) printf("%8x ", r.d[i]);
    printf("\n");
    printf("         An: ");
    for (int i = 0; i < 8; i++) printf("%8x ", r.a[i]);
    printf("\n\n");
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

        printf("Setup:   ");
        dumpSetup(s);

        printf("Musashi: ");
        dumpResult(r1);

        printf("Moira:   ");
        dumpResult(r2);

        printf("Please send a bug report to: dirk.hoffmann@me.com\n");
        printf("Thanks you!\n\n");
        assert(false);
    }
}
