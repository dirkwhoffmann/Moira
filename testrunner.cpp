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
    s.ext1 = smartRandom();
    s.ext2 = smartRandom();
    for (int i = 0; i < 8; i++) s.d[i] = smartRandom();
    for (int i = 0; i < 8; i++) s.a[i] = smartRandom();

    for (unsigned i = 0; i < sizeof(s.mem); i++) {
        s.mem[i] = smartRandom();
    }
}

void setupInstruction(Setup &s, uint32_t pc, uint16_t opcode)
{
    s.pc = pc;
    // s.opcode = opcode;

    set16(s.mem, pc, opcode);
    set16(s.mem, pc + 2, s.ext1);
    set16(s.mem, pc + 4, s.ext2);

    memcpy(musashiMem, s.mem, sizeof(musashiMem));
    memcpy(moiraMem, s.mem, sizeof(moiraMem));
}

void resetMusashi(Setup &s)
{
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

            // Reset the sandbox (memory accesses observer)
            sandbox.prepare();

            // Execute both CPU cores
            runSingleTest(setup);
        }

        printf(" PASSED (Musashi: %.2fs Moira: %.2fs)\n",
               muclk / double(CLOCKS_PER_SEC),
               moclk / double(CLOCKS_PER_SEC));
    }
}

void runSingleTest(Setup &s)
{
    Result mur, mor;

    // Prepare
    resetMusashi(s);
    resetMoira(s);

    for (int i = 0; i < RUNS; i++) {

        // Compare
        // compare(s, mur, mor);

        // Run
        muclk += runMusashi(i, s, mur);
        moclk += runMoira(i, s, mor);

        // Compare
        compare(s, mur, mor);
    }
}

clock_t runMusashi(int i, Setup &s, Result &r)
{
    clock_t elapsed = 0;

    r.oldpc = m68k_get_reg(NULL, M68K_REG_PC);
    r.opcode = get16(musashiMem, r.oldpc);
    r.dasmCnt = m68k_disassemble(r.dasm, r.oldpc, M68K_CPU_TYPE_68000);

    moira::Instr instr = moiracpu->getInfo(r.opcode).I;

    // Skip some instructions that are likely to be broken in Musashi
    bool skip =
    instr == moira::ABCD    ||
    instr == moira::SBCD    ||
    instr == moira::NBCD;

    if (!skip) {
        
        if (VERBOSE)
            printf("%d: $%04x ($%04x): %s (Musashi)\n", i, r.oldpc, r.opcode, r.dasm);

        elapsed = clock();
        r.cycles = m68k_execute(1);
        elapsed = clock() - elapsed;

    } else {
        r.cycles = 0;
    }

    // Record the result
    recordMusashiRegisters(r);

    return elapsed;
}

clock_t runMoira(int i, Setup &s, Result &r)
{
    clock_t elapsed = 0;

    r.oldpc = moiracpu->getPC();
    r.opcode = get16(moiraMem, r.oldpc);
    r.dasmCnt = moiracpu->disassemble(r.oldpc, r.dasm);

    u32 pc = moiracpu->getPC();
    u16 op = get16(moiraMem, pc);
    moira::Instr instr = moiracpu->getInfo(op).I;

    // Skip some instructions that are likely to be broken in Musashi
    bool skip =
    instr == moira::ABCD    ||
    instr == moira::SBCD    ||
    instr == moira::NBCD;

    int64_t cycles = moiracpu->getClock();
    if (!skip) {

        if (VERBOSE)
            printf("%d: $%04x ($%04x): %s (Moira)\n", i, r.oldpc, r.opcode, r.dasm);

        elapsed = clock();
        moiracpu->execute();
        elapsed = clock() - elapsed;
    }

    // Record the result
    r.cycles = (int)(moiracpu->getClock() - cycles);
    r.oldpc = pc;
    r.opcode = op;
    recordMoiraRegisters(r);

    return elapsed;
}

void recordMusashiRegisters(Result &r)
{
    r.pc = m68k_get_reg(NULL, M68K_REG_PC);
    r.sr = m68k_get_reg(NULL, M68K_REG_SR);
    r.usp = m68k_get_reg(NULL, M68K_REG_USP);
    r.ssp = m68k_get_reg(NULL, M68K_REG_ISP);
    for (int i = 0; i < 8; i++) {
        r.d[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + i));
        r.a[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_A0 + i));
    }
}

