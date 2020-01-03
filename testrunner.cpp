// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "testrunner.h"

moira::Moira *moiracpu;
Tester_68k *tester;
// uint8_t mem[0x10000];
uint8_t musashiMem[0x10000];
uint8_t moiraMem[0x10000];
Sandbox sandbox;

uint32 smartRandom()
{
    switch (rand() % 16) {

        case  0: // 8-bit numbers
        case  1: return rand() & 0xFF;
        case  2: return 0x80;
        case  3: return 0xFF;

        case  4: // 16-bit numbers
        case  5: return rand() & 0xFFFF;
        case  6: return 0x8000;
        case  7: return 0xFFFF;

        case  8: // 32-bit numbers
        case  9: return rand() & 0xFFFFFFFF;
        case 10: return 0x80000000;
        case 11: return 0xFFFFFFFF;

        default: return 0;
    }
}

void setupMusashi()
{
    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
}

void setupMoira()
{

}

void createTestCase(Setup &s)
{
    s.supervisor = smartRandom() % 2;
    s.ccr = smartRandom();

    for (int i = 0; i < 8; i++) s.d[i] = smartRandom();
    for (int i = 0; i < 8; i++) s.a[i] = smartRandom();

    for (unsigned i = 0; i < sizeof(s.mem); i++) {
        s.mem[i] = smartRandom();
    }
}

void setupInstruction(Setup &s, uint32_t pc, uint16_t opcode)
{
    s.pc = pc;
    s.opcode = opcode;

    s.mem[pc] = opcode >> 8;
    s.mem[pc + 1] = opcode & 0xFF;
}

void resetMusashi(Setup &s)
{
    memcpy(musashiMem, s.mem, sizeof(musashiMem));

    m68k_set_reg(M68K_REG_USP, 0);
    m68k_set_reg(M68K_REG_ISP, 0);
    m68k_set_reg(M68K_REG_MSP, 0);

    m68k_pulse_reset();

    for (int i = 0; i < 8; i++) {
        m68k_set_reg((m68k_register_t)(M68K_REG_D0 + i), s.d[i]);
    }
    for (int i = 0; i < 7; i++) {
        m68k_set_reg((m68k_register_t)(M68K_REG_A0 + i), s.a[i]);
    }
    m68ki_set_ccr(s.ccr);
    s.supervisor ? m68ki_set_sm_flag(SFLAG_SET) : m68ki_set_sm_flag(SFLAG_CLEAR);
}

void resetMoira(Setup &s)
{
    // memcpy(mem, s.mem, sizeof(mem));
    memcpy(moiraMem, s.mem, sizeof(moiraMem));

    moiracpu->reset();

    for (int i = 0; i < 8; i++) {
        moiracpu->setD(i,s.d[i]);
    }
    for (int i = 0; i < 7; i++) {
        moiracpu->setA(i,s.a[i]);
    }
    moiracpu->setCCR(s.ccr);
    moiracpu->setSupervisorMode(s.supervisor);
}

clock_t muclk = 0, moclk = 0;

void run()
{
    Setup setup;

    printf("Moira CPU tester. (C) Dirk W. Hoffmann, 2019\n\n");
    printf("The test program runs Moira agains Musashi with randomly generated data.\n");
    printf("It runs until a bug has been found.\n\n");

    setupMusashi();
    setupMoira();
    srand(0); // (int)time(NULL));

    for (long round = 1 ;; round++) {

        printf("Round %ld ", round); fflush(stdout);
        createTestCase(setup);

        // Iterate through all opcodes
        for (int opcode = 0x0000; opcode < 65536; opcode++) {

            if ((opcode & 0xFFF) == 0) { printf("."); fflush(stdout); }

            // Prepare the test case with the selected instruction
            setupInstruction(setup, pc, opcode);
            if (VERBOSE) {
                char mos[128];
                moiracpu->disassemble(setup.pc, mos);
                printf("$%04x: %s\n", setup.opcode, mos);
            }

            // Test the disassembler
            runDasmTest(setup);

            // Reset the sandbox (memory accesses observer)
            sandbox.prepare();

            runSingleTest(setup);
        }

        printf(" PASSED (Musashi: %.2fs Moira: %.2fs)\n",
               muclk / double(CLOCKS_PER_SEC),
               moclk / double(CLOCKS_PER_SEC));
    }
}

