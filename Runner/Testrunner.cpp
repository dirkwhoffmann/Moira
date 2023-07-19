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
#include <cstdio>
#include <cstdarg>

TestCPU *moiracpu;
Sandbox sandbox;
Randomizer randomizer;
u8 musashiMem[0x10000];
u8 moiraMem[0x10000];
u32 musashiFC = 0;
moira::Model cpuModel = M68000;
u16 opcode = 0;

// Binutils
meminfo mi;
disassemble_info di;

static int binutilsFprintf(FILE* f, const char* format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    char* buf = (char*)f;
    const int l = strlen(buf);
    const int n = vsnprintf(buf + l, sizeof(Result::dasmBinutils) - l, format, ap);va_end(ap);
    return n;
}

void selectModel(moira::Model model)
{
    cpuModel = model;

    setupMusashi();
    setupBinutils();
    setupMoira();
}

const char *selectedModel()
{
    switch (cpuModel) {

        case M68000:    return "68000";
        case M68010:    return "68010";
        case M68EC020:  return "EC020";
        case M68020:    return "68020";
        case M68EC030:  return "EC030";
        case M68030:    return "68030";
        case M68EC040:  return "EC040";
        case M68LC040:  return "LC040";
        case M68040:    return "68040";

        default:
            return "???";
    }
}

void setupBinutils()
{
    switch (cpuModel) {

        case M68000:    di.mach = MACH_68000; break;
        case M68010:    di.mach = MACH_68010; break;
        case M68EC020:  // Not supported by binutils
        case M68020:    di.mach = MACH_68020; break;
        case M68EC030:  // Not supported by binutils
        case M68030:    di.mach = MACH_68030; break;
        case M68EC040:  // Not supported by binutils
        case M68LC040:  // Not supported by binutils
        case M68040:    di.mach = MACH_68040; break;
    }

    // di.stream = binutilsStream;
    di.caller_private = &mi;
    di.memory_error_func = [](int, bfd_vma, struct disassemble_info*) {};
    di.read_memory_func = [](bfd_vma memaddr, bfd_byte* myaddr, unsigned int length, struct disassemble_info* dinfo) -> int {
        meminfo* mi = static_cast<meminfo*>(dinfo->caller_private);
        while (length--)
            *myaddr++ = memaddr < mi->len ? mi->bytes[memaddr++] : 0;
        return 0; // return <> 0 to indicate error
    };
    di.fprintf_func = &binutilsFprintf;
    di.print_address_func = [](bfd_vma addr, struct disassemble_info* dinfo) {
        dinfo->fprintf_func(dinfo->stream, "%u", addr);
    };
}

void setupMusashi()
{
    m68k_init();

    switch (cpuModel) {

        case M68000:    m68k_set_cpu_type(M68K_CPU_TYPE_68000);     break;
        case M68010:    m68k_set_cpu_type(M68K_CPU_TYPE_68010);     break;
        case M68EC020:  m68k_set_cpu_type(M68K_CPU_TYPE_68EC020);   break;
        case M68020:    m68k_set_cpu_type(M68K_CPU_TYPE_68020);     break;
        case M68EC030:  m68k_set_cpu_type(M68K_CPU_TYPE_68EC030);   break;
        case M68030:    m68k_set_cpu_type(M68K_CPU_TYPE_68030);     break;
        case M68EC040:  m68k_set_cpu_type(M68K_CPU_TYPE_68EC040);   break;
        case M68LC040:  m68k_set_cpu_type(M68K_CPU_TYPE_68LC040);   break;
        case M68040:    m68k_set_cpu_type(M68K_CPU_TYPE_68040);     break;
    }
}

void setupMoira()
{
    moiracpu->setModel(cpuModel);
}

void setupTestEnvironment(Setup &s, long round)
{
    s.round = round;
    s.supervisor = round % 2;
    s.ccr = u8(rand());
    s.ext1 = u16(randomizer.rand());
    s.ext2 = u16(randomizer.rand());
    s.ext3 = u16(randomizer.rand());
    s.vbr = cpuModel == M68000 ? 0 : u16(randomizer.rand() & ~1);
    s.sfc = u16(randomizer.rand());
    s.dfc = u16(randomizer.rand());
    s.cacr = randomizer.rand() & moiracpu->cacrMask();
    s.caar = randomizer.rand();

    for (int i = 0; i < 8; i++) s.d[i] = randomizer.rand();
    for (int i = 0; i < 8; i++) s.a[i] = randomizer.rand();

    for (unsigned i = 0; i < sizeof(s.mem); i++) {
        s.mem[i] = u8(randomizer.rand());
    }
}

