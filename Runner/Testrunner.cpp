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
#include <ctime>
#include <string>
#include <strings.h>
#include "UAEGlue/UAECpu.h"

namespace runner {

using namespace moira;

// The reference core (determined by the compatibility mode of the tested Moira)
static constexpr bool useUAE     = (compat == Compatibility::UAE);
static constexpr bool useMusashi = (compat == Compatibility::MUSASHI);

// The Moira instance under test
TestCPU *moiracpu;

// The currently tested CPU type and instruction
Model cpuModel = Model::M68000;
u16 opcode = 0;

static const struct { Model model; const char *name; } modelTable[] = {

    { Model::M68000,   "68000" },
    { Model::M68010,   "68010" },
    { Model::M68EC020, "EC020" },
    { Model::M68020,   "68020" },
    { Model::M68EC030, "EC030" },
    { Model::M68030,   "68030" },
    { Model::M68EC040, "EC040" },
    { Model::M68LC040, "LC040" },
    { Model::M68040,   "68040" }
};

const std::vector<Model> &allModels()
{
    static const std::vector<Model> all = []() {
        std::vector<Model> result;
        for (auto &m : modelTable) result.push_back(m.model);
        return result;
    }();
    return all;
}

const char *modelName(Model model)
{
    for (auto &m : modelTable) if (m.model == model) return m.name;
    return "???";
}

std::optional<Model> parseModel(const char *name)
{
    for (auto &m : modelTable) if (strcasecmp(m.name, name) == 0) return m.model;
    return std::nullopt;
}

static const char *reference()
{
    switch (compat) {

        case Compatibility::MUSASHI: return "Musashi";
        case Compatibility::UAE:     return "UAE";
    
        default:
            return "???";
    }
}

const char *selectedModel()
{
    return modelName(cpuModel);
}

void selectModel(Model model)
{
    cpuModel = model;

    setupMusashi(cpuModel);
    setupUAE(cpuModel);
    setupBinutils(cpuModel);
    setupMoira();

    // Configure the sandbox
    sandbox.checkCycles = useUAE && cpuModel == Model::M68000;
    sandbox.addrMask = (cpuModel == Model::M68EC020) ? 0xFFFFFF : 0xFFFFFFFF;
    sandbox.byteGranular = useUAE && cpuModel != Model::M68000 && cpuModel != Model::M68010;
}

//
// UAE as reference
//

static uaecpu::State uaeState;

static void runUAE(Setup &s, Result &r)
{
    uaecpu::State st;

    // Setup
    
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
    for (int i = 0; i < 8; i++) st.fpse[i] = s.fpr[i].signExp();
    for (int i = 0; i < 8; i++) st.fpman[i] = s.fpr[i].mantissa();
    st.fpcr = s.fpcr;
    st.fpsr = s.fpsr;
    st.fpiar = s.fpiar;

    uaecpu::setState(st);

    // Run

    clock_t elapsed = clock();
    uaecpu::step();
    r.elapsed[0] = clock() - elapsed;
    r.cycles = int(uaecpu::cycles());

    // Analyze
    
    uaeState = uaecpu::getState();
    r.pc = uaeState.pc;
    r.sr = uaeState.sr;
    r.usp = uaeState.usp;
    r.isp = uaeState.isp;
    r.msp = uaeState.msp;
    r.fc = 0;
    r.vbr = uaeState.vbr;
    r.sfc = uaeState.sfc & 7;
    r.dfc = uaeState.dfc & 7;
    r.cacr = uaeState.cacr;
    r.caar = uaeState.caar;
    for (int i = 0; i < 8; i++) r.d[i] = uaeState.d[i];
    for (int i = 0; i < 7; i++) r.a[i] = uaeState.a[i];
    r.a[7] = uaeState.a7;
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

    Instr instr = moiracpu->getInstrInfo(opcode).I;

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

    memcpy(refMem, s.mem, sizeof(refMem));
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

void recordRefFpu(Result &r)
{
    if (useUAE) {
        for (int i = 0; i < 8; i++) r.fpr[i] = Float80::fromBits(uaeState.fpse[i], uaeState.fpman[i]);
        r.fpcr = uaeState.fpcr;
        r.fpsr = uaeState.fpsr;
        r.fpiar = uaeState.fpiar;
        return;
    }
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

clock_t refclk[2] = {0,0}, moiraclk[2] = {0,0};

void run(unsigned seed, const std::vector<Model> &models)
{
    printf("Moira CPU tester. (C) Dirk W. Hoffmann, 2019 - 2026\n");
    printf("Core %s\n\n", Moira::build().c_str());
    printf("The test program runs Moira against a reference CPU with randomly generated data.\n");
    printf("\n");
    printf("    Test rounds : %ld\n", ROUNDS);
    printf("    Random seed : %d\n", seed);
    printf("     Exec range : %s\n", TOSTRING(doExec(opcode)));
    printf("     Dasm range : %s\n", TOSTRING(doDasm(opcode)));
    printf("      Reference : %s\n", reference());

    srand(seed);

    if (!moiracpu) moiracpu = new TestCPU();

    for (long i = 1; i <= ROUNDS; i++) {

        printf("\nRound %ld:\n\n", i);

        for (Model model : models) {

            // Select the CPU core
            cpuModel = model;
            selectModel(cpuModel);

            // Execute tests
            if constexpr (CHECK_CPU) runCPU(i);
            if constexpr (CHECK_MMU) runMMU(i);
            if constexpr (CHECK_FPU) runFPU(i);
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
                if (moiracpu->getInstrInfo(op).I == Instr::ILLEGAL) continue;
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

    // Musashi's FPU is not a reliable reference
    if (!useUAE) {
        printf("%s FPU skipped (no reliable %s reference)\n", selectedModel(), reference());
        return;
    }

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
        if (PROFILE_CPU) { mu += refclk[0]; mo += moiraclk[0]; }
        if (PROFILE_DASM) { mu += refclk[1]; mo += moiraclk[1]; }

        printf(" (Moira: %.2fs  %s: %.2fs)",
               mo / double(CLOCKS_PER_SEC),
               reference(),
               mu / double(CLOCKS_PER_SEC));
    }
    printf("\n");
}

void runSingleTest(Setup &s, u16 op)
{
    Result refResult, moiraResult;

    opcode = op;

    // Prepare the test case with the selected instruction
    setupTestInstruction(s, pc, op);

    // Reset the sandbox (memory accesses observer)
    {
        auto info = moiracpu->getInstrInfo(op);
        bool enable = doExec((int)op) && !skip(op) && !skipAccessCheck(op);
        bool ignore = (info.I == Instr::CLR && info.S == Long) ||
                      (info.I == Instr::CHK && cpuModel != Model::M68000);
        sandbox.prepare(enable, ignore);
    }

    // Prepare
    resetMusashi(s);
    resetMoira(s);

    bool fpu = isFpuOpcode(op);
    if (fpu) {
        resetMusashiFpu(s);
        resetMoiraFpu(s);
    }

    // Run the binutils disassembler
    runBinutils(s, refResult);

    // Run the reference CPU (and the Musashi disassembler)
    runReference(s, refResult);
    // Moira is configured without address error emulation (MOIRA_EMULATE_ADDRESS_ERROR)
    if (useUAE && (uaecpu::lastException() == 3) && !MOIRA_EMULATE_ADDRESS_ERROR) return;
    refclk[0] += refResult.elapsed[0];
    refclk[1] += refResult.elapsed[1];

    // Run Moira
    runMoira(s, moiraResult);
    moiraclk[0] += moiraResult.elapsed[0];
    moiraclk[1] += moiraResult.elapsed[1];

    // Compare
    if (fpu) {
        recordRefFpu(refResult);
        recordMoiraFpu(moiraResult);
    }
    compare(s, refResult, moiraResult);
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

// Disassembles the instruction with Musashi's disassembler and executes it on the
// reference CPU (Musashi or, if selected, UAE)
void runReference(Setup &s, Result &r)
{
    r.oldpc = m68k_get_reg(NULL, M68K_REG_PC);
    r.opcode = get16(refMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;

    // Run the Musashi disassembler (used in both modes, because it is the reference for disassembly)
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

    // Run UAE's disassembler
    r.dasmUAE[0] = 0;
    r.dasmCntUAE = 0;
    if (useUAE) r.dasmCntUAE = uaecpu::disassemble(s.pc, r.dasmUAE, sizeof(r.dasmUAE));

    // Run the reference CPU
    if (doExec(r.opcode)) {

        if (useUAE) {
            runUAE(s, r);
            return;
        }
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
        moiracpu->setDasmLetterCase(LetterCase::MIXED_CASE);
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

        // Disassemble the instruction in Moira's own format (compared with UAE)
        moiracpu->setDasmSyntax(Syntax::UAE);
        moiracpu->setDasmLetterCase(LetterCase::LOWER_CASE);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        r.dasmCntMoiraUAE = moiracpu->disassemble(r.dasmMoiraUAE, r.oldpc);
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
    // UAE only writes back the bytes of a bit field that are affected,
    // whereas Moira writes back the whole long word. The final memory
    // contents are still compared.
    if (!useUAE) return false;
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

    Instr instr = moiracpu->getInstrInfo(op).I;

    if (useUAE) {

        // UAE doesn't implement the module instructions
        if (instr == Instr::CALLM || instr == Instr::RTM) return true;

        // RTE processes random stack frames (format errors, bus error frames)
        if (instr == Instr::RTE && cpuModel != Model::M68000 && cpuModel != Model::M68010) return true;

        // The 68010 supports the bus error stack frame (format $8), which Moira doesn't process.
        // The format word sits at offset 6 of the frame (the supervisor stack pointer is $2000).
        if (instr == Instr::RTE && cpuModel == Model::M68010 && (get16(refMem, 0x2006) >> 12) == 8) return true;

        // LINK.l A7 pushes A7-4 on the 68040 (Moira's 040 models run on the 68020 core)
        if (instr == Instr::LINK && (op & 7) == 7 && cpuModel >= Model::M68EC040) return true;

        // Zero divide uses a different stack frame on the 68040 (Moira's 040 models run on the 68020 core)
        if ((instr == Instr::DIVL || instr == Instr::DIVU || instr == Instr::DIVS) &&
            cpuModel >= Model::M68EC040) return true;

        // UAE emulates FPU instructions that the 68040 traps (Moira follows real hardware)
        if ((op & 0xFFC0) == 0xF200 && cpuModel == Model::M68040) {
            u16 e = get16(refMem, 0x1002);
            u16 cod = e >> 13, cmd = e & 0x7F;
            if ((cod == 0 || cod == 2) && (cmd == 0x02 || cmd == 0x06 || cmd == 0x08 || cmd == 0x09 ||
                (cmd >= 0x0A && cmd <= 0x12 && cmd != 0x0B && cmd != 0x13) ||
                cmd == 0x14 || cmd == 0x15 || cmd == 0x16 || cmd == 0x19 ||
                (cmd >= 0x1C && cmd <= 0x1F) || cmd == 0x21 || cmd == 0x25 || cmd == 0x26 ||
                (cmd >= 0x30 && cmd <= 0x37))) return true;
        }

        // FMOVECR from an undefined ROM offset returns implementation specific values
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(refMem, 0x1002);
            u16 off = e & 0x7F;
            if ((e & 0xFC00) == 0x5C00 && off != 0x00 && !(off >= 0x0B && off <= 0x0F) && !(off >= 0x30 && off <= 0x3F)) return true;
        }

        // FMOVE to <ea> with a non-zero k-factor field is only defined for packed formats
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(refMem, 0x1002);
            u16 fmt = (e >> 10) & 7;
            if ((e >> 13) == 3 && (e & 0x7F) && fmt != 3 && fmt != 7) return true;
        }

        // FMOVEM to/from control registers with an empty register list or non-zero reserved bits is undefined
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(refMem, 0x1002);
            if (((e >> 13) == 4 || (e >> 13) == 5) && ((e & 0x1C00) == 0 || (e & 0x3FF))) return true;
        }

        // FMOVEM <ea>,<control registers> with several registers and Dn/An as source. Moira
        // accepts this, UAE (and the 6888x manual) don't. Flagged, not mimicked.
        if ((op & 0xFFC0) == 0xF200 && ((op >> 3) & 7) <= 1) {
            u16 e = get16(refMem, 0x1002);
            u16 lst = (e >> 10) & 7;
            if ((e >> 13) == 4 && lst != 1 && lst != 2 && lst != 4) return true;
            if (((e >> 13) == 4 || (e >> 13) == 5) && ((op >> 3) & 7) == 1 && lst != 1) return true;
        }

        // Register-to-register FPU operations and FMOVECR with a non-zero <ea> field: Moira raises line F, UAE ignores the field
        if ((op & 0xFFC0) == 0xF200 && (op & 0x3F) && ((get16(refMem, 0x1002) >> 13) == 0 ||
            (get16(refMem, 0x1002) & 0xFC00) == 0x5C00)) return true;

        // Rounded results of the 68040 FS/FD instructions differ on underflow (under investigation)
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(refMem, 0x1002);
            if (((e >> 13) == 0 || (e >> 13) == 2) && (e & 0x40)) return true;
        }

        // Packed decimal: random input contains invalid digits and the digit rounding on output differs
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(refMem, 0x1002);
            if (((e >> 13) == 2 || (e >> 13) == 3) && (((e >> 10) & 7) == 3 || ((e >> 10) & 7) == 7)) return true;
        }

        // FMOVEM data registers with an empty static list or reserved mode bits
        if ((op & 0xFFC0) == 0xF200) {
            u16 e = get16(refMem, 0x1002);
            if (((e >> 13) == 6 || (e >> 13) == 7) && !(e & 0x0800) && (e & 0xFF) == 0) return true;
            if (((e >> 13) == 6 || (e >> 13) == 7) && (((e >> 11) & 3) < 2 || (e & 0x0700) || (((e >> 11) & 3) == 3 && (e & 0x8F)))) return true;
        }

        // MULL with identical high and low registers has an undefined result
        if (instr == Instr::MULL) {
            u16 e = get16(refMem, 0x1002);
            if ((e & 0x400) && ((e >> 12) & 7) == (e & 7)) return true;
        }

        // CAS2 with identical compare registers is degenerate
        if (instr == Instr::CAS2) {
            u16 e1 = get16(refMem, 0x1002), e2 = get16(refMem, 0x1004);
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
                u16 w = get16(refMem, 0x1000 + off);
                if ((w & 0x100) == 0) continue;
                bool bs = (w & 0x30) == 0 || (w & 0x08) || (w & 7) == 4 || ((w & 0x40) && (w & 7) >= 5);
                if (bs) return true;
            }
        }
        return false;
    }

    // Skip some instructions that the reference CPUs handle differently (known Musashi problems)
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

void compare(Setup &s, Result &rRef, Result &rMoira)
{
    bool error = false;

    if (doDasm((int)s.opcode)) {

        if (!compareDasm(rRef, rMoira)) {
            printf("\nDISASSEMBLER MISMATCH FOUND");
            error = true;
        }
    }

    if (doExec((int)s.opcode)) {

        // FPU opcodes where only one side raises line F (reserved bits set in the opcode or extension word)
        bool legalityMismatch = useUAE && (s.opcode & 0xFE00) == 0xF200 &&
                                ((rRef.pc >> 12) == 1) != ((rMoira.pc >> 12) == 1);

        // The FPCR precision value 3 is reserved. UAE and Moira round tiny results differently in this mode.
        bool reservedPrecision = useUAE && (s.opcode & 0xFE00) == 0xF200 && (s.fpcr & 0xC0) == 0xC0;

        if (!legalityMismatch && !reservedPrecision && !skip(s.opcode)) {

            if (!comparePC(rRef, rMoira)) {
                printf("\nPROGRAM COUNTER MISMATCH FOUND");
                error = true;
            }
            if (!compareCycles(rRef, rMoira)) {
                printf("\nCLOCK MISMATCH FOUND");
                error = true;
            }
            if (!compareSR(rRef, rMoira)) {
                printf("\nSTATUS REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareUSP(rRef, rMoira)) {
                printf("\nUSP MISMATCH FOUND");
                error = true;
            }
            if (!compareISP(rRef, rMoira)) {
                printf("\nISP MISMATCH FOUND");
                error = true;
            }
            if (!compareMSP(rRef, rMoira)) {
                printf("\nMSP MISMATCH FOUND");
                error = true;
            }
            if (!compareD(rRef, rMoira)) {
                printf("\nDATA REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareA(rRef, rMoira)) {
                printf("\nADDRESS REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareVBR(rRef, rMoira)) {
                printf("\nVBR REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareSFC(rRef, rMoira)) {
                printf("\nSFC REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareDFC(rRef, rMoira)) {
                printf("\nDFC REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareCAxR(rRef, rMoira)) {
                printf("\nCACHE REGISTER MISMATCH FOUND");
                error = true;
            }
            if (isFpuOpcode(s.opcode)) {

                if (!compareFPR(rRef, rMoira)) {
                    printf("\nFLOATING-POINT REGISTER MISMATCH FOUND");
                    error = true;
                }
                if (!compareFPCR(rRef, rMoira)) {
                    printf("\nFPCR MISMATCH FOUND");
                    error = true;
                }
                if (!compareFPSR(rRef, rMoira)) {
                    printf("\nFPSR MISMATCH FOUND");
                    error = true;
                }
                if (!compareFPIAR(rRef, rMoira)) {
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
        printf("\nInstruction:  Musashi: [%d] '%s'", rRef.dasmCntMusashi, rRef.dasmMusashi);
        printf("\n                Moira: [%d] '%s'\n", rMoira.dasmCntMusashi, rMoira.dasmMusashi);

        printf("\n             Binutils: [%d] '%s'", rRef.dasmCntBinutils, rRef.dasmBinutils);
        printf("\n                Moira: [%d] '%s'\n", rMoira.dasmCntBinutils, rMoira.dasmBinutils);
        printf("\n");

        printf("Setup:   ");
        dumpSetup(s);

        printf("%-7s: ", reference());
        dumpResult(rRef);

        printf("Moira:   ");
        dumpResult(rMoira);

        if (isFpuOpcode(s.opcode)) {
            for (int i = 0; i < 8; i++) {
                printf("FP%d: setup %s  %s %s  Moira %s\n", i,
                       s.fpr[i].str().c_str(), reference(), rRef.fpr[i].str().c_str(), rMoira.fpr[i].str().c_str());
            }
            printf("FPCR: %08x / %08x / %08x (setup / %s / Moira)\n", s.fpcr, rRef.fpcr, rMoira.fpcr, reference());
            printf("FPSR: %08x / %08x / %08x\n", s.fpsr, rRef.fpsr, rMoira.fpsr);
            printf("FPIAR: %08x / %08x / %08x\n\n", s.fpiar, rRef.fpiar, rMoira.fpiar);
        }

        bugReport();
    }
}


//
// Comparing disassembler output with UAE
//

// Returns true if the two disassembler outputs match. Instructions that either
// side regards as illegal are not compared, because the two disassemblers
// differ in how they report them.
bool dasmEqualUAE(const char *uae, const char *moira)
{
    auto starts = [](const char *s, const char *prefix) { return strncasecmp(s, prefix, strlen(prefix)) == 0; };

    if (starts(uae, "illegal") || starts(uae, "[") || starts(uae, "f-line") || starts(uae, "f?")) return true;
    if (starts(uae, "movec")) return strcasecmp(uae, moira) == 0;   // UAE prints some control registers in upper case
    if (strstr(uae, "(68020+)")) return true;
    if (starts(uae, "fmovecr") || strstr(uae, "#+") || strstr(uae, "#-") || (starts(uae, "fmove.p") && strstr(uae, ",#$")) || strstr(uae, "/#$")) {
        return true;   // Floating-point constants are printed in a different notation
    }   // Addressing mode not available on the selected model
    if (starts(moira, "dc.w") || isdigit((unsigned char)moira[0])) return true;

    return strcmp(uae, moira) == 0;
}

static bool noUAEDasmForCoprocessor(u16 opcode)
{
    switch (cpuModel) {
        case Model::M68EC020: case Model::M68EC030: case Model::M68EC040: case Model::M68LC040:
            return opcode >= 0xF000;
        default:
            return false;
    }
}

bool compareDasm(Result &rRef, Result &rMoira)
{
    auto I = moiracpu->getInstrInfo(rRef.opcode).I;

    bool skipMusashi = rRef.opcode >= 0xF000;
    bool skipBinutils = false;

    switch (cpuModel) {

        case Model::M68010:

            skipBinutils |= I == Instr::CMPI;
            break;
            
        case Model::M68EC020:
        case Model::M68EC030:
        case Model::M68EC040:
        case Model::M68LC040:

            skipBinutils |= rRef.opcode >= 0xF000;
            break;

        default:
            break;
    }

    // Compare with the Musashi disassembler
    if (!skipMusashi) {

        if (rRef.dasmCntMusashi != rMoira.dasmCntMusashi) return false;
        if (strcmp(rRef.dasmMusashi, rMoira.dasmMusashi) != 0) return false;
    }

    // Compare with UAE's disassembler (UAE decodes coprocessor opcodes regardless of the model)
    if (useUAE && !noUAEDasmForCoprocessor(rRef.opcode) && !dasmEqualUAE(rRef.dasmUAE, rMoira.dasmMoiraUAE)) return false;

    // Compare with binutils
    if (!skipBinutils) {

        if (I != Instr::FTRAPcc) {
            if (rRef.dasmCntBinutils != rMoira.dasmCntBinutils) return false;
        }
        if (strcmp(rRef.dasmBinutils, rMoira.dasmBinutils) != 0) return false;
    }

    return true;
}

bool compareD(Result &rRef, Result &rMoira)
{
    assert(rRef.opcode == rMoira.opcode);
    Instr instr = moiracpu->getInstrInfo(rRef.opcode).I;
    if (instr == Instr::DIVU || instr == Instr::DIVS)
    {
        // The reference CPUs differ in some corner cases
        return true;
    }

    for (int i = 0; i < 8; i++) if (rRef.d[i] != rMoira.d[i]) return false;
    return true;
}

bool compareA(Result &rRef, Result &rMoira)
{
    for (int i = 0; i < 8; i++) if (rRef.a[i] != rMoira.a[i]) return false;
    return true;
}

bool comparePC(Result &rRef, Result &rMoira)
{
    // UAE clears the lowest PC bit after a jump to an odd address. Moira
    // is configured without address error emulation and keeps it.
    if (useUAE && (rMoira.pc & 1) && rRef.pc == (rMoira.pc & ~1u)) return true;

    return rRef.pc == rMoira.pc;
}

bool compareSR(Result &rRef, Result &rMoira)
{
    assert(rRef.opcode == rMoira.opcode);
    Instr instr = moiracpu->getInstrInfo(rRef.opcode).I;
    if (instr == Instr::DIVU || instr == Instr::DIVS) {

        // The reference CPU differs (and is likely wrong). Ignore it
        return true;
    }

    // N and V are undefined for CMP2 and CHK2 and not compared with UAE
    if (useUAE && (instr == Instr::CMP2 || instr == Instr::CHK2)) {
        return (rRef.sr & ~0x0A) == (rMoira.sr & ~0x0A);
    }

    // N and V are undefined for BCD instructions (68040 differs from older models)
    if (useUAE && cpuModel >= Model::M68EC040 &&
        (instr == Instr::ABCD || instr == Instr::SBCD || instr == Instr::NBCD)) {
        return (rRef.sr & ~0x0A) == (rMoira.sr & ~0x0A);
    }

    // Z, V and C are undefined for CHK on the 68020 and above
    if (useUAE && instr == Instr::CHK && cpuModel != Model::M68000 && cpuModel != Model::M68010) {
        return (rRef.sr & ~0x07) == (rMoira.sr & ~0x07);
    }

    return rRef.sr == rMoira.sr;
}

bool compareUSP(Result &rRef, Result &rMoira)
{
    return rRef.usp == rMoira.usp;
}

bool compareISP(Result &rRef, Result &rMoira)
{
    return rRef.isp == rMoira.isp;
}

bool compareMSP(Result &rRef, Result &rMoira)
{
    return rRef.msp == rMoira.msp;
}

bool compareVBR(Result &rRef, Result &rMoira)
{
    return rRef.vbr == rMoira.vbr;
}

bool compareSFC(Result &rRef, Result &rMoira)
{
    return rRef.sfc == rMoira.sfc;
}

bool compareDFC(Result &rRef, Result &rMoira)
{
    return rRef.dfc == rMoira.dfc;
}

bool compareCAxR(Result &rRef, Result &rMoira)
{
    return rRef.cacr == rMoira.cacr && rRef.caar == rMoira.caar;
}

bool compareFPR(Result &rRef, Result &rMoira)
{
    for (int i = 0; i < 8; i++) {

        if (rRef.fpr[i].identical(rMoira.fpr[i])) continue;

        // On underflow, UAE returns a zero significand with the minimum exponent of the
        // format instead of a true zero (same exemption as in FpuRefTests)
        if (useUAE) {
            auto &ref = rRef.fpr[i], &moira = rMoira.fpr[i];
            bool pseudoZero = ref.mantissa() == 0 && ref.biasedExponent() != 0 && ref.biasedExponent() != 0x7FFF;
            if (pseudoZero && moira.isZero() && moira.sign() == ref.sign()) continue;
        }
        return false;
    }
    return true;
}

bool compareFPCR(Result &rRef, Result &rMoira)
{
    // UAE stores the reserved low bits of FPCR
    if (useUAE) return (rRef.fpcr & ~0xF) == (rMoira.fpcr & ~0xF);

    return rRef.fpcr == rMoira.fpcr;
}
bool compareFPSR(Result &rRef, Result &rMoira)
{
    // UAE modifies FPSR even if the instruction ends up in a line F exception
    if (useUAE && (rRef.pc >> 12) != 1) return true;

    /* An overflow always implies an inexact result. UAE only raises INEX2 if the
     * rounding discards non-zero bits, which isn't the case if the operand is a
     * huge unnormalized number whose low mantissa bits are zero after normalization.
     */
    u32 ref = rRef.fpsr, moira = rMoira.fpsr;
    if (useUAE && (ref & 0x1000) && (moira & 0x1000)) { ref |= 0x0200; moira |= 0x0200; }

    // UAE doesn't raise OPERR when it converts a signaling NaN to an integer
    if (useUAE && (ref & 0x4000)) return (ref | 0x2000) == (moira | 0x2000);

    return ref == moira;
}
bool compareFPIAR(Result &rRef, Result &rMoira)
{
    // UAE updates FPIAR even if the instruction ends up in a line F exception
    if (useUAE && (rRef.pc >> 12) != 1) return true;

    // UAE skips the FPIAR update if a conversion in FMOVE <ea> raises an operand error
    if (useUAE && (rRef.fpsr & 0x6000)) return true;

    return rRef.fpiar == rMoira.fpiar;
}
bool compareMemory() { return memcmp(refMem, moiraMem, sizeof(refMem)) == 0; }

bool compareCycles(Result &rRef, Result &rMoira)
{
    assert(rRef.opcode == rMoira.opcode);
    auto I = moiracpu->getInstrInfo(rRef.opcode).I;
    auto S = moiracpu->getInstrInfo(rRef.opcode).S;
    auto M = moiracpu->getInstrInfo(rRef.opcode).M;

    // Exclude some instructions
    if (I == Instr::TAS) return true;

    // FPU timing is not emulated yet
    if (isFpuOpcode(rRef.opcode)) return true;

    /* UAE's cores are only used as a timing reference for the 68000. The
     * 68010 timing is partially copied from the 68000, and the 68020+ cores
     * aren't cycle-exact.
     */
    if (useUAE && cpuModel != Model::M68000) return true;

    /* Timing comparison is off for every model beyond the 68000 and 68010
     * while the instruction cache is emulated. Those models (68020 and up)
     * all execute on Moira's C68020 core (see createJumpTable() in
     * MoiraInit_cpp.h), which routes instruction fetches through the cache.
     * Musashi does not model the 68020's instruction cache, so its cycle
     * counts assume every fetch goes straight to the bus. Moira's
     * cache-aware fetch timing (tuned against WinUAE/UAE, not Musashi)
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

    return rRef.cycles == rMoira.cycles;
}

void bugReport()
{
    printf("Please send a bug report to: dirk.hoffmann@me.com\n");
    printf("Thanks you!\n\n");
    exit(1);
}

} // namespace runner
