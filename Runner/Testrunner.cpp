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
#include <ctime>
#include <string>
#ifdef HAVE_AMIBERRY
#include "AmiberryGlue/AmiberryCpu.h"
#endif

TestCPU *moiracpu;
Sandbox sandbox;
Randomizer randomizer;
u8 musashiMem[0x10000];
u8 moiraMem[0x10000];
u32 musashiFC = 0;
moira::Model cpuModel = Model::M68000;
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

#ifdef HAVE_AMIBERRY
static bool amiberryRef;
static void setupAmiberry();
#else
static const bool amiberryRef = false;
#endif

void selectModel(moira::Model model)
{
    cpuModel = model;

    setupMusashi();
#ifdef HAVE_AMIBERRY
    if (amiberryRef) setupAmiberry();
#endif
    sandbox.checkCycles = !amiberryRef || cpuModel == Model::M68000;
    sandbox.addrMask = (cpuModel == Model::M68EC020) ? 0xFFFFFF : 0xFFFFFFFF;
    sandbox.byteGranular = amiberryRef && cpuModel != Model::M68000 && cpuModel != Model::M68010;
    setupBinutils();
    setupMoira();
}

const char *selectedModel()
{
    switch (cpuModel) {

        case Model::M68000:    return "68000";
        case Model::M68010:    return "68010";
        case Model::M68EC020:  return "EC020";
        case Model::M68020:    return "68020";
        case Model::M68EC030:  return "EC030";
        case Model::M68030:    return "68030";
        case Model::M68EC040:  return "EC040";
        case Model::M68LC040:  return "LC040";
        case Model::M68040:    return "68040";

        default:
            return "???";
    }
}

