// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Testrunner.h"

TestCPU *moiracpu;
Sandbox sandbox;
u8 musashiMem[0x10000];
u8 moiraMem[0x10000];
u32 musashiFC = 0;
long testrun = 0;
int cpuType = 0;

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

void selectCore(int core)
{
    cpuType = core;

    setupMusashi();
    setupMoira();

    printf("\n");
    printf("Emulated CPU: Motorola %d\n\n", cpuType);
    printf("              Exec range: %s\n", TOSTRING(doExec(opcode)));
    printf("              Dasm range: %s\n", TOSTRING(doDasm(opcode)));
    printf("\n");
}

void setupMusashi()
{
    m68k_init();

    switch (cpuType) {

        case 68000: m68k_set_cpu_type(M68K_CPU_TYPE_68000); break;
        case 68010: m68k_set_cpu_type(M68K_CPU_TYPE_68010); break;
        case 68020: m68k_set_cpu_type(M68K_CPU_TYPE_68020); break;

        default:
            assert(false);
    }
}

void setupMoira()
{
    switch (cpuType) {

        case 68000: moiracpu->setCore(M68000); break;
        case 68010: moiracpu->setCore(M68010); break;
        case 68020: moiracpu->setCore(M68020); break;

        default:
            assert(false);
    }
}

void createTestCase(Setup &s)
{
    s.supervisor = testrun % 2;
    s.ccr = u8(smartRandom());
    s.ext1 = u16(smartRandom());
    s.ext2 = u16(smartRandom());
    s.ext3 = u16(smartRandom());
    s.vbr = u16(smartRandom());
    s.sfc = u16(smartRandom());
    s.dfc = u16(smartRandom());
    for (int i = 0; i < 8; i++) s.d[i] = smartRandom();
    for (int i = 0; i < 8; i++) s.a[i] = smartRandom();

    for (unsigned i = 0; i < sizeof(s.mem); i++) {
        s.mem[i] = u8(smartRandom());
    }
}

void setupInstruction(Setup &s, u32 pc, u16 opcode)
{
    if (opcode == 0x4E73) {                     // RTE

    }

    if (opcode == 0x4E7A || opcode == 0x4E7B) { // MOVEC

        switch (smartRandom() & 5) {
            case 0: s.ext1 = (s.ext1 & 0xF000) | 0x000; break;
            case 1: s.ext1 = (s.ext1 & 0xF000) | 0x001; break;
            case 2: s.ext1 = (s.ext1 & 0xF000) | 0x800; break;
            case 3: s.ext1 = (s.ext1 & 0xF000) | 0x801; break;
        }
    }

    s.pc = pc;
    s.opcode = opcode;

    set16(s.mem, pc, opcode);
    set16(s.mem, pc + 2, s.ext1);
    set16(s.mem, pc + 4, s.ext2);
    set16(s.mem, pc + 6, s.ext3);

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
    m68k_set_reg(M68K_REG_VBR, s.vbr);
    m68k_set_reg(M68K_REG_SFC, s.sfc);
    m68k_set_reg(M68K_REG_DFC, s.dfc);
    m68ki_set_ccr(s.ccr);
    s.supervisor ? m68ki_set_sm_flag(SFLAG_SET) : m68ki_set_sm_flag(SFLAG_CLEAR);
}

void resetMoira(Setup &s)
{
    moiracpu->reset();
    moiracpu->setClock(0);

    for (int i = 0; i < 8; i++) {
        moiracpu->setD(i,s.d[i]);
    }
    for (int i = 0; i < 7; i++) {
        moiracpu->setA(i,s.a[i]);
    }
    moiracpu->setVBR(s.vbr);
    moiracpu->setSFC(s.sfc);
    moiracpu->setDFC(s.dfc);
    moiracpu->setCCR(s.ccr);
    moiracpu->setSupervisorMode(s.supervisor);
}

clock_t muclk = 0, moclk = 0;

