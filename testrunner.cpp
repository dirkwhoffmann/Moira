// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "testrunner.h"

TestCPU *moiracpu;
uint8_t musashiMem[0x10000];
uint8_t moiraMem[0x10000];
u32 musashiFC = 0;
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
    m68k_set_cpu_type(CPUTYPE);
}

void setupMoira()
{
    switch (CPUTYPE) {

        case M68K_CPU_TYPE_68000:

            printf("Emulated CPU: Motorola 68000\n");
            moiracpu->setModel(M68000);
            break;

        case M68K_CPU_TYPE_68010:

            printf("Emulated CPU: Motorola 68010\n");
            moiracpu->setModel(M68010);
            break;

        default:

            printf("Unsupported CPU model (%d)\n", CPUTYPE);
            break;
    }
}

void createTestCase(Setup &s)
{
    s.supervisor = smartRandom() % 2;
    s.ccr = (uint8_t)smartRandom();
    s.ext1 = (uint16_t)smartRandom();
    s.ext2 = (uint16_t)smartRandom();
    for (int i = 0; i < 8; i++) s.d[i] = smartRandom();
    for (int i = 0; i < 8; i++) s.a[i] = smartRandom();

    for (unsigned i = 0; i < sizeof(s.mem); i++) {
        s.mem[i] = (uint8_t)smartRandom();
    }
}

void setupInstruction(Setup &s, uint32_t pc, uint16_t opcode)
{
    s.pc = pc;
    s.opcode = opcode;

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

    printf("Moira CPU tester. (C) Dirk W. Hoffmann, 2019 - 2022\n\n");
    printf("The test program runs Moira agains Musashi with randomly generated data.\n");
    printf("It runs until a bug has been found.\n\n");

    setupMusashi();
    setupMoira();

    printf("\n");
    srand(0);

    for (long round = 1 ;; round++) {

        printf("Round %ld ", round); fflush(stdout);
        createTestCase(setup);

        // Iterate through all opcodes
        for (int opcode = 0x0000; opcode < 65536; opcode++) {

            if ((opcode & 0xFFF) == 0) { printf("."); fflush(stdout); }

            // Prepare the test case with the selected instruction
            setupInstruction(setup, pc, (uint16_t)opcode);

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
    r.dasmCnt = m68k_disassemble(r.dasm, r.oldpc, CPUTYPE);

    if (!skip(r.opcode)) {
        
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

    int64_t cycles = moiracpu->getClock();
    if (!skip(op)) {

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

bool skip(u16 op)
{
    bool result;

    moira::Instr instr = moiracpu->getInfo(op).I;

    // Skip some instructions that are broken in Musashi
    result =
    instr == moira::ABCD    ||
    instr == moira::SBCD    ||
    instr == moira::NBCD;

    return result;
}

void recordMusashiRegisters(Result &r)
{
    r.pc = m68k_get_reg(NULL, M68K_REG_PC);
    r.sr = (uint16_t)m68k_get_reg(NULL, M68K_REG_SR);
    r.usp = m68k_get_reg(NULL, M68K_REG_USP);
    r.ssp = m68k_get_reg(NULL, M68K_REG_ISP);
    r.fc = musashiFC;
    r.vbr = m68k_get_reg(NULL, M68K_REG_VBR);
    r.sfc = m68k_get_reg(NULL, M68K_REG_SFC);
    r.dfc = m68k_get_reg(NULL, M68K_REG_DFC);
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
    r.fc = moiracpu->readFC();
    r.vbr = moiracpu->getVBR();
    r.sfc = moiracpu->getSFC();
    r.dfc = moiracpu->getDFC();
    for (int i = 0; i < 8; i++) r.d[i] = moiracpu->getD(i);
    for (int i = 0; i < 8; i++) r.a[i] = moiracpu->getA(i);
}

void dumpSetup(Setup &s)
{
    printf("PC: %04x ", s.pc);
    printf("Opcode: %04x Ext1: %04x Ext2: %04x ", s.opcode, s.ext1, s.ext2);
    printf("CCR: %02x %s", s.ccr, s.supervisor ? "SUPERVISOR MODE\n" : "\n");
    printf("         Dn: ");
    for (int i = 0; i < 8; i++) printf("%8x ", s.d[i]);
    printf("\n");
    printf("         An: ");
    for (int i = 0; i < 8; i++) printf("%8x ", s.a[i]);
    printf("\n\n");
}

void dumpResult(Result &r)
{
    printf("Old PC: %4x (opcode %x)  ", r.oldpc, r.opcode);
    printf("New PC: %4x  ", r.pc);
    printf("Elapsed cycles: %d\n" , r.cycles);
    printf("         SR: %x  ", r.sr);
    printf("SSP: %x  ", r.ssp);
    printf("USP: %x  ", r.usp);
    printf("FC: %x  ", r.fc);

    if (CPUTYPE >= M68K_CPU_TYPE_68010) {

        printf("SFC: %x  ", r.sfc);
        printf("DFC: %x  ", r.dfc);
        printf("VBR: %x  ", r.vbr);
    }
    printf("\n");

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
    if (!compareVBR(r1, r2)) {
        printf("\nVBR REGISTER MISMATCH FOUND\n");
        error = true;
    }
    if (!compareSFC(r1, r2)) {
        printf("\nSFC REGISTER MISMATCH FOUND\n");
        error = true;
    }
    if (!compareDFC(r1, r2)) {
        printf("\nDFC REGISTER MISMATCH FOUND\n");
        error = true;
    }
    if (sandbox.getErrors()) {
        printf("\nSANDBOX ERRORS REPORTED\n");
        error = true;
    }

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

bool compareVBR(Result &r1, Result &r2)
{
    return r1.vbr == r2.vbr;
}

bool compareSFC(Result &r1, Result &r2)
{
    return r1.sfc == r2.sfc;
}

bool compareDFC(Result &r1, Result &r2)
{
    return r1.dfc == r2.dfc;
}

bool compareCycles(Result &r1, Result &r2)
{
    assert(r1.opcode == r2.opcode);
    auto I = moiracpu->getInfo(r1.opcode).I;
    auto S = moiracpu->getInfo(r1.opcode).S;
    auto M = moiracpu->getInfo(r1.opcode).M;

    // Exclude some instructions
    if (I == moira::TAS) return true;
    if (CPUTYPE == M68K_CPU_TYPE_68010) {

        if (I == moira::CLR && S == Byte && M == MODE_AL) return true;
        if (I == moira::CLR && S == Word && M == MODE_AL) return true;
        if (I == moira::TRAPV) return true;
    }

    return r1.cycles == r2.cycles;
}

void bugReport()
{
    printf("Please send a bug report to: dirk.hoffmann@me.com\n");
    printf("Thanks you!\n\n");
    exit(0);
}