void runDasmTest(Setup &s)
{
    if (TEST_DASM) {

        int muc, moc;
        char mus[128], mos[128];

        muc = m68k_disassemble(mus, s.pc, M68K_CPU_TYPE_68000);
        moc = moiracpu->disassemble(s.pc, mos);
        compare(muc, moc, mus, mos);
    }
}

void runSingleTest(Setup &s)
{
    Result mur, mor;

    // Prepare Musashi
    resetMusashi(s);

    // Run Musashi
    muclk += runMusashi(s, mur);

    // Prepare Moira
    resetMoira(s);

    // Run Moira
    moclk += runMoira(s, mor);

    // Compare
    compare(s, mur, mor);
}

clock_t runMusashi(Setup &s, Result &r)
{
    clock_t elapsed = 0;
    u32 pc = m68k_get_reg(NULL, M68K_REG_PC);
    u16 op = get16(musashiMem, pc);
    moira::Instr i = moiracpu->getInfo(op).I;

    // Skip some instructions that are likely to be broken in Musashi
    bool skip =
    i == moira::ABCD    ||
    i == moira::SBCD    ||
    i == moira::NBCD    ||
    i == moira::ILLEGAL ||
    i == moira::LINE_A  ||
    i == moira::LINE_F;

    if (!skip) {
        elapsed = clock();
        r.cycles = m68k_execute(1);
        elapsed = clock() - elapsed;
    } else {
        r.cycles = 0;
    }

    // Record the result
    r.pc = m68k_get_reg(NULL, M68K_REG_PC);
    r.sr = m68k_get_reg(NULL, M68K_REG_SR);
    r.usp = m68k_get_reg(NULL, M68K_REG_USP);
    r.ssp = m68k_get_reg(NULL, M68K_REG_ISP);
    for (int i = 0; i < 8; i++) {
        r.d[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + i));
        r.a[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_A0 + i));
    }

    // Post-process the result to make it comparable with Moira

    return elapsed;
}

clock_t runMoira(Setup &s, Result &r)
{
    clock_t elapsed = 0;
    u32 pc = moiracpu->getPC();
    u16 op = get16(musashiMem, pc);
    moira::Instr i = moiracpu->getInfo(op).I;

    // Skip some instructions that are likely to be broken in Musashi
    bool skip =
    i == moira::ABCD    ||
    i == moira::SBCD    ||
    i == moira::NBCD    ||
    i == moira::ILLEGAL ||
    i == moira::LINE_A  ||
    i == moira::LINE_F;

    int64_t cycles = moiracpu->getClock();
    if (!skip) {
        elapsed = clock();
        moiracpu->execute();
        elapsed = clock() - elapsed;
    }

    // Record the result
    r.cycles = (int)(moiracpu->getClock() - cycles);
    r.pc = moiracpu->getPC();
    r.sr = moiracpu->getSR();
    for (int i = 0; i < 8; i++) r.d[i] = moiracpu->getD(i);
    for (int i = 0; i < 8; i++) r.a[i] = moiracpu->getA(i);

    return elapsed;
}

