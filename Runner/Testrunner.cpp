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
#include "m68k_disasm.h"

TestCPU *moiracpu;
Sandbox sandbox;
Randomizer randomizer;
u8 musashiMem[0x10000];
u8 moiraMem[0x10000];
u32 musashiFC = 0;
moira::Model cpuModel = M68000;
long cpuround[16] = { };
long mmuround[16] = { };
long fpuround[16] = { };

// Binutils
char* binutilsBuffer = NULL;
size_t binutilsBufferSize = 0;
FILE* binutilsStream = open_memstream(&binutilsBuffer, &binutilsBufferSize);
meminfo mi;
disassemble_info di;

// M68k disassembler
struct vda68k::DisasmPara_68k dp;
char opcode[16];
char operands[128];
char iwordbuf[32];
char tmpbuf[8];

void selectModel(moira::Model model)
{
    cpuModel = model;

    setupMusashi();
    setupM68k();
    setupBinutils();
    setupMoira();

    printf("\n");
    printf("Emulated CPU: ");

    switch (cpuModel) {

        case M68000:    printf("Motorola 68000");   break;
        case M68010:    printf("Motorola 68010");   break;
        case M68EC020:  printf("Motorola 68EC020"); break;
        case M68020:    printf("Motorola 68020");   break;
        case M68EC030:  printf("Motorola 68EC030"); break;
        case M68030:    printf("Motorola 68030");   break;
        case M68EC040:  printf("Motorola 68EC040"); break;
        case M68LC040:  printf("Motorola 68LC040"); break;
        case M68040:    printf("Motorola 68040"); break;
    }

    printf("\n\n");
    printf("              Exec range: %s\n", TOSTRING(doExec(opcode)));
    printf("              Dasm range: %s\n", TOSTRING(doDasm(opcode)));
    printf("\n");
}