void run()
{
    Setup setup;

    printf("Moira CPU tester. (C) Dirk W. Hoffmann, 2019 - 2022\n\n");
    printf("The test program runs Moira agains Musashi with randomly generated data.\n");
    printf("It runs until a bug has been found.\n");

    selectCore(68020);
    srand(0);

    for (testrun = 1 ;; testrun++) {

        // Switch the CPU core from time to time

        if (testrun % 16 == 0) {

            selectCore(cpuType == 68000 ? 68010 :
                       cpuType == 68010 ? 68020 : 68000);
        }
        
        printf("Round %ld ", testrun); fflush(stdout);
        createTestCase(setup);

        // Iterate through all opcodes
        for (int opcode = 0x0000; opcode < 65536; opcode++) {

            if ((opcode & 0xFFF) == 0) { printf("."); fflush(stdout); }

            // REMOVE ASAP
            // if ((opcode >> 12) != 0b1110) continue;

            if constexpr (SKIP_ILLEGAL) {
                if (moiracpu->getInfo(opcode).I == moira::ILLEGAL) continue;
            }

            // Prepare the test case with the selected instruction
            setupInstruction(setup, pc, u16(opcode));

            // Reset the sandbox (memory accesses observer)
            sandbox.prepare(u16(opcode));

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

    // Run
    muclk += runMusashi(s, mur);
    moclk += runMoira(s, mor);

    // Compare
    compare(s, mur, mor);
}

clock_t runMusashi(Setup &s, Result &r)
{
    clock_t elapsed = 0;

    r.oldpc = m68k_get_reg(NULL, M68K_REG_PC);
    r.opcode = get16(musashiMem, r.oldpc);

    switch (cpuType) {

        case 68000:
            r.dasmCnt = m68k_disassemble(r.dasm, s.pc, M68K_CPU_TYPE_68000);
            break;
        case 68010:
            r.dasmCnt = m68k_disassemble(r.dasm, s.pc, M68K_CPU_TYPE_68010);
            break;
        case 68020:
            r.dasmCnt = m68k_disassemble(r.dasm, s.pc, M68K_CPU_TYPE_68020);
            break;
    }

    if (VERBOSE)
        printf("$%04x ($%04x): %s (Musashi)\n", r.oldpc, r.opcode, r.dasm);

    // Run Musashi
    elapsed = clock();
    r.cycles = m68k_execute(1);
    elapsed = clock() - elapsed;

    // Record the result
    recordMusashiRegisters(r);

    return elapsed;
}

clock_t runMoira(Setup &s, Result &r)
{
    clock_t elapsed = 0;

    r.oldpc = moiracpu->getPC();
    r.opcode = get16(moiraMem, r.oldpc);
    r.dasmCnt = moiracpu->disassemble(r.oldpc, r.dasm);

    u32 pc = moiracpu->getPC();
    u16 op = get16(moiraMem, pc);

    int64_t cycles = moiracpu->getClock();

        if (VERBOSE)
            printf("$%04x ($%04x): %s (Moira)\n", r.oldpc, r.opcode, r.dasm);

    // Run Moira
    elapsed = clock();
    moiracpu->execute();
    elapsed = clock() - elapsed;

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
    r.sr = (u16)m68k_get_reg(NULL, M68K_REG_SR);
    r.usp = m68k_get_reg(NULL, M68K_REG_USP);
    r.isp = m68k_get_reg(NULL, M68K_REG_ISP);
    r.msp = m68k_get_reg(NULL, M68K_REG_MSP);
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
    r.isp = moiracpu->getISP();
    r.msp = moiracpu->getMSP();
    r.fc = moiracpu->readFC();
    r.vbr = moiracpu->getVBR();
    r.sfc = moiracpu->getSFC();
    r.dfc = moiracpu->getDFC();
    for (int i = 0; i < 8; i++) r.d[i] = moiracpu->getD(i);
    for (int i = 0; i < 8; i++) r.a[i] = moiracpu->getA(i);
}

void dumpSetup(Setup &s)
{
    printf("PC: %04x  Opcode: %04x  ", s.pc, s.opcode);
    printf("Ext1: %04x  Ext2: %04x  Ext3: %04x ", s.ext1, s.ext2, s.ext3);
    printf("%s\n", s.supervisor ? "(SUPERVISOR MODE)" : "");
    printf("         ");
    printf("CCR: %02x  ", s.ccr);
    printf("VBR: %02x  ", s.vbr);
    printf("SFC: %02x  ", s.sfc);
    printf("DFC: %02x\n", s.dfc);
    printf("         ");
    printf("Dn: ");
    for (int i = 0; i < 8; i++) printf("%8x ", s.d[i]);
    printf("\n");
    printf("         ");
    printf("An: ");
    for (int i = 0; i < 8; i++) printf("%8x ", s.a[i]);
    printf("\n\n");
}

void dumpResult(Result &r)
{
    // printf("Old PC: %4x (opcode %x)  ", r.oldpc, r.opcode);
    printf("PC: %04x  Elapsed cycles: %d\n", r.pc, r.cycles);
    printf("         ");
    printf("SR: %x  ", r.sr);
    printf("USP: %x  ", r.usp);
    printf("ISP: %x  ", r.isp);
    printf("MSP: %x  ", r.msp);
    printf("FC: %x  ", r.fc);
    printf("VBR: %x  ", r.vbr);
    printf("SFC: %x  ", r.sfc);
    printf("DFC: %x  ", r.dfc);
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

    if (doDasm((int)s.opcode)) {

        if (!compareDasm(r1, r2)) {
            printf("\nDISASSEMBLER MISMATCH FOUND");
            error = true;
        }
    }

    if (doExec((int)s.opcode)) {

        if (!comparePC(r1, r2)) {
            printf("\nPROGRAM COUNTER MISMATCH FOUND");
            error = true;
        }
        if (!compareCycles(r1, r2)) {
            printf("\nCLOCK MISMATCH FOUND");
            error = true;
        }

        if (!skip(s.opcode)) {

            if (!compareSR(r1, r2)) {
                printf("\nSTATUS REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareUSP(r1, r2)) {
                printf("\nUSP MISMATCH FOUND");
                error = true;
            }
            if (!compareISP(r1, r2)) {
                printf("\nISP MISMATCH FOUND");
                error = true;
            }
            if (!compareMSP(r1, r2)) {
                printf("\nMSP MISMATCH FOUND");
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
                printf("\nVBR REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareSFC(r1, r2)) {
                printf("\nSFC REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareDFC(r1, r2)) {
                printf("\nDFC REGISTER MISMATCH FOUND");
                error = true;
            }
            if (sandbox.getErrors()) {
                printf("\nSANDBOX ERRORS REPORTED");
                error = true;
            }
        }
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

bool compareUSP(Result &r1, Result &r2)
{
    return r1.usp == r2.usp;
}

bool compareISP(Result &r1, Result &r2)
{
    return r1.isp == r2.isp;
}

bool compareMSP(Result &r1, Result &r2)
{
    return r1.msp == r2.msp;
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
    if (cpuType == 68010) {

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