void dumpSetup(Setup &s)
{
    printf("PC: %4x ", s.pc);
    printf("CCR: %2x\n", s.ccr);
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

    if (!comparePC(s, r1, r2)) {
        printf("\n\nPC MISMATCH FOUND\n");
        error = true;
    }
    if (!compareSR(s, r1, r2)) {
        printf("\n\nSR MISMATCH FOUND\n");
        error = true;
    }
    if (!compareCycles(s, r1, r2)) {
        printf("\n\nCLOCK MISMATCH FOUND\n");
        error = true;
    }
    if (!compareD(s, r1, r2)) {
        printf("\n\nDATA REGISTER MISMATCH FOUND\n");
        error = true;
    }
    if (!compareA(s, r1, r2)) {
        printf("\n\nADDRESS REGISTER MISMATCH FOUND\n");
        error = true;
    }
    /*
    if (!compareIRD(s, r1, r2)) {
        printf("\n\nWRONG IRD VALUE: %x\n", moiracpu->getIRD());
        error = true;
    }
    if (!compareIRC(s, r1, r2)) {
        printf("\n\nWRONG IRC VALUE: %x\n", moiracpu->getIRC());
        error = true;
    }
    */

    error |= (sandbox.getErrors() != 0);

    if (error) {

        moiracpu->disassemble(s.pc, str);
        printf("\nInstruction: %s ", str);
        printf("(opcode $%x out of $FFFF)\n\n", s.opcode);

        printf("Setup:   ");
        dumpSetup(s);

        printf("Musashi: ");
        dumpResult(r1);

        printf("Moira:   ");
        dumpResult(r2);

        bugReport();
    }
}

bool compareD(Setup &s, Result &r1, Result &r2)
{
    moira::Instr instr = moiracpu->getInfo(s.opcode).I;
    if (instr == moira::DIVU || instr == moira::DIVS)
    {
        // Musashi differs in some corner cases
        return true;
    }

    for (int i = 0; i < 8; i++) if (r1.d[i] != r2.d[i]) return false;
    return true;
}

bool compareA(Setup &s, Result &r1, Result &r2)
{
    for (int i = 0; i < 8; i++) if (r1.a[i] != r2.a[i]) return false;
    return true;
}

bool comparePC(Setup &s, Result &r1, Result &r2)
{
    return r1.pc == r2.pc;
}

bool compareSR(Setup &s, Result &r1, Result &r2)
{
    moira::Instr instr = moiracpu->getInfo(s.opcode).I;
    if (instr == moira::DIVU || instr == moira::DIVS) {

        // Musashi differs (and is likely wrong). Ignore it
        return true;
    }

    return r1.sr == r2.sr;
}

bool compareIRD(Setup &s, Result &r1, Result &r2)
{
    // Exclude STOP command which doesn't perform a prefetch
    if (moiracpu->getInfo(s.opcode).I == moira::STOP) return true;

    return moiracpu->getIRD() == get16(moiraMem, r2.pc);
}

bool compareIRC(Setup &s, Result &r1, Result &r2)
{
    // Exclude STOP command which doesn't perform a prefetch
    if (moiracpu->getInfo(s.opcode).I == moira::STOP) return true;

    return moiracpu->getIRC() == get16(moiraMem, r2.pc + 2);
}

bool compareCycles(Setup &s, Result &r1, Result &r2)
{
    moira::Instr instr = moiracpu->getInfo(s.opcode).I;

    // Ignore instruction that are wrong in Musashi
    if (instr == moira::MULS ||
        instr == moira::MULU ||
        instr == moira::DIVS ||
        instr == moira::DIVU ||
        instr == moira::BCLR ||
        instr == moira::BSET ||
        instr == moira::BCHG ||
        instr == moira::CHK  ||
        instr == moira::TAS) return true;

    return r1.cycles == r2.cycles;
}

void compare(int c1, int c2, char *s1, char *s2)
{
    if (c1 != c2 || strcmp(s1, s2) != 0) {

        printf("\nDISASSEMBLER MISMATCH FOUND:\n\n");
        printf("    Musashi: %s\n", s1);
        printf("      Moira: %s\n", s2);
        printf("      Bytes: %d / %d\n\n", c1, c2);

        bugReport();
    }
}

void bugReport()
{
    printf("Please send a bug report to: dirk.hoffmann@me.com\n");
    printf("Thanks you!\n\n");
    assert(false);
}