void setupM68k()
{
    memset(&dp,0,sizeof(struct vda68k::DisasmPara_68k));
    dp.opcode = opcode;
    dp.operands = operands;
    dp.radix = 16;
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

    di.stream = binutilsStream;
    di.caller_private = &mi;
    di.memory_error_func = [](int, bfd_vma, struct disassemble_info*) {};
    di.read_memory_func = [](bfd_vma memaddr, bfd_byte* myaddr, unsigned int length, struct disassemble_info* dinfo) -> int {
        meminfo* mi = static_cast<meminfo*>(dinfo->caller_private);
        while (length--)
            *myaddr++ = memaddr < mi->len ? mi->bytes[memaddr++] : 0;
        return 0; // return <> 0 to indicate error
    };
    di.fprintf_func = &fprintf;
    di.print_address_func = [](bfd_vma addr, struct disassemble_info* dinfo) {
        // dinfo->fprintf_func(dinfo->stream, "$%X", addr);
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
    s.vbr = u16(randomizer.rand());
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
    // static u64 mmu = 0;
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

void resetM68k(Setup &s)
{
    dp.instr = (u16 *)&musashiMem[s.pc];
    dp.iaddr = (vda68k::m68k_word *)pc;
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
    printf("Moira CPU tester. (C) Dirk W. Hoffmann, 2019 - 2022\n\n");
    printf("The test program runs Moira agains Musashi with randomly generated data.\n\n");

    printf("    Test rounds: %ld\n", ROUNDS);
    printf("    Random seed: %d\n", randomizer.init());

    selectModel(M68040);

    for (long i = 0; i < ROUNDS;) {

        // Switch the CPU core from time to time
        if (++i % 1000 == 0) selectModel(cpuModel == M68040 ? M68000 : Model(cpuModel + 1));

        // Run a CPU round
        // runCPU(cpuround[cpuModel]++);

        // Continue with a MMU round if applicable
        if constexpr (CHECK_MMU) runMMU(mmuround[cpuModel]++);

        // Continue with a FPU round if applicable
        if constexpr (CHECK_FPU) runFPU(fpuround[cpuModel]++);
    }
}

void runCPU(long round)
{
    Setup setup;

    printf("CPU round %ld ", round); fflush(stdout);
    setupTestEnvironment(setup, round);

    // Initialize the random number generator
    randomizer.prepare(round);

    // Iterate through all opcodes
    for (int opcode = 0x0000; opcode <= 0xFFFF; opcode++) {

        if ((opcode & 0xFFF) == 0) { printf("."); fflush(stdout); }

        if constexpr (SKIP_ILLEGAL) {
            if (moiracpu->getInfo(opcode).I == moira::ILLEGAL) continue;
        }

        // Execute both CPU cores
        runSingleTest(setup, u16(opcode));
    }
    passed();
}

void runMMU(long round)
{
    if (!moiracpu->hasMMU()) return;

    Setup setup;

    printf("MMU round %ld ", round); fflush(stdout);
    setupTestEnvironment(setup, round);

    // Initialize the random number generator
    randomizer.prepare(round);

    for (int op = 0xF000; op <= 0xF03F; op++) {

        if ((op % 4) == 0) { printf("."); fflush(stdout); }

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

    printf("FPU round %ld ", round); fflush(stdout);
    setupTestEnvironment(setup, round);

    // Initialize the random number generator
    randomizer.prepare(round);

    // for (int op = 0xF200; op <= 0xF23F; op++) {
    for (int op = 0xF238; op <= 0xF23F; op++) {

        if ((op % 4) == 0) { printf("."); fflush(stdout); }

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

    // Prepare the test case with the selected instruction
    setupTestInstruction(s, pc, op);

    // Reset the sandbox (memory accesses observer)
    sandbox.prepare(op);

    // Prepare
    resetMusashi(s);
    resetM68k(s);
    resetMoira(s);

    // Run the vda68k disassembler
    runM68k(s, mur);

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

void runM68k(Setup &s, Result &r)
{
    // Run disassembler in Motorola syntax
    auto n1 = M68k_Disassemble(&dp) - dp.instr;
    r.dasmCntMoto = 2 * n1;
    if (strcmp(operands, "") == 0) {
        sprintf(r.dasmMoto, "%s", opcode);
    } else {
        sprintf(r.dasmMoto, "%-7s %s", opcode, operands);
    }

    // Run disassembler in MIT syntax
    auto n2 = M68k_Disassemble(&dp, true) - dp.instr;
    r.dasmCntMIT = 2 * n2;
    if (strcmp(operands, "") == 0) {
        sprintf(r.dasmMIT, "%s", opcode);
    } else {
        sprintf(r.dasmMIT, "%-7s %s", opcode, operands);
    }
}

void runBinutils(Setup &s, Result &r)
{
    memcpy(mi.bytes, moiraMem, sizeof(mi.bytes));
    mi.len = 0x10000;

    // r.dasmCntBinutils = print_insn_m68k(0, &di);
    r.dasmCntBinutils = print_insn_m68k(pc, &di);
    fflush(binutilsStream);
    memcpy(r.dasmBinutils, binutilsBuffer, std::min(binutilsBufferSize, (size_t)128));
    r.dasmBinutils[binutilsBufferSize] = 0;
    rewind(binutilsStream);

    // unsigned char * p = moiraMem + pc;
    // printf("%02x%02x %02x%02x %02x%02x [%d] : %s\n", p[0], p[1], p[2], p[3], p[4], p[5], r.dasmCntBinutils, r.dasmBinutils);
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
        moiracpu->setDasmStyle(DASM_MUSASHI);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        clock_t elapsed = clock();
        r.dasmCntMusashi = moiracpu->disassemble(r.oldpc, r.dasmMusashi);
        r.elapsed[1] = clock() - elapsed;

        // Disassemble the instruction in GNU format (binutils)
        moiracpu->setDasmStyle(DASM_GNU);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 10, .plainZero = true });
        r.dasmCntBinutils = moiracpu->disassemble(r.oldpc, r.dasmBinutils);

        // Disassemble the instruction in Vda68k Motorola format
        moiracpu->setDasmStyle(DASM_VDA68K_MOT);
        moiracpu->setDasmNumberFormat({ .prefix = "0x", .radix = 16, .plainZero = true });
        r.dasmCntMoto = moiracpu->disassemble(r.oldpc, r.dasmMoto);

        // Disassemble the instruction in Vda68k MIT format
        moiracpu->setDasmStyle(DASM_VDA68K_MIT);
        moiracpu->setDasmNumberFormat({ .prefix = "0x", .radix = 16, .plainZero = true });
        r.dasmCntMIT = moiracpu->disassemble(r.oldpc, r.dasmMIT);
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

    /*
    if (s.opcode >= 0xF000) {
        printf("%x: %s\n", s.opcode, r1.dasmMusashi);
    }
    */
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
        printf("\nInstruction: [%d] %-40s (Musashi)", r1.dasmCntMusashi, r1.dasmMusashi);
        printf("\n             [%d] %-40s (Moira)\n", r2.dasmCntMusashi, r2.dasmMusashi);

        printf("\n             [%d] %-40s (Binutils)", r1.dasmCntBinutils, r1.dasmBinutils);
        printf("\n             [%d] %-40s (Moira)\n", r2.dasmCntBinutils, r2.dasmBinutils);

        printf("\n             [%d] %-40s (Vda68k, Motorola)", r1.dasmCntMoto, r1.dasmMoto);
        printf("\n             [%d] %-40s (Moira)\n", r2.dasmCntMoto, r2.dasmMoto);

        printf("\n             [%d] %-40s (Vda68k, MIT)", r1.dasmCntMIT, r1.dasmMIT);
        printf("\n             [%d] %-40s (Moira)\n\n", r2.dasmCntMIT, r2.dasmMIT);

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
    bool skipMusashi = false;
    bool skipBinutils = false;
    bool skipM68k = false;

    auto I = moiracpu->getInfo(r1.opcode).I;

    // Skip both Musashi and M68k in the Line A range
    skipMusashi |= r1.opcode >= 0xF000;
    skipM68k |= r1.opcode >= 0xF000;

    // Skip broken Musashi opcodes
    skipMusashi |= (((r1.opcode & 0xff20) == 0xf400) || // cinv
                    ((r1.opcode & 0xff20) == 0xf420) || // cpush
                    ((r1.opcode & 0xfff8) == 0xf620) || // move16
                    ((r1.opcode & 0xfff8) == 0xf600) || // move16
                    ((r1.opcode & 0xfff8) == 0xf608) || // move16
                    ((r1.opcode & 0xfff8) == 0xf610) || // move16
                    ((r1.opcode & 0xfff8) == 0xf618) ); // move16

    // Skip broken Binutils opcodes
    skipBinutils |= I == CMPI && cpuModel == M68010;

    // M68k seems to be buggy in the line F range
    skipM68k = r1.opcode >= 0xF000 || I == ILLEGAL || I == LINE_F;

    switch (cpuModel) {

        case moira::M68000:
        case moira::M68010:

            skipMusashi |= ((r1.opcode & 0xf800) == 0xf000);
            break;

        case moira::M68EC020:
        case moira::M68020:
        case moira::M68EC030:

            skipMusashi |= (((r1.opcode & 0xfe00) == 0xf000) || // MMU
                            ((r1.opcode & 0xfe00) == 0xf200) ); // FPU
            break;

        case moira::M68030:

            skipMusashi |= (((r1.opcode & 0xfe00) == 0xf200) ); // FPU
            break;

        case moira::M68EC040:

            skipMusashi |= (((r1.opcode & 0xfe00) == 0xf000) || // MMU
                            ((r1.opcode & 0xfe00) == 0xf200) ); // FPU
            break;

        case moira::M68LC040:

            skipMusashi |= (((r1.opcode & 0xfe00) == 0xf200) ); // FPU
            break;

        case moira::M68040:

            break;

        default:
            break;
    }

    bool is040 = (cpuModel == moira::M68040   ||
                  cpuModel == moira::M68EC040 ||
                  cpuModel == moira::M68LC040 );

    if (is040) {

        // Disable CPI
        skipMusashi |= (((r1.opcode & 0xf1c0) == 0xf000) || // cpGen
                        ((r1.opcode & 0xf1f8) == 0xf048) || // cpDbcc
                        ((r1.opcode & 0xf1c0) == 0xf040) || // cpScc
                        ((r1.opcode & 0xf1c0) == 0xf080) || // cpBcc16
                        ((r1.opcode & 0xf1c0) == 0xf0c0) || // cpBcc32
                        ((r1.opcode & 0xf1f8) == 0xf078) || // cpTrap
                        ((r1.opcode & 0xf1c0) == 0xf140) || // cpRestore
                        ((r1.opcode & 0xf1c0) == 0xf100) ); // cpSave

        // Enable special 68040 instructions
        skipMusashi &= !(((r1.opcode & 0xff20) == 0xf400) || // cinv
                         ((r1.opcode & 0xff20) == 0xf420) || // cpush
                         ((r1.opcode & 0xfff8) == 0xf620) || // move16
                         ((r1.opcode & 0xfff8) == 0xf600) || // move16
                         ((r1.opcode & 0xfff8) == 0xf608) || // move16
                         ((r1.opcode & 0xfff8) == 0xf610) || // move16
                         ((r1.opcode & 0xfff8) == 0xf618) ); // move16
    }

    // Compare with Musashi
    if (!skipMusashi) {

        if (r1.dasmCntMusashi != r2.dasmCntMusashi) return false;
        if (strcmp(r1.dasmMusashi, r2.dasmMusashi) != 0) return false;
    }

    // Compare with binutils
    if (!skipBinutils) {

        if (r1.dasmCntBinutils != r2.dasmCntBinutils) return false;
        if (strcmp(r1.dasmBinutils, r2.dasmBinutils) != 0) return false;
    }

    // DEPRECATED
    if (!skipM68k) {

        // Compare with M68k (Motorola syntax)
        if (r1.dasmCntMoto != r2.dasmCntMoto) return false;
        if (strcmp(r1.dasmMoto, r2.dasmMoto) != 0) return false;

        // Compare with M68k (MIT syntax)
        if (r1.dasmCntMIT != r2.dasmCntMIT) return false;
        if (strcmp(r1.dasmMIT, r2.dasmMIT) != 0) return false;
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
    exit(0);
}