void recordMoiraRegisters(Result &r)
{
    r.pc = moiracpu->getPC();
    r.sr = moiracpu->getSR();
    r.usp = moiracpu->getUSP();
    r.ssp = moiracpu->getSSP();
    for (int i = 0; i < 8; i++) r.d[i] = moiracpu->getD(i);
    for (int i = 0; i < 8; i++) r.a[i] = moiracpu->getA(i);
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
    printf("Old PC: %4x (opcode %x)\n", r.oldpc, r.opcode);
    printf("         New PC: %4x ", r.pc);
    printf("SR: %2x ", r.sr);
    printf("SSP: %2x ", r.ssp);
    printf("USP: %2x ", r.usp);
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

    if (!compareDasm(r1, r2)) {
        printf("\nDISASSEMBLER MISMATCH FOUND");
        error = true;
    }
    if (!comparePC(r1, r2)) {
        printf("\nPROGRAM COUNTER MISMATCH FOUND");
        error = true;
    }
    if (!compareSR(r1, r2)) {
        printf("\nSTATUS REGISTER MISMATCH FOUND");
        error = true;
    }
    if (!compareSP(r1, r2)) {
        printf("\nSTACK POINTER MISMATCH FOUND");
        error = true;
    }
    if (!compareCycles(r1, r2)) {
        printf("\nCLOCK MISMATCH FOUND");
        error = true;
    }
    if (!compareD(r1, r2)) {
        printf("\nDATA REGISTER MISMATCH FOUND");
        error = true;
    }
    if (!compareA(r1, r2)) {
        printf("\nADDRESS REGISTER MISMATCH FOUND");
        error = true;
    }
    if (!compareIRD(r1, r2)) {
        printf("\n\nWRONG IRD VALUE: %x\n", moiracpu->getIRD());
        error = true;
    }
    if (!compareIRC(r1, r2)) {
        printf("\n\nWRONG IRC VALUE: %x\n", moiracpu->getIRC());
        error = true;
    }

    error |= (sandbox.getErrors() != 0);

    if (error) {

        printf("\n\nInstruction: %s (Musashi)", r1.dasm);
        printf(  "\n             %s (Moira)\n\n", r2.dasm);

        printf("Setup:   ");
        dumpSetup(s);

        printf("Musashi: ");
        dumpResult(r1);

        printf("Moira:   ");
        dumpResult(r2);

        bugReport();
    }
}

bool compareDasm(Result &r1, Result &r2)
{
    if (r1.dasmCnt != r2.dasmCnt) return false;
    return strcmp(r1.dasm, r2.dasm) == 0;
}

bool compareD(Result &r1, Result &r2)
{
    assert(r1.opcode == r2.opcode);
    moira::Instr instr = moiracpu->getInfo(r1.opcode).I;
    if (instr == moira::DIVU || instr == moira::DIVS)
    {
        // Musashi differs in some corner cases
        return true;
    }

    for (int i = 0; i < 8; i++) if (r1.d[i] != r2.d[i]) return false;
    return true;
}

bool compareA(Result &r1, Result &r2)
{
    for (int i = 0; i < 8; i++) if (r1.a[i] != r2.a[i]) return false;
    return true;
}

bool comparePC(Result &r1, Result &r2)
{
    return r1.pc == r2.pc;
}

bool compareSR(Result &r1, Result &r2)
{
    assert(r1.opcode == r2.opcode);
    moira::Instr instr = moiracpu->getInfo(r1.opcode).I;
    if (instr == moira::DIVU || instr == moira::DIVS) {

        // Musashi differs (and is likely wrong). Ignore it
        return true;
    }

    return r1.sr == r2.sr;
}

bool compareSP(Result &r1, Result &r2)
{
    if (r1.ssp != r2.ssp) return false;
    if (r1.usp != r2.usp) return false;

    return true;
}

bool compareIRD(Result &r1, Result &r2)
{
    /*
    assert(r1.opcode == r2.opcode);

    // Exclude STOP command which doesn't perform a prefetch
    if (moiracpu->getInfo(r1.opcode).I == moira::STOP) return true;

    return moiracpu->getIRD() == get16(moiraMem, r2.pc);
    */
    return true;
}

bool compareIRC(Result &r1, Result &r2)
{
    /*
    // Exclude STOP command which doesn't perform a prefetch
    if (moiracpu->getInfo(r1.opcode).I == moira::STOP) return true;

    return moiracpu->getIRC() == get16(moiraMem, r2.pc + 2);
    */
    return true;
}

bool compareCycles(Result &r1, Result &r2)
{
    assert(r1.opcode == r2.opcode);
    moira::Instr instr = moiracpu->getInfo(r1.opcode).I;

    // Exclude some instructions
    if (instr == moira::TAS) return true;

    return r1.cycles == r2.cycles;
}

void bugReport()
{
    printf("Please send a bug report to: dirk.hoffmann@me.com\n");
    printf("Thanks you!\n\n");
    assert(false);
}