void setupTestInstruction(Setup &s, u32 pc, u16 opcode)
{
    static long mvc = 0;

    moira::Instr instr = moiracpu->getInfo(opcode).I;

    // Prepare special extension words for some instructions
    switch (instr) {

        case RTE:

            break;

        case MOVEC:
        {
            if (s.round % 2) {

                s.ext1 &= 0xF000;
                if (mvc % 2) s.ext1 |= 0x800;
                s.ext1 |= (mvc >> 2) & 0xFF;
            }
            break;
        }
        default:
            break;
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
    m68k_set_reg(M68K_REG_CACR, s.cacr);
    m68k_set_reg(M68K_REG_CAAR, s.caar);
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
    moiracpu->setCACR(s.cacr);
    moiracpu->setCAAR(s.caar);
    moiracpu->setSupervisorMode(s.supervisor);
}

clock_t muclk[2] = {0,0}, moclk[2] = {0,0};

void run()
{
    printf("Moira CPU tester. (C) Dirk W. Hoffmann, 2019 - 2023\n\n");
    printf("The test program runs Moira agains Musashi with randomly generated data.\n");
    printf("\n");
    printf("    Test rounds : %ld\n", ROUNDS);
    printf("    Random seed : %d\n", randomizer.init());
    printf("     Exec range : %s\n", TOSTRING(doExec(opcode)));
    printf("     Dasm range : %s\n", TOSTRING(doDasm(opcode)));

    for (long i = 1; i <= ROUNDS; i++) {

        printf("\nRound %ld:\n\n", i);

        while (1) {

            // Select the CPU core
            selectModel(cpuModel);

            // Execute tests
            if constexpr (CHECK_CPU) runCPU(i);
            if constexpr (CHECK_MMU) runMMU(i);
            if constexpr (CHECK_FPU) runFPU(i);

            // Switch the CPU core
            if (cpuModel == M68040) break;
            cpuModel = Model(cpuModel + 1);
        }
    }

    printf("\n");
    printf("All tests completed\n");
}

void runCPU(long round)
{
    Setup setup;

    printf("%s CPU ", selectedModel()); fflush(stdout);

    for (int i = 0; i < 32; i++) {

        printf("."); fflush(stdout);

        setupTestEnvironment(setup, round * 32 + i);

        for (int op = 0x0000; op <= 0xFFFF; op++) {

            if constexpr (SKIP_ILLEGAL) {
                if (moiracpu->getInfo(op).I == moira::ILLEGAL) continue;
            }

            // Execute both CPU cores
            runSingleTest(setup, u16(op));
        }
    }
    passed();
}

void runMMU(long round)
{
    if (!moiracpu->hasMMU()) return;

    Setup setup;

    printf("%s MMU ", selectedModel()); fflush(stdout);
    setupTestEnvironment(setup, round);

    for (int op = 0xF000; op <= 0xF03F; op++) {

        if ((op % 2) == 0) { printf("."); fflush(stdout); }

        for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

            runSingleTest(setup, u16(op), u16(ext));
        }
    }
    passed();
}

void runFPU(long round)
{
    if (!moiracpu->hasFPU()) return;

    Setup setup;

    printf("%s FPU ", selectedModel()); fflush(stdout);
    setupTestEnvironment(setup, round);

    for (int op = 0xF200; op <= 0xF23F; op++) {

        if ((op % 2) == 0) { printf("."); fflush(stdout); }

        for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

            runSingleTest(setup, u16(op), u16(ext));
        }
    }
    passed();
}

void passed()
{
    printf(" PASSED ");
    if (PROFILE_DASM || PROFILE_CPU) {

        clock_t mu = 0, mo = 0;
        if (PROFILE_CPU) { mu += muclk[0]; mo += moclk[0]; }
        if (PROFILE_DASM) { mu += muclk[1]; mo += moclk[1]; }

        printf(" (Moira: %.2fs  Musashi: %.2fs)",
               mo / double(CLOCKS_PER_SEC),
               mu / double(CLOCKS_PER_SEC));
    }
    printf("\n");
}