void setupBinutils()
{
    switch (cpuModel) {

        case Model::M68000:    di.mach = MACH_68000; break;
        case Model::M68010:    di.mach = MACH_68010; break;
        case Model::M68EC020:  // Not supported by binutils
        case Model::M68020:    di.mach = MACH_68020; break;
        case Model::M68EC030:  // Not supported by binutils
        case Model::M68030:    di.mach = MACH_68030; break;
        case Model::M68EC040:  // Not supported by binutils
        case Model::M68LC040:  // Not supported by binutils
        case Model::M68040:    di.mach = MACH_68040; break;
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

//
// Amiberry as reference (optional, select with REFERENCE=amiberry)
//

#ifdef HAVE_AMIBERRY

static bool amiberryRefInit = (amiberryRef = MOIRA_MIMIC_AMIBERRY || (getenv("REFERENCE") && std::string(getenv("REFERENCE")) == "amiberry"));
static amicpu::State amiState;

static void amiberryWrite(int size, u32 addr, u32 value, u32 fc, u64 cycle)
{
    if (CHECK_MEM_WRITES)
        sandbox.record(size == 1 ? POKE8 : POKE16, addr, cycle, fc, size == 1 ? u8(value) : u16(value));
}

static void setupAmiberry()
{
    using amicpu::Cpu;
    Cpu model =
    cpuModel == Model::M68000   ? Cpu::M68000 :
    cpuModel == Model::M68010   ? Cpu::M68010 :
    cpuModel == Model::M68EC020 ? Cpu::M68EC020 :
    cpuModel == Model::M68020   ? Cpu::M68020 :
    cpuModel == Model::M68EC030 ? Cpu::M68EC030 :
    cpuModel == Model::M68030   ? Cpu::M68030 :
    cpuModel == Model::M68EC040 ? Cpu::M68EC040 :
    cpuModel == Model::M68LC040 ? Cpu::M68LC040 : Cpu::M68040;

    amicpu::init(model, musashiMem, amiberryWrite);
}

static void runAmiberry(Setup &s, Result &r)
{
    amicpu::State st;

    for (int i = 0; i < 8; i++) st.d[i] = s.d[i];
    for (int i = 0; i < 7; i++) st.a[i] = s.a[i];
    st.usp = 0;
    st.isp = 0x2000;
    st.msp = 0;
    st.pc = s.pc;
    st.sr = (s.supervisor ? 0x2000 : 0) | 0x0700 | s.ccr;
    st.vbr = s.vbr;
    st.sfc = s.sfc;
    st.dfc = s.dfc;
    st.cacr = s.cacr;
    st.caar = s.caar;
    for (int i = 0; i < 8; i++) {
        st.fpse[i] = s.fpr[i].signExp();
        st.fpman[i] = s.fpr[i].mantissa();
    }
    st.fpcr = s.fpcr;
    st.fpsr = s.fpsr;
    st.fpiar = s.fpiar;

    amicpu::setState(st);
    clock_t elapsed = clock();
    amicpu::step();
    r.elapsed[0] = clock() - elapsed;
    r.cycles = int(amicpu::cycles());

    amiState = amicpu::getState();
    r.pc = amiState.pc;
    r.sr = amiState.sr;
    r.usp = amiState.usp;
    r.isp = amiState.isp;
    r.msp = amiState.msp;
    r.fc = 0;
    r.vbr = amiState.vbr;
    r.sfc = amiState.sfc & 7;
    r.dfc = amiState.dfc & 7;
    r.cacr = amiState.cacr;
    r.caar = amiState.caar;
    for (int i = 0; i < 8; i++) r.d[i] = amiState.d[i];
    for (int i = 0; i < 7; i++) r.a[i] = amiState.a[i];
    r.a[7] = amiState.a7;
}

#endif

void setupMusashi()
{
    m68k_init();

    switch (cpuModel) {

        case Model::M68000:    m68k_set_cpu_type(M68K_CPU_TYPE_68000);     break;
        case Model::M68010:    m68k_set_cpu_type(M68K_CPU_TYPE_68010);     break;
        case Model::M68EC020:  m68k_set_cpu_type(M68K_CPU_TYPE_68EC020);   break;
        case Model::M68020:    m68k_set_cpu_type(M68K_CPU_TYPE_68020);     break;
        case Model::M68EC030:  m68k_set_cpu_type(M68K_CPU_TYPE_68EC030);   break;
        case Model::M68030:    m68k_set_cpu_type(M68K_CPU_TYPE_68030);     break;
        case Model::M68EC040:  m68k_set_cpu_type(M68K_CPU_TYPE_68EC040);   break;
        case Model::M68LC040:  m68k_set_cpu_type(M68K_CPU_TYPE_68LC040);   break;
        case Model::M68040:    m68k_set_cpu_type(M68K_CPU_TYPE_68040);     break;
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
    s.vbr = cpuModel == Model::M68000 ? 0 : u16(randomizer.rand() & ~1);
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

    moira::Instr instr = moiracpu->getInstrInfo(opcode).I;

    // Prepare special extension words for some instructions
    switch (instr) {

        case Instr::RTE:

            break;

        case Instr::MOVEC:
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

void resetMusashiFpu(Setup &s)
{
    for (int i = 0; i < 8; i++) {
        REG_FP[i].high = s.fpr[i].signExp();
        REG_FP[i].low = s.fpr[i].mantissa();
    }
    REG_FPCR = s.fpcr;
    REG_FPSR = s.fpsr;
    REG_FPIAR = s.fpiar;
}

void resetMoiraFpu(Setup &s)
{
    for (int i = 0; i < 8; i++) moiracpu->fpu.fpr[i] = s.fpr[i];
    moiracpu->fpu.setFPCR(s.fpcr);
    moiracpu->fpu.setFPSR(s.fpsr);
    moiracpu->fpu.setFPIAR(s.fpiar);
}

void recordMusashiFpu(Result &r)
{
#ifdef HAVE_AMIBERRY
    if (amiberryRef) {
        for (int i = 0; i < 8; i++) r.fpr[i] = Float80::fromBits(amiState.fpse[i], amiState.fpman[i]);
        r.fpcr = amiState.fpcr;
        r.fpsr = amiState.fpsr;
        r.fpiar = amiState.fpiar;
        return;
    }
#endif
    for (int i = 0; i < 8; i++) {
        r.fpr[i] = Float80::fromBits(REG_FP[i].high, REG_FP[i].low);
    }
    r.fpcr = REG_FPCR;
    r.fpsr = REG_FPSR;
    r.fpiar = REG_FPIAR;
}

void recordMoiraFpu(Result &r)
{
    for (int i = 0; i < 8; i++) r.fpr[i] = moiracpu->fpu.fpr[i];
    r.fpcr = moiracpu->fpu.getFPCR();
    r.fpsr = moiracpu->fpu.getFPSR();
    r.fpiar = moiracpu->fpu.getFPIAR();
}

clock_t muclk[2] = {0,0}, moclk[2] = {0,0};

void run()
{
    std::time_t t = std::time(0);
    std::tm* now = std::localtime(&t);

    run(1900 + now->tm_year + now->tm_yday);
}

void run(unsigned seed)
{
    printf("Moira CPU tester. (C) Dirk W. Hoffmann, 2019 - 2026\n");
    printf("Core %s\n\n", Moira::build().c_str());
    printf("The test program runs Moira agains Musashi with randomly generated data.\n");
    printf("\n");
    printf("    Test rounds : %ld\n", ROUNDS);
    printf("    Random seed : %d\n", seed);
    printf("     Exec range : %s\n", TOSTRING(doExec(opcode)));
    printf("     Dasm range : %s\n", TOSTRING(doDasm(opcode)));
    printf("      Reference : %s\n", amiberryRef ? "Amiberry" : "Musashi");

    srand(seed);
    
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
            if (cpuModel == Model::M68040) break;
            cpuModel = Model(int(cpuModel) + 1);
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
                if (moiracpu->getInstrInfo(op).I == moira::Instr::ILLEGAL) continue;
            }

            // FPU opcodes are covered by runFPU
            if (isFpuOpcode(u16(op))) continue;

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

    for (int i = 0; i < FPU_SETUPS; i++) {

        printf("."); fflush(stdout);

        setupTestEnvironment(setup, round * FPU_SETUPS + i);
        setupFpuEnvironment(setup);

        for (int op = 0xF200; op <= 0xF23F; op++) {

            for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

                runSingleTest(setup, u16(op), u16(ext));
            }
        }
    }
    passed();
}

bool isFpuOpcode(u16 op)
{
    return (op & 0xFF00) == 0xF200 && moiracpu != nullptr && moiracpu->hasFPU();
}

void setupFpuEnvironment(Setup &s)
{
    // A pool of interesting values
    static const Float80 pool[] = {

        Float80::zero(), Float80::zero(true),
        Float80::one(), Float80::one(true),
        Float80::infinity(), Float80::infinity(true),
        Float80::quietNaN(), Float80::signalingNaN(),
        Float80::fromBits(0x4000, 0x8000000000000000ULL),   // 2.0
        Float80::fromBits(0x3FFE, 0x8000000000000000ULL),   // 0.5
        Float80::fromBits(0x4005, 0xA000000000000000ULL),   // 40.0
        Float80::fromBits(0xC005, 0xA000000000000000ULL),   // -40.0
        Float80::fromBits(0x0001, 0x8000000000000000ULL),   // Smallest normal
        Float80::fromBits(0x0000, 0x4000000000000000ULL),   // Denormal
        Float80::fromBits(0x7FFE, 0xFFFFFFFFFFFFFFFFULL),   // Largest finite
        Float80::fromBits(0x3FFF, 0x8000000000000001ULL),   // 1 + epsilon
    };
    const int poolSize = sizeof(pool) / sizeof(pool[0]);

    for (int i = 0; i < 8; i++) {

        u32 r = randomizer.rand();
        if (r % 3 == 0) {

            // Pick a special value
            s.fpr[i] = pool[(r >> 8) % poolSize];

        } else {

            // Create a random normalized number with a moderate exponent
            u64 man = (u64(randomizer.rand()) << 32 | randomizer.rand()) | (1ULL << 63);
            u16 exp = u16(0x3FFF - 40 + randomizer.rand() % 80);
            u16 sign = (randomizer.rand() & 1) << 15;
            s.fpr[i] = Float80::fromBits(sign | exp, man);
        }
    }

    // Control register: Random rounding mode and precision, no exception enables
    s.fpcr = randomizer.rand() & 0xF0;
    s.fpsr = randomizer.rand() & 0x0F000000;
    s.fpiar = randomizer.rand();
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

    bool fpu = isFpuOpcode(op);
    if (fpu) {
        resetMusashiFpu(s);
        resetMoiraFpu(s);
    }

    // Run the binutils disassembler
    runBinutils(s, mur);

    // Run the reference CPU
    runMusashi(s, mur);
#ifdef HAVE_AMIBERRY
    // Moira is configured without address error emulation (MOIRA_EMULATE_ADDRESS_ERROR)
    if (amiberryRef && (amicpu::lastException() == 3) && !MOIRA_EMULATE_ADDRESS_ERROR) return;
#endif
    muclk[0] += mur.elapsed[0];
    muclk[1] += mur.elapsed[1];

    // Run Moira
    runMoira(s, mor);
    moclk[0] += mor.elapsed[0];
    moclk[1] += mor.elapsed[1];

    // Compare
    if (fpu) {
        recordMusashiFpu(mur);
        recordMoiraFpu(mor);
    }
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
    cpuModel == Model::M68000   ? M68K_CPU_TYPE_68000 :
    cpuModel == Model::M68010   ? M68K_CPU_TYPE_68010 :
    cpuModel == Model::M68EC020 ? M68K_CPU_TYPE_68EC020 :
    cpuModel == Model::M68020   ? M68K_CPU_TYPE_68020 :
    cpuModel == Model::M68EC030 ? M68K_CPU_TYPE_68EC030 :
    cpuModel == Model::M68030   ? M68K_CPU_TYPE_68030 :
    cpuModel == Model::M68EC040 ? M68K_CPU_TYPE_68EC040 :
    cpuModel == Model::M68LC040 ? M68K_CPU_TYPE_68LC040 : M68K_CPU_TYPE_68040;

    clock_t elapsed = clock();
    r.dasmCntMusashi = m68k_disassemble(r.dasmMusashi, s.pc, type);
    r.elapsed[1] = clock() - elapsed;

    if (VERBOSE) {
        printf("$%04x ($%04x): %s (Musashi)\n", r.oldpc, r.opcode, r.dasmMusashi);
    }

    // Run the reference CPU
    if (doExec(r.opcode)) {

#ifdef HAVE_AMIBERRY
        if (amiberryRef) {
            runAmiberry(s, r);
            return;
        }
#endif
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
        moiracpu->setDasmSyntax(Syntax::MUSASHI);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        clock_t elapsed = clock();
        r.dasmCntMusashi = moiracpu->disassemble(r.dasmMusashi, r.oldpc);
        r.elapsed[1] = clock() - elapsed;

        // Disassemble the instruction in GNU format (binutils)
#ifdef MOTOROLA
        moiracpu->setDasmSyntax(Syntax::GNU);
#else
        moiracpu->setDasmSyntax(Syntax::GNU_MIT);
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

bool skipAccessCheck(u16 op)
{
    // Amiberry only writes back the bytes of a bit field that are affected,
    // whereas Moira writes back the whole long word. The final memory
    // contents are still compared.
    if (!amiberryRef) return false;
    if ((op & 0xF8C0) == 0xE8C0) return true;

    // FPU opcodes may be legal for one side only (final memory contents are still compared)
    if ((op & 0xFE00) == 0xF200) return true;

    // N and V are undefined for CHK2, which shows up in the exception frame
    auto instr = moiracpu->getInstrInfo(op).I;
    return instr == Instr::CHK2 || instr == Instr::CMP2;
}

bool skip(u16 op)
{
    bool result;

    moira::Instr instr = moiracpu->getInstrInfo(op).I;

    if (amiberryRef) {

        // Amiberry doesn't implement the module instructions
        if (instr == Instr::CALLM || instr == Instr::RTM) return true;

        // RTE processes random stack frames (format errors, bus error frames)
        if (instr == Instr::RTE && cpuModel != Model::M68000 && cpuModel != Model::M68010) return true;

        // LINK.l A7 pushes A7-4 on the 68040 (Moira's 040 models run on the 68020 core)
        if (instr == Instr::LINK && (op & 7) == 7 && cpuModel >= Model::M68EC040) return true;

        // Zero divide uses a different stack frame on the 68040 (Moira's 040 models run on the 68020 core)
        if ((instr == Instr::DIVL || instr == Instr::DIVU || instr == Instr::DIVS) &&
            cpuModel >= Model::M68EC040) return true;

        // Amiberry emulates FPU instructions that the 68040 traps (Moira follows real hardware)
        if ((op & 0xFFC0) == 0xF200 && cpuModel == Model::M68040) {
            u16 e = get16(musashiMem, 0x1002);
            u16 cod = e >> 13, cmd = e & 0x7F;
            if ((cod == 0 || cod == 2) && (cmd == 0x02 || cmd == 0x06 || cmd == 0x08 || cmd == 0x09 ||
                (cmd >= 0x0A && cmd <= 0x12 && cmd != 0x0B && cmd != 0x13) ||
                cmd == 0x14 || cmd == 0x15 || cmd == 0x16 || cmd == 0x19 ||
                (cmd >= 0x1C && cmd <= 0x1F) || cmd == 0x21 || cmd == 0x25 || cmd == 0x26 ||
                (cmd >= 0x30 && cmd <= 0x37))) return true;
        }

        // FMOVECR from an undefined ROM offset returns implementation specific values
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(musashiMem, 0x1002);
            u16 off = e & 0x7F;
            if ((e & 0xFC00) == 0x5C00 && off != 0x00 && !(off >= 0x0B && off <= 0x0F) && !(off >= 0x30 && off <= 0x3F)) return true;
        }

        // FMOVE to <ea> with a non-zero k-factor field is only defined for packed formats
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(musashiMem, 0x1002);
            u16 fmt = (e >> 10) & 7;
            if ((e >> 13) == 3 && (e & 0x7F) && fmt != 3 && fmt != 7) return true;
        }

        // FMOVEM to/from control registers with an empty register list or non-zero reserved bits is undefined
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(musashiMem, 0x1002);
            if (((e >> 13) == 4 || (e >> 13) == 5) && ((e & 0x1C00) == 0 || (e & 0x3FF))) return true;
        }

        // FMOVEM <ea>,<control registers> with several registers and Dn/An as source. Moira
        // accepts this, Amiberry (and the 6888x manual) don't. Flagged, not mimicked.
        if ((op & 0xFFC0) == 0xF200 && ((op >> 3) & 7) <= 1) {
            u16 e = get16(musashiMem, 0x1002);
            u16 lst = (e >> 10) & 7;
            if ((e >> 13) == 4 && lst != 1 && lst != 2 && lst != 4) return true;
            if (((e >> 13) == 4 || (e >> 13) == 5) && ((op >> 3) & 7) == 1 && lst != 1) return true;
        }

        // Register-to-register FPU operations and FMOVECR with a non-zero <ea> field: Moira raises line F, Amiberry ignores the field
        if ((op & 0xFFC0) == 0xF200 && (op & 0x3F) && ((get16(musashiMem, 0x1002) >> 13) == 0 ||
            (get16(musashiMem, 0x1002) & 0xFC00) == 0x5C00)) return true;

        // Rounded results of the 68040 FS/FD instructions differ on underflow (under investigation)
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(musashiMem, 0x1002);
            if (((e >> 13) == 0 || (e >> 13) == 2) && (e & 0x40)) return true;
        }

        // Packed decimal: random input contains invalid digits and the digit rounding on output differs
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(musashiMem, 0x1002);
            if (((e >> 13) == 2 || (e >> 13) == 3) && (((e >> 10) & 7) == 3 || ((e >> 10) & 7) == 7)) return true;
        }

        // FMOVEM data registers with an empty static list or reserved mode bits
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(musashiMem, 0x1002);
            if (((e >> 13) == 6 || (e >> 13) == 7) && !(e & 0x0800) && (e & 0xFF) == 0) return true;
            if (((e >> 13) == 6 || (e >> 13) == 7) && (((e >> 11) & 3) < 2 || (e & 0x0700) || (((e >> 11) & 3) == 3 && (e & 0x8F)))) return true;
        }

        // MULL with identical high and low registers has an undefined result
        if (instr == Instr::MULL) {
            u16 e = get16(musashiMem, 0x1002);
            if ((e & 0x400) && ((e >> 12) & 7) == (e & 7)) return true;
        }

        // CAS2 with identical compare registers is degenerate
        if (instr == Instr::CAS2) {
            u16 e1 = get16(musashiMem, 0x1002), e2 = get16(musashiMem, 0x1004);
            if ((e1 & 7) == (e2 & 7) || ((e1 >> 6) & 7) == ((e2 >> 6) & 7)) return true;
        }

        /* The behavior is undefined for reserved encodings of the full
         * extension word (indexed modes of the 68020 and above). Skip the
         * instruction if one of its first words looks like one.
         */
        int mode = (op >> 3) & 7, reg = op & 7;
        int dmode = (op >> 6) & 7, dreg = (op >> 9) & 7;
        bool indexed = mode == 6 || (mode == 7 && reg == 3);
        if (op >= 0x1000 && op < 0x4000) indexed |= dmode == 6 || (dmode == 7 && dreg == 3);

        if (cpuModel >= Model::M68EC020 && indexed) {
            for (int off = 2; off <= 6; off += 2) {
                u16 w = get16(musashiMem, 0x1000 + off);
                if ((w & 0x100) == 0) continue;
                bool bs = (w & 0x30) == 0 || (w & 0x08) || (w & 7) == 4 || ((w & 0x40) && (w & 7) >= 5);
                if (bs) return true;
            }
        }
        return false;
    }

    // Skip some instructions that are broken in Musashi
    result =
    instr == Instr::ABCD    ||
    instr == Instr::SBCD    ||
    instr == Instr::NBCD    ||
    instr == Instr::BFCHG   ||
    instr == Instr::BFCLR   ||
    instr == Instr::BFEXTS  ||
    instr == Instr::BFEXTU  ||
    instr == Instr::BFFFO   ||
    instr == Instr::BFINS   ||
    instr == Instr::BFSET   ||
    instr == Instr::BFTST;

    if (cpuModel != Model::M68000) {

        result |= instr == Instr::RTE;
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

        // FPU opcodes where only one side raises line F (reserved bits set in the opcode or extension word)
        bool legalityMismatch = amiberryRef && (s.opcode & 0xFE00) == 0xF200 &&
                                ((r1.pc >> 12) == 1) != ((r2.pc >> 12) == 1);

        if (!legalityMismatch && !skip(s.opcode)) {

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
            if (isFpuOpcode(s.opcode)) {

                if (!compareFPR(r1, r2)) {
                    printf("\nFLOATING-POINT REGISTER MISMATCH FOUND");
                    error = true;
                }
                if (!compareFPCR(r1, r2)) {
                    printf("\nFPCR MISMATCH FOUND");
                    error = true;
                }
                if (!compareFPSR(r1, r2)) {
                    printf("\nFPSR MISMATCH FOUND");
                    error = true;
                }
                if (!compareFPIAR(r1, r2)) {
                    printf("\nFPIAR MISMATCH FOUND");
                    error = true;
                }
                if (!compareMemory()) {
                    printf("\nMEMORY MISMATCH FOUND");
                    error = true;
                }
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

        if (isFpuOpcode(s.opcode)) {
            for (int i = 0; i < 8; i++) {
                printf("FP%d: setup %s  Musashi %s  Moira %s\n", i,
                       s.fpr[i].str().c_str(), r1.fpr[i].str().c_str(), r2.fpr[i].str().c_str());
            }
            printf("FPCR: %08x / %08x / %08x (setup / Musashi / Moira)\n", s.fpcr, r1.fpcr, r2.fpcr);
            printf("FPSR: %08x / %08x / %08x\n", s.fpsr, r1.fpsr, r2.fpsr);
            printf("FPIAR: %08x / %08x / %08x\n\n", s.fpiar, r1.fpiar, r2.fpiar);
        }

        bugReport();
    }
}

bool compareDasm(Result &r1, Result &r2)
{
    auto I = moiracpu->getInstrInfo(r1.opcode).I;

    bool skipMusashi = r1.opcode >= 0xF000;
    bool skipBinutils = false;

    switch (cpuModel) {

        case Model::M68010:

            skipBinutils |= I == Instr::CMPI;
            break;
            
        case Model::M68EC020:
        case Model::M68EC030:
        case Model::M68EC040:
        case Model::M68LC040:

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

        if (I != Instr::FTRAPcc) {
            if (r1.dasmCntBinutils != r2.dasmCntBinutils) return false;
        }
        if (strcmp(r1.dasmBinutils, r2.dasmBinutils) != 0) return false;
    }

    return true;
}

bool compareD(Result &r1, Result &r2)
{
    assert(r1.opcode == r2.opcode);
    moira::Instr instr = moiracpu->getInstrInfo(r1.opcode).I;
    if (instr == Instr::DIVU || instr == Instr::DIVS)
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
    // Amiberry clears the lowest PC bit after a jump to an odd address. Moira
    // is configured without address error emulation and keeps it.
    if (amiberryRef && (r2.pc & 1) && r1.pc == (r2.pc & ~1u)) return true;

    return r1.pc == r2.pc;
}

bool compareSR(Result &r1, Result &r2)
{
    assert(r1.opcode == r2.opcode);
    moira::Instr instr = moiracpu->getInstrInfo(r1.opcode).I;
    if (instr == Instr::DIVU || instr == Instr::DIVS) {

        // Musashi differs (and is likely wrong). Ignore it
        return true;
    }

    // N and V are undefined for CMP2 and CHK2 and not compared with Amiberry
    if (amiberryRef && (instr == Instr::CMP2 || instr == Instr::CHK2)) {
        return (r1.sr & ~0x0A) == (r2.sr & ~0x0A);
    }

    // N and V are undefined for BCD instructions (68040 differs from older models)
    if (amiberryRef && cpuModel >= Model::M68EC040 &&
        (instr == Instr::ABCD || instr == Instr::SBCD || instr == Instr::NBCD)) {
        return (r1.sr & ~0x0A) == (r2.sr & ~0x0A);
    }

    // Z, V and C are undefined for CHK on the 68020 and above
    if (amiberryRef && instr == Instr::CHK && cpuModel != Model::M68000 && cpuModel != Model::M68010) {
        return (r1.sr & ~0x07) == (r2.sr & ~0x07);
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

bool compareFPR(Result &r1, Result &r2)
{
    for (int i = 0; i < 8; i++) if (!r1.fpr[i].identical(r2.fpr[i])) return false;
    return true;
}

bool compareFPCR(Result &r1, Result &r2)
{
    // Amiberry stores the reserved low bits of FPCR
    if (amiberryRef) return (r1.fpcr & ~0xF) == (r2.fpcr & ~0xF);

    return r1.fpcr == r2.fpcr;
}
bool compareFPSR(Result &r1, Result &r2)
{
    // Amiberry modifies FPSR even if the instruction ends up in a line F exception
    if (amiberryRef && (r1.pc >> 12) != 1) return true;

    // Amiberry doesn't raise OPERR when it converts a signaling NaN to an integer
    if (amiberryRef && (r1.fpsr & 0x4000)) return (r1.fpsr | 0x2000) == (r2.fpsr | 0x2000);

    return r1.fpsr == r2.fpsr;
}
bool compareFPIAR(Result &r1, Result &r2)
{
    // Amiberry updates FPIAR even if the instruction ends up in a line F exception
    if (amiberryRef && (r1.pc >> 12) != 1) return true;

    // Amiberry skips the FPIAR update if a conversion in FMOVE <ea> raises an operand error
    if (amiberryRef && (r1.fpsr & 0x6000)) return true;

    return r1.fpiar == r2.fpiar;
}
bool compareMemory() { return memcmp(musashiMem, moiraMem, sizeof(musashiMem)) == 0; }

bool compareCycles(Result &r1, Result &r2)
{
    assert(r1.opcode == r2.opcode);
    auto I = moiracpu->getInstrInfo(r1.opcode).I;
    auto S = moiracpu->getInstrInfo(r1.opcode).S;
    auto M = moiracpu->getInstrInfo(r1.opcode).M;

    // Exclude some instructions
    if (I == Instr::TAS) return true;

    // FPU timing is not emulated yet
    if (isFpuOpcode(r1.opcode)) return true;

    /* Amiberry's cores are only used as a timing reference for the 68000. The
     * 68010 timing is partially copied from the 68000, and the 68020+ cores
     * aren't cycle-exact.
     */
    if (amiberryRef && cpuModel != Model::M68000) return true;

    /* Timing comparison is off for every model beyond the 68000 and 68010
     * while the instruction cache is emulated. Those models (68020 and up)
     * all execute on Moira's C68020 core (see createJumpTable() in
     * MoiraInit_cpp.h), which routes instruction fetches through the cache.
     * Musashi does not model the 68020's instruction cache, so its cycle
     * counts assume every fetch goes straight to the bus. Moira's
     * cache-aware fetch timing (tuned against WinUAE/Amiberry, not Musashi)
     * legitimately diverges from that flat model.
     */
    if constexpr (MOIRA_EMULATE_ICACHE) {
        if (cpuModel != Model::M68000 && cpuModel != Model::M68010) return true;
    }

    // WHY DO WE IGNORE THESE?
    if (cpuModel == Model::M68010) {

        if (I == Instr::CLR && S == Byte && M == Mode::AL) return true;
        if (I == Instr::CLR && S == Word && M == Mode::AL) return true;
        if (I == Instr::TRAPV) return true;
    }

    return r1.cycles == r2.cycles;
}

void bugReport()
{
    printf("Please send a bug report to: dirk.hoffmann@me.com\n");
    printf("Thanks you!\n\n");
    exit(1);
}