void runSingleTest(Setup &s, u16 op)
{
    Result mur, mor;

    opcode = op;

    // Prepare the test case with the selected instruction
    setupTestInstruction(s, pc, op);

    // Reset the sandbox (memory accesses observer)
    sandbox.prepare(op);

    // Prepare
    resetMusashi(s);
    resetMoira(s);

    // Run the binutils disassembler
    runBinutils(s, mur);

    // Run Musashi
    runMusashi(s, mur);
    muclk[0] += mur.elapsed[0];
    muclk[1] += mur.elapsed[1];

    // Run Moira
    runMoira(s, mor);
    moclk[0] += mor.elapsed[0];
    moclk[1] += mor.elapsed[1];

    // Compare
    compare(s, mur, mor);
}

void runSingleTest(Setup &s, u16 op, u16 ext)
{
    s.ext1 = ext;
    runSingleTest(s, op);
}

void runBinutils(Setup &s, Result &r)
{
    memcpy(mi.bytes, moiraMem, sizeof(mi.bytes));
    mi.len = 0x10000;

    di.stream = (FILE*)r.dasmBinutils;
    r.dasmBinutils[0] = 0;
    r.dasmCntBinutils = print_insn_m68k(pc, &di);
}

void runMusashi(Setup &s, Result &r)
{
    r.oldpc = m68k_get_reg(NULL, M68K_REG_PC);
    r.opcode = get16(musashiMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;

    // Run the Musashi disassembler
    auto type =
    cpuModel == M68000   ? M68K_CPU_TYPE_68000 :
    cpuModel == M68010   ? M68K_CPU_TYPE_68010 :
    cpuModel == M68EC020 ? M68K_CPU_TYPE_68EC020 :
    cpuModel == M68020   ? M68K_CPU_TYPE_68020 :
    cpuModel == M68EC030 ? M68K_CPU_TYPE_68EC030 :
    cpuModel == M68030   ? M68K_CPU_TYPE_68030 :
    cpuModel == M68EC040 ? M68K_CPU_TYPE_68EC040 :
    cpuModel == M68LC040 ? M68K_CPU_TYPE_68LC040 : M68K_CPU_TYPE_68040;

    clock_t elapsed = clock();
    r.dasmCntMusashi = m68k_disassemble(r.dasmMusashi, s.pc, type);
    r.elapsed[1] = clock() - elapsed;

    if (VERBOSE) {
        printf("$%04x ($%04x): %s (Musashi)\n", r.oldpc, r.opcode, r.dasmMusashi);
    }

    // Run the Musashi CPU
    if (doExec(r.opcode)) {

        clock_t elapsed = clock();
        r.cycles = m68k_execute(1);
        r.elapsed[0] = clock() - elapsed;

        recordMusashiRegisters(r);
    }
}

void runMoira(Setup &s, Result &r)
{
    r.oldpc = moiracpu->getPC();
    r.opcode = get16(moiraMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;

    // Disassemble the instruction
    if (doDasm(r.opcode)) {

        // Disassemble the instruction in Musashi format
        moiracpu->setDasmSyntax(DASM_MUSASHI);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        clock_t elapsed = clock();
        r.dasmCntMusashi = moiracpu->disassemble(r.dasmMusashi, r.oldpc);
        r.elapsed[1] = clock() - elapsed;

        // Disassemble the instruction in GNU format (binutils)
#ifdef MOTOROLA
        moiracpu->setDasmSyntax(DASM_GNU);
#else
        moiracpu->setDasmSyntax(DASM_GNU_MIT);
#endif
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 10, .plainZero = true });
        r.dasmCntBinutils = moiracpu->disassemble(r.dasmBinutils, r.oldpc);
    }

    // Run the Moira CPU
    if (doExec(r.opcode)) {

        u32 pc = moiracpu->getPC();
        u16 op = get16(moiraMem, pc);

        int64_t cycles = moiracpu->getClock();

        if (VERBOSE)
            printf("$%04x ($%04x): %s (Moira)\n", r.oldpc, r.opcode, r.dasmMusashi);

        // Execute instruction
        clock_t elapsed = clock();
        moiracpu->execute();
        r.elapsed[0] = clock() - elapsed;

        // Record the result
        r.cycles = (int)(moiracpu->getClock() - cycles);
        r.oldpc = pc;
        r.opcode = op;
        recordMoiraRegisters(r);
    }
}

bool skip(u16 op)
{
    bool result;

    moira::Instr instr = moiracpu->getInfo(op).I;

    // Skip some instructions that are broken in Musashi
    result =
    instr == moira::ABCD    ||
    instr == moira::SBCD    ||
    instr == moira::NBCD    ||
    instr == moira::BFCHG   ||
    instr == moira::BFCLR   ||
    instr == moira::BFEXTS  ||
    instr == moira::BFEXTU  ||
    instr == moira::BFFFO   ||
    instr == moira::BFINS   ||
    instr == moira::BFSET   ||
    instr == moira::BFTST;

    if (cpuModel != M68000) {

        result |= instr == moira::RTE;
    }

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
    r.cacr = m68k_get_reg(NULL, M68K_REG_CACR);
    r.caar = m68k_get_reg(NULL, M68K_REG_CAAR);
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
    r.cacr = moiracpu->getCACR();
    r.caar = moiracpu->getCAAR();
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
    printf("DFC: %02x  ", s.dfc);
    printf("CACR: %02x ", s.cacr);
    printf("CAAR: %02x\n", s.caar);
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
    printf("CACR: %02x ", r.cacr);
    printf("CAAR: %02x ", r.caar);

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

        if (!skip(s.opcode)) {

            if (!comparePC(r1, r2)) {
                printf("\nPROGRAM COUNTER MISMATCH FOUND");
                error = true;
            }
            if (!compareCycles(r1, r2)) {
                printf("\nCLOCK MISMATCH FOUND");
                error = true;
            }
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
            if (!compareCAxR(r1, r2)) {
                printf("\nCACHE REGISTER MISMATCH FOUND");
                error = true;
            }
            if (sandbox.getErrors()) {
                printf("\nSANDBOX ERRORS REPORTED");
                error = true;
            }
        }
    }

    if (error) {

        printf("\n");
        printf("\nInstruction:  Musashi: [%d] '%s'", r1.dasmCntMusashi, r1.dasmMusashi);
        printf("\n                Moira: [%d] '%s'\n", r2.dasmCntMusashi, r2.dasmMusashi);

        printf("\n             Binutils: [%d] '%s'", r1.dasmCntBinutils, r1.dasmBinutils);
        printf("\n                Moira: [%d] '%s'\n", r2.dasmCntBinutils, r2.dasmBinutils);
        printf("\n");

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
    auto I = moiracpu->getInfo(r1.opcode).I;

    bool skipMusashi = r1.opcode >= 0xF000;
    bool skipBinutils = false;

    switch (cpuModel) {

        case moira::M68010:

            skipBinutils |= I == CMPI;
            break;
            
        case moira::M68EC020:
        case moira::M68EC030:
        case moira::M68EC040:
        case moira::M68LC040:

            skipBinutils |= r1.opcode >= 0xF000;
            break;

        default:
            break;
    }

    // Compare with Musashi
    if (!skipMusashi) {

        if (r1.dasmCntMusashi != r2.dasmCntMusashi) return false;
        if (strcmp(r1.dasmMusashi, r2.dasmMusashi) != 0) return false;
    }

    // Compare with binutils
    if (!skipBinutils) {

        if (I != FTRAPcc) {
            if (r1.dasmCntBinutils != r2.dasmCntBinutils) return false;
        }
        if (strcmp(r1.dasmBinutils, r2.dasmBinutils) != 0) return false;
    }

    return true;
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

bool compareCAxR(Result &r1, Result &r2)
{
    return r1.cacr == r2.cacr && r1.caar == r2.caar;
}

bool compareCycles(Result &r1, Result &r2)
{
    assert(r1.opcode == r2.opcode);
    auto I = moiracpu->getInfo(r1.opcode).I;
    auto S = moiracpu->getInfo(r1.opcode).S;
    auto M = moiracpu->getInfo(r1.opcode).M;

    // Exclude some instructions
    if (I == moira::TAS) return true;

    // WHY DO WE IGNORE THESE?
    if (cpuModel == M68010) {

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
    exit(1);
}
