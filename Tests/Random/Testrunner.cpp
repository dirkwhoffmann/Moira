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
#include "Reference.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <string>
#ifdef _MSC_VER
#include <string.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

namespace runner {

using namespace moira;

// The reference core (determined by the compatibility mode of the tested Moira)
static constexpr bool useUAE     = (compat == Compatibility::UAE);
static constexpr bool useMusashi = (compat == Compatibility::MUSASHI);
static constexpr bool useMAME    = (compat == Compatibility::MAME);

// The disassembler tests don't depend on the compatibility mode. They compare Moira with
// all reference disassemblers at once (the MIT flavor compares Moira with Binutils only).
// Set while MMU instructions are compared against UAE
static bool mmuCompare = false;

// The setup of the test that is currently running (see RUNNER_DUMP)
static Setup *lastSetup = nullptr;

static constexpr bool dasmMusashi = DASM_STAGES ? !DASM_MIT : useMusashi;
static constexpr bool dasmUAE     = DASM_STAGES ? !DASM_MIT : useUAE;

// The Moira instance under test
TestCPU *moiracpu;

// The currently tested CPU type and instruction
CPUModel cpuModel = CPUModel::M68000;
FPUModel fpuModel = FPUModel::NONE;
u16 opcode = 0;

// Speed comparison
clock_t refclk[2] = {0,0};
clock_t moiraclk[2] = {0,0};

static void resetClocks() { refclk[0] = refclk[1] = moiraclk[0] = moiraclk[1] = 0; }

static const struct { CPUModel model; const char *name; } modelTable[] = {

    { CPUModel::M68000,   "68000" },
    { CPUModel::M68010,   "68010" },
    { CPUModel::M68EC020, "EC020" },
    { CPUModel::M68020,   "68020" },
    { CPUModel::M68EC030, "EC030" },
    { CPUModel::M68030,   "68030" },
    { CPUModel::M68EC040, "EC040" },
    { CPUModel::M68LC040, "LC040" },
    { CPUModel::M68040,   "68040" }
};

const std::vector<CPUModel> &allModels()
{
    static const std::vector<CPUModel> all = []() {
        std::vector<CPUModel> result;
        for (auto &m : modelTable) if (DASM_STAGES || ref().supports(m.model)) result.push_back(m.model);
        return result;
    }();
    return all;
}

const char *modelName(CPUModel model)
{
    for (auto &m : modelTable) if (m.model == model) return m.name;
    return "???";
}

std::optional<CPUModel> parseModel(const char *name)
{
    for (auto &m : modelTable) if (strcasecmp(m.name, name) == 0) return m.model;
    return std::nullopt;
}

static const struct { FPUModel model; const char *name; } fpuTable[] = {

    { FPUModel::M68881,   "68881" },
    { FPUModel::M68882,   "68882" },
    { FPUModel::BUILTIN, "builtin" }
};

const std::vector<FPUModel> &allFpuModels()
{
    static const std::vector<FPUModel> all = []() {
        std::vector<FPUModel> result;
        for (auto &m : fpuTable) result.push_back(m.model);
        return result;
    }();
    return all;
}

const char *fpuModelName(FPUModel model)
{
    for (auto &m : fpuTable) if (m.model == model) return m.name;
    return "none";
}

std::optional<FPUModel> parseFpuModel(const char *name)
{
    for (auto &m : fpuTable) if (strcasecmp(m.name, name) == 0) return m.model;
    return std::nullopt;
}

bool isCompatible(CPUModel cpu, FPUModel fpu)
{
    switch (fpu) {

        case FPUModel::BUILTIN:
            return cpu == CPUModel::M68040;

        case FPUModel::M68881:
        case FPUModel::M68882:
            return cpu == CPUModel::M68020 || cpu == CPUModel::M68EC020 ||
                   cpu == CPUModel::M68030 || cpu == CPUModel::M68EC030;

        default:
            return false;
    }
}

const char *reference()
{
    return DASM_STAGES ? "Ref" : ref().name();
}

const char *selectedModel()
{
    return modelName(cpuModel);
}

void selectModel(CPUModel model)
{
    cpuModel = model;

    // Only the 68040 has a built-in FPU. External FPUs are installed with selectFpu.
    fpuModel = (cpuModel == CPUModel::M68040) ? FPUModel::BUILTIN : FPUModel::NONE;

    if (!DASM_STAGES) ref().setup(cpuModel, fpuModel);
    if (dasmMusashi) setupMusashi(cpuModel);
    if (dasmUAE) setupUAE(cpuModel, fpuModel);
    setupBinutils(cpuModel);
    setupMoira();

    // Configure the sandbox
    sandbox.checkCycles = ref().checksCycles(cpuModel);
    sandbox.addrMask = (cpuModel == CPUModel::M68EC020) ? 0xFFFFFF : 0xFFFFFFFF;
    sandbox.byteGranular = ref().byteGranular(cpuModel);
}

void setupMoira()
{
    moiracpu->setModel(cpuModel);
    moiracpu->setFpuModel(fpuModel);
}

void selectFpu(FPUModel fpu)
{
    fpuModel = fpu;

    if (!DASM_STAGES) ref().selectFpu(cpuModel, fpuModel);
    if (dasmUAE) setupUAE(cpuModel, fpuModel);
    setupMoira();
}

void setupTestEnvironment(Setup &s, long round)
{
    s.round = round;
    s.supervisor = round % 2;
    s.ccr = u8(portableRand());
    s.ext1 = u16(randomizer.rand());
    s.ext2 = u16(randomizer.rand());
    s.ext3 = u16(randomizer.rand());
    s.vbr = cpuModel == CPUModel::M68000 ? 0 : u16(randomizer.rand() & ~1);
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

    // Let the reference adjust the test case (if needed)
    ref().prepare(s);

    set16(s.mem, pc, opcode);
    set16(s.mem, pc + 2, s.ext1);
    set16(s.mem, pc + 4, s.ext2);
    set16(s.mem, pc + 6, s.ext3);

    memcpy(refMem, s.mem, sizeof(refMem));
    memcpy(moiraMem, s.mem, sizeof(moiraMem));
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

    for (int i = 0; i < 8; i++) moiracpu->fpu.fpr[i] = s.fpr[i];
    moiracpu->fpu.setFPCR(s.fpcr);
    moiracpu->fpu.setFPSR(s.fpsr);
    moiracpu->fpu.setFPIAR(s.fpiar);

    moiracpu->mmu.mmu030.crp = s.crp;
    moiracpu->mmu.mmu030.srp = s.srp;
    moiracpu->mmu.mmu030.tc = s.tc;
    moiracpu->updateMmuFlag();
    moiracpu->mmu.mmu030.tt0 = s.tt0;
    moiracpu->mmu.mmu030.tt1 = s.tt1;
    moiracpu->mmu.mmu030.mmusr = s.mmusr;
    for (auto &e : moiracpu->mmu.mmu030.atc) e.valid = false;
}

void run(unsigned seed, long rounds, const std::vector<CPUModel> &cpus, const std::vector<FPUModel> &fpus)
{
    portableSrand(seed);

    if (!moiracpu) moiracpu = new TestCPU();

    // Replay a single test that has been dumped with RUNNER_DUMP
    if (const char *name = getenv("RUNNER_REPLAY")) {

        FILE *f = fopen(name, "rb");
        if (!f) { printf("Cannot open %s\n", name); exit(1); }
        int model = 0;
        static Setup replay;
        if (fread(&model, sizeof(model), 1, f) != 1 || fread(&replay, sizeof(Setup), 1, f) != 1) { printf("Cannot read %s\n", name); exit(1); }
        fclose(f);

        if (getenv("RUNNER_USER")) replay.supervisor = false;
        selectModel(CPUModel(model));
        if (ref().comparesMMU(cpuModel)) { mmuCompare = true; enableMMUUAE(true); }
        runSingleTest(replay, replay.opcode, replay.ext1);
        printf("Replayed test passed\n");
        return;
    }

    for (long i = 1; i <= rounds; i++) {

        printf("\nRound %ld:\n\n", i);

        for (CPUModel cpu : cpus) {

            // Select the CPU core
            selectModel(cpu);

            // Execute tests
            if constexpr (CHECK_CPU) runCPU(i);
            if constexpr (CHECK_MMU) { runMMU(i); if (useUAE && cpu == CPUModel::M68030) runMMUTranslate(i);
                                    if (useUAE && cpu == CPUModel::M68040) runMMUTranslate040(i); }

            // Test the disassembler
            if constexpr (CHECK_DASM) {
                runDasmCPU(i);
                runDasmMMU(i);
            }

            // Run the FPU tests for all FPUs that can be combined with this CPU
            if constexpr (CHECK_FPU || CHECK_DASM) {

                for (FPUModel fpu : fpus) {

                    if (!isCompatible(cpu, fpu)) continue;
                    selectFpu(fpu);
                    if constexpr (CHECK_FPU) runFPU(i);
                    if constexpr (CHECK_DASM) runDasmFPU(i);

                    // Without a reference FPU, there is nothing more to report
                    if (!ref().hasFpu()) break;
                }
            }
        }
    }
}

void runCPU(long round)
{
    static const int CPU_SETUPS = 32;
    
    Setup setup;
    resetClocks();

    printf("%11s CPU : CORE ", selectedModel()); fflush(stdout);

    for (int i = 0; i < CPU_SETUPS; i++) {

        printf("."); fflush(stdout);

        setupTestEnvironment(setup, round * CPU_SETUPS + i);

        for (int op = 0x0000; op <= 0xFFFF; op++) {

            // Skip illegal opcodes if requested
            if constexpr (SKIP_ILLEGAL) { if (isIllegalOpcode(u16(op))) continue; }

            // Skip instructions covered by runFPU
            if (isFpuOpcode(u16(op))) continue;

            // Execute both CPU cores
            runSingleTest(setup, u16(op));
        }
    }
    passed(moiraclk[0], refclk[0]);
}

//
// MMU address translation
//

namespace {

struct TranslationSetup {

    u32 tc;
    u64 crp;
    u64 srp;
    u32 tt0;
    u32 tt1;
    int levels;
    int bits[4];
};

// Fills the memory with descriptor-like data
void fillWithDescriptors()
{
    for (int i = 0; i < 0x10000; i += 4) {

        u32 w;
        int kind = portableRand() % 8;

        if (kind < 4) {
            // Short descriptor
            u32 r0 = u32(portableRand()), r1 = portableRand(), r2 = portableRand();
            w = (r0 & 0xFFFFFFF0) | (r1 % 4) | (r2 & 0xC);
            if (portableRand() % 6 == 0) w &= ~3u;
        } else if (kind < 6) {
            // Upper half of a long descriptor (limit, flags, DT)
            u32 limit = (portableRand() % 3 == 0) ? 0x7FFF : u32(portableRand() % 70);
            u32 r0 = portableRand(), r1 = portableRand(), r2 = portableRand(), r3 = portableRand();
            w = (r0 % 6 == 0 ? 0x80000000 : 0) | limit << 16 | (r1 & 0x0100) | (r2 & 0x1C) | (r3 % 4);
            if (portableRand() % 6 == 0) w &= ~3u;
        } else {
            // Lower half of a long descriptor (address)
            u32 r0 = portableRand();
            u32 hi = portableRand() % 3 == 0 ? u32(portableRand()) << 16 : 0;
            w = (r0 & 0xFFF0) | hi;
        }
        refMem[i] = moiraMem[i] = u8(w >> 24);
        refMem[i + 1] = moiraMem[i + 1] = u8(w >> 16);
        refMem[i + 2] = moiraMem[i + 2] = u8(w >> 8);
        refMem[i + 3] = moiraMem[i + 3] = u8(w);
    }
}

TranslationSetup randomTranslationSetup()
{
    TranslationSetup t;

    // Page size, initial shift, and table index bits (sum must be 32)
    int ps = 8 + portableRand() % 8;
    int is = portableRand() % 3 == 0 ? portableRand() % 6 : 0;
    int rest = 32 - ps - is;

    do {
        t.levels = 2 + portableRand() % 3;
        int left = rest;
        for (int i = 0; i < 4; i++) t.bits[i] = 0;
        for (int i = 0; i < t.levels; i++) {
            int minLeft = t.levels - i - 1;
            int maxBits = std::min(6, left - minLeft);
            int b = (i == t.levels - 1) ? left : 1 + portableRand() % maxBits;
            t.bits[i] = b;
            left -= b;
        }
        if (left != 0) t.levels = 0;
    } while (t.levels == 0 || t.bits[t.levels - 1] > 6 || t.bits[t.levels - 1] < 1);

    u32 tcE = portableRand() % 2 ? 0x02000000 : 0;
    u32 tcF = portableRand() % 4 == 0 ? 0x01000000 : 0;
    t.tc = 0x80000000 | tcE | tcF | ps << 20 | is << 16
         | t.bits[0] << 12 | t.bits[1] << 8 | t.bits[2] << 4 | t.bits[3];

    auto rootPointer = [&]() {
        u64 limit = portableRand() % 3 == 0 ? 0x7FFF : portableRand() % 70;
        u64 lu = portableRand() % 6 == 0;
        u64 dt = 1 + portableRand() % 3;
        return lu << 63 | limit << 48 | dt << 32 | u64(portableRand() & 0xFFF0);
    };
    auto transparentTranslation = [&]() {
        if (portableRand() % 2) return u32(0);
        u32 mask = (0xFF00 >> (portableRand() % 9)) & 0xFF;
        u32 b = portableRand() & 0xFF;
        u32 f10 = portableRand() % 2, f9 = portableRand() % 2, f8 = portableRand() % 2;
        u32 m1 = portableRand() % 8, m0 = portableRand() % 8;
        return b << 24 | mask << 16 | 0x8000 | f10 << 10 | f9 << 9 | f8 << 8 | m1 << 4 | m0;
    };
    t.tt0 = transparentTranslation();
    t.tt1 = transparentTranslation();
    t.crp = rootPointer();
    t.srp = rootPointer();
    return t;
}

// Fills the memory with 68040 table descriptors
void fillWith040Descriptors()
{
    for (int i = 0; i < 0x10000; i += 4) {

        // Descriptor type: invalid, page, indirect, or table / page
        int r = portableRand() % 100;
        u32 type = r < 15 ? 0 : r < 55 ? 1 : r < 70 ? 2 : 3;

        u32 w0 = u32(portableRand()), w1 = u32(portableRand());
        u32 w = (w0 & 0xFF00) | (w1 & 0xFC) | type;
        if (portableRand() % 3 == 0) w &= ~0x7Cu;               // Clear W, U, M, CM, S
        if (portableRand() % 4 == 0) w |= u32(portableRand() & 3) << 8; // U0, U1
        if (portableRand() % 4 == 0) w |= 0x400;                // G
        refMem[i] = moiraMem[i] = u8(w >> 24);
        refMem[i + 1] = moiraMem[i + 1] = u8(w >> 16);
        refMem[i + 2] = moiraMem[i + 2] = u8(w >> 8);
        refMem[i + 3] = moiraMem[i + 3] = u8(w);
    }
}

u32 randomTT040()
{
    if (portableRand() % 2) return 0;
    u32 mask = (0xFF00 >> (portableRand() % 9)) & 0xFF;
    u32 b = portableRand() & 0xFF;
    u32 f14 = portableRand() % 2, f13 = portableRand() % 2;
    u32 f2 = portableRand() % 4 == 0, f5 = portableRand() % 4, f8 = portableRand() % 4;
    return b << 24 | mask << 16 | 0x8000 | f14 << 14 | f13 << 13 | f2 << 2 | f5 << 5 | f8 << 8;
}

}


void runMMU(long round)
{
    if (!moiracpu->hasMMU()) return;

    // Let UAE execute MMU instructions and compare the MMU registers
    mmuCompare = ref().comparesMMU(cpuModel);
    if (mmuCompare) enableMMUUAE(true);
 
    static const int MMU_SETUPS = 4;
    Setup setup;
    resetClocks();

    printf("%11s MMU : CORE ", selectedModel()); fflush(stdout);

    for (int i = 0; i < MMU_SETUPS; i++) {

        setupTestEnvironment(setup, round * MMU_SETUPS + i);

        // Start with a valid (but disabled) translation setup
        TranslationSetup t = randomTranslationSetup();
        setup.tc = t.tc & 0x7FFFFFFF;
        setup.crp = t.crp;
        setup.srp = t.srp;
        setup.tt0 = t.tt0;
        setup.tt1 = t.tt1;

        for (int op = 0xF000; op <= 0xF03F; op++) {

            if ((op % 8) == 0) { printf("."); fflush(stdout); }

            for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

                runSingleTest(setup, u16(op), u16(ext));
            }
        }
    }
    if (mmuCompare) { enableMMUUAE(false); mmuCompare = false; }
    passed(moiraclk[0], refclk[0]);
}

void runMMUTranslate(long round)
{
    static const int TABLE_SETUPS = 256;
    static const int ADDRESSES = 512;

    long successes = 0, faults = 0;
    resetClocks();
    printf("%11s MMU : XLAT ", selectedModel()); fflush(stdout);

    for (int i = 0; i < TABLE_SETUPS; i++) {

        if (i % 16 == 0) { printf("."); fflush(stdout); }

        fillWithDescriptors();
        auto t = randomTranslationSetup();

        // Configure both MMUs
        setMMUUAE(t.crp, t.srp, t.tc, t.tt0, t.tt1);
        moiracpu->mmu.mmu030.crp = t.crp;
        moiracpu->mmu.mmu030.srp = t.srp;
        moiracpu->mmu.mmu030.tc = t.tc;
        moiracpu->mmu.mmu030.tt0 = t.tt0;
        moiracpu->mmu.mmu030.tt1 = t.tt1;

        u32 lastAddr = 0; u8 lastFC = 1;

        for (int j = 0; j < ADDRESSES; j++) {

            // Flush the ATCs regularly. This keeps the number of cached pages below
            // the ATC size, so that the (pseudo-LRU) replacement strategy doesn't matter.
            if (j % 16 == 0) {
                flushATCUAE();
                for (auto &e : moiracpu->mmu.mmu030.atc) e.valid = false;
            }

            // Pick an address that is likely to be covered by the tables
            u32 addr = u32(portableRand()) << (portableRand() % 17);
            if (portableRand() % 4 == 0) addr &= 0xFFFF;
            bool super = portableRand() % 2, data = portableRand() % 2, write = data && portableRand() % 2;
            u8 fc = (super ? 4 : 0) | (data ? 1 : 2);

            // UAE's ATC lookup is undefined for write accesses if the root pointer
            // terminates the walk, because the M bit has nowhere to live.
            u64 rp = (super && (t.tc & 0x02000000)) ? t.srp : t.crp;
            if (write && ((rp >> 32) & 3) == 1) write = false;   // (also with FCL: FC lookup is skipped)

            // Run PTEST on both MMUs (no side effects)
            {
                u32 level = portableRand() % 6;
                u32 paddr = addr, pfc = fc;
                if (level == 0) {

                    // UAE compares the unmasked address with the ATC entries. Use the
                    // page of an earlier access to get ATC hits.
                    paddr = (portableRand() % 4 ? lastAddr : addr) & ~((1u << (t.tc >> 20 & 0xF)) - 1);
                    if (portableRand() % 4) pfc = lastFC;
                }
                bool pwrite = portableRand() % 2;
                u32 statusRef = 0, descrRef = 0, descrMoira = 0;
                ptestUAE(paddr, pfc, pwrite, level, &statusRef, &descrRef);
                moiracpu->setSupervisorMode(super);
                sandbox.prepare(false, true);
                u32 statusMoira = moiracpu->mmu.mmu030.test(paddr, pfc, pwrite, level, descrMoira);

                // The descriptor address is only returned for levels > 0
                bool equal = statusRef == statusMoira && (level == 0 || descrRef == descrMoira) && !memcmp(refMem, moiraMem, sizeof(refMem));
                if (!equal) {

                    printf("\n\nPTEST MISMATCH FOUND\n\n");
                    printf("Setup %d.%d: TC: %08x  CRP: %016llx  SRP: %016llx  TT0: %08x  TT1: %08x\n", i, j, t.tc, t.crp, t.srp, t.tt0, t.tt1);
                    printf("PTEST%c: addr %08x  FC %d  level %d\n", pwrite ? 'W' : 'R', paddr, pfc, level);
                    printf("UAE  : MMUSR %04x  descriptor %08x\n", statusRef, descrRef);
                    printf("Moira: MMUSR %04x  descriptor %08x\n", statusMoira, descrMoira);
                    exit(1);
                }
            }

            // Translate with both MMUs
            u32 physRef = 0, physMoira = 0;
            bool okRef = translateUAE(addr, super, data, write, &physRef);
            moiracpu->setSupervisorMode(super);
            sandbox.prepare(false, true);     // Don't record the descriptor updates
            auto translated = moiracpu->mmu.translate(addr, fc, write);
            bool okMoira = translated.has_value();
            if (okMoira) physMoira = *translated;

            (okRef ? successes : faults)++;
            lastAddr = addr; lastFC = fc;
            bool equal = okRef == okMoira && (!okRef || physRef == physMoira) && !memcmp(refMem, moiraMem, sizeof(refMem));
            if (equal) continue;

            printf("\n\nTRANSLATION MISMATCH FOUND\n\n");
            printf("Setup %d.%d: TC: %08x  CRP: %016llx  SRP: %016llx  TT0: %08x  TT1: %08x\n", i, j, t.tc, t.crp, t.srp, t.tt0, t.tt1);
            printf("Access: addr %08x  FC %d  %s\n", addr, fc, write ? "write" : "read");
            printf("UAE  : %s %08x\n", okRef ? "ok   " : "fault", physRef);
            printf("Moira: %s %08x\n", okMoira ? "ok   " : "fault", physMoira);
            for (int k = 0; k < 0x10000; k++) {
                if (refMem[k] != moiraMem[k]) {
                    printf("Memory differs at %04x: UAE %02x Moira %02x\n", k, refMem[k], moiraMem[k]);
                    break;
                }
            }
            exit(1);
        }
    }
    passed(0, 0);
    printf("%11s           %ld translations, %ld faults\n", "", successes, faults);
}

void runMMUTranslate040(long round)
{
    static const int TABLE_SETUPS = 256;
    static const int ADDRESSES = 512;

    long successes = 0, faults = 0;
    resetClocks();
    printf("%11s MMU : XLAT ", selectedModel()); fflush(stdout);

    auto &mmu = moiracpu->mmu.mmu040;

    for (int i = 0; i < TABLE_SETUPS; i++) {

        if (i % 16 == 0) { printf("."); fflush(stdout); }

        fillWith040Descriptors();

        u32 urp = u32(portableRand()) & 0xFE00, srp = u32(portableRand()) & 0xFE00;
        u32 tc = 0x8000 | (portableRand() % 2 ? 0x4000 : 0);
        u32 itt0 = randomTT040(), itt1 = randomTT040(), dtt0 = randomTT040(), dtt1 = randomTT040();

        // Configure both MMUs
        setMMU040UAE(urp, srp, tc, itt0, itt1, dtt0, dtt1);
        mmu.urp = urp; mmu.srp = srp;
        mmu.setITT0(itt0); mmu.setITT1(itt1); mmu.setDTT0(dtt0); mmu.setDTT1(dtt1);
        mmu.setTC(tc);

        for (int j = 0; j < ADDRESSES; j++) {

            // Flush the ATCs regularly (UAE's ATC is 4-way set associative)
            if (j % 8 == 0) { flushATC040UAE(); mmu.atcFlush(); }

            u32 addr = u32(portableRand()) << (portableRand() % 17);
            if (portableRand() % 4 == 0) addr &= 0xFFFF;
            bool super = portableRand() % 2, data = portableRand() % 2, write = data && portableRand() % 2;
            u8 fc = (super ? 4 : 0) | (data ? 1 : 2);

            // PTEST
            if (portableRand() % 4 == 0) {

                bool pwrite = portableRand() % 2;
                u32 statusRef = ptest040UAE(addr, fc, pwrite);
                moiracpu->setSupervisorMode(super);
                sandbox.prepare(false, true);
                u32 statusMoira = mmu.test(addr, fc, pwrite);

                if (statusRef != statusMoira || memcmp(refMem, moiraMem, sizeof(refMem))) {

                    printf("\n\nPTEST MISMATCH FOUND\n\n");
                    printf("Setup %d.%d: TC: %04x  URP: %08x  SRP: %08x\n", i, j, tc, urp, srp);
                    printf("             ITT0: %08x  ITT1: %08x  DTT0: %08x  DTT1: %08x\n", itt0, itt1, dtt0, dtt1);
                    printf("PTEST%c: addr %08x  FC %d\n", pwrite ? 'W' : 'R', addr, fc);
                    printf("UAE  : MMUSR %08x\n", statusRef);
                    printf("Moira: MMUSR %08x\n", statusMoira);
                    endMMU040UAE();
                    exit(1);
                }
                continue;
            }

            // Translate with both MMUs
            u32 physRef = 0, physMoira = 0;
            bool okRef = translate040UAE(addr, super, data, write, &physRef);
            moiracpu->setSupervisorMode(super);
            sandbox.prepare(false, true);     // Don't record the descriptor updates
            auto translated = mmu.translate(addr, fc, write);
            bool okMoira = translated.has_value();
            if (okMoira) physMoira = *translated;

            (okRef ? successes : faults)++;
            bool equal = okRef == okMoira && (!okRef || physRef == physMoira) && !memcmp(refMem, moiraMem, sizeof(refMem));
            if (equal) continue;

            printf("\n\nTRANSLATION MISMATCH FOUND\n\n");
            printf("Setup %d.%d: TC: %04x  URP: %08x  SRP: %08x\n", i, j, tc, urp, srp);
            printf("             ITT0: %08x  ITT1: %08x  DTT0: %08x  DTT1: %08x\n", itt0, itt1, dtt0, dtt1);
            printf("Access: addr %08x  FC %d  %s\n", addr, fc, write ? "write" : "read");
            printf("UAE  : %s %08x\n", okRef ? "ok   " : "fault", physRef);
            printf("Moira: %s %08x\n", okMoira ? "ok   " : "fault", physMoira);
            for (int k = 0; k < 0x10000; k++) {
                if (refMem[k] != moiraMem[k]) {
                    printf("Memory differs at %04x: UAE %02x Moira %02x\n", k, refMem[k], moiraMem[k]);
                    break;
                }
            }
            endMMU040UAE();
            exit(1);
        }
    }
    endMMU040UAE();
    mmu.setTC(0);
    passed(0, 0);
    printf("%11s           %ld translations, %ld faults\n", "", successes, faults);
}

void runFPU(long round)
{
    // Musashi's FPU is not a reliable reference
    if (!ref().hasFpu()) {
        printf("%s FPU skipped (no reliable reference)\n", selectedModel());
        return;
    }

    static const int FPU_SETUPS = 4;
    Setup setup;
    resetClocks();

    if (fpuModel == FPUModel::BUILTIN)
        printf("%11s FPU : CORE ", selectedModel());
    else
        printf("%5s+%5s FPU : CORE ", selectedModel(), fpuModelName(fpuModel));
    fflush(stdout);

    for (int i = 0; i < FPU_SETUPS; i++) {

        setupTestEnvironment(setup, round * FPU_SETUPS + i);
        setupFpuEnvironment(setup);

        for (int op = 0xF200; op <= 0xF23F; op++) {

            if ((op % 8) == 0) { printf("."); fflush(stdout); }
            
            for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

                runSingleTest(setup, u16(op), u16(ext));
            }
        }
    }
    passed(moiraclk[0], refclk[0]);
}

void runDasmCPU(long round)
{
    static const int CPU_SETUPS = 32;

    Setup setup;
    resetClocks();

    printf("%11s CPU : DASM ", selectedModel()); fflush(stdout);

    for (int i = 0; i < CPU_SETUPS; i++) {

        printf("."); fflush(stdout);

        setupTestEnvironment(setup, round * CPU_SETUPS + i);

        for (int op = 0x0000; op <= 0xFFFF; op++) {

            if constexpr (SKIP_ILLEGAL) { if (isIllegalOpcode(u16(op))) continue; }

            // Skip instructions covered by runDasmFPU
            if (isFpuOpcode(u16(op))) continue;

            runDasmSingleTest(setup, u16(op));
        }
    }
    passed(moiraclk[1], refclk[1]);
}

void runDasmMMU(long round)
{
    if (!moiracpu->hasMMU()) return;

    static const int MMU_SETUPS = 4;
    Setup setup;
    resetClocks();

    printf("%11s MMU : DASM ", selectedModel()); fflush(stdout);

    for (int i = 0; i < MMU_SETUPS; i++) {

        setupTestEnvironment(setup, round * MMU_SETUPS + i);

        for (int op = 0xF000; op <= 0xF03F; op++) {

            if ((op % 8) == 0) { printf("."); fflush(stdout); }

            for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

                runDasmSingleTest(setup, u16(op), u16(ext));
            }
        }
    }
    passed(moiraclk[1], refclk[1]);
}

void runDasmFPU(long round)
{
    static const int FPU_SETUPS = 4;
    Setup setup;
    resetClocks();

    if (fpuModel == FPUModel::BUILTIN)
        printf("%11s FPU : DASM ", selectedModel());
    else
        printf("%5s+%5s FPU : DASM ", selectedModel(), fpuModelName(fpuModel));
    fflush(stdout);

    for (int i = 0; i < FPU_SETUPS; i++) {

        setupTestEnvironment(setup, round * FPU_SETUPS + i);

        for (int op = 0xF200; op <= 0xF23F; op++) {

            if ((op % 8) == 0) { printf("."); fflush(stdout); }

            for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

                runDasmSingleTest(setup, u16(op), u16(ext));
            }
        }
    }
    passed(moiraclk[1], refclk[1]);
}

bool misalignedRead = false;

bool isMmuOpcode(u16 op)
{
    return mmuCompare && (op & 0xFFC0) == 0xF000;
}

bool isFpuOpcode(u16 op)
{
    return (op & 0xFF00) == 0xF200 && moiracpu != nullptr && moiracpu->fpu.isPresent();
}

bool isIllegalOpcode(u16 op)
{
    return moiracpu->getInstrInfo(op).I == Instr::ILLEGAL;
}

void setupFpuEnvironment(Setup &s)
{
    // A pool of interesting values
    static const MFloat80 pool[] = {

        MFloat80::zero(), MFloat80::zero(true),
        MFloat80::one(), MFloat80::one(true),
        MFloat80::infinity(), MFloat80::infinity(true),
        MFloat80::quietNaN(), MFloat80::signalingNaN(),
        MFloat80::fromBits(0x4000, 0x8000000000000000ULL),   // 2.0
        MFloat80::fromBits(0x3FFE, 0x8000000000000000ULL),   // 0.5
        MFloat80::fromBits(0x4005, 0xA000000000000000ULL),   // 40.0
        MFloat80::fromBits(0xC005, 0xA000000000000000ULL),   // -40.0
        MFloat80::fromBits(0x0001, 0x8000000000000000ULL),   // Smallest normal
        MFloat80::fromBits(0x0000, 0x4000000000000000ULL),   // Denormal
        MFloat80::fromBits(0x7FFE, 0xFFFFFFFFFFFFFFFFULL),   // Largest finite
        MFloat80::fromBits(0x3FFF, 0x8000000000000001ULL),   // 1 + epsilon
    };
    const int poolSize = sizeof(pool) / sizeof(pool[0]);

    for (int i = 0; i < 8; i++) {

        u32 r = randomizer.rand();
        if (r % 3 == 0) {

            // Pick a special value
            s.fpr[i] = pool[(r >> 8) % poolSize];

        } else {

            // Create a random normalized number with a moderate exponent
            u64 manHi = randomizer.rand();
            u64 manLo = randomizer.rand();
            u64 man = (manHi << 32 | manLo) | (1ULL << 63);
            u16 exp = u16(0x3FFF - 40 + randomizer.rand() % 80);
            u16 sign = u16((randomizer.rand() & 1) << 15);
            s.fpr[i] = MFloat80::fromBits(sign | exp, man);
        }
    }

    // Control register: Random rounding mode and precision, no exception enables
    s.fpcr = randomizer.rand() & 0xF0;
    s.fpsr = randomizer.rand() & 0x0F000000;
    s.fpiar = randomizer.rand();

    // MMU registers (translation stays disabled initially)
    u64 crpHi = randomizer.rand(), crpLo = randomizer.rand();
    s.crp = crpHi << 32 | crpLo;
    u64 srpHi = randomizer.rand(), srpLo = randomizer.rand();
    s.srp = srpHi << 32 | srpLo;
    s.tc = randomizer.rand() & 0x7FFFFFFF;
    s.tt0 = randomizer.rand();
    s.tt1 = randomizer.rand();
    s.mmusr = randomizer.rand() & 0xEE47;
}

void passed(clock_t moira, clock_t ref)
{
    printf(" PASSED  (Moira: %.2fs  %s: %.2fs)\n",
           double(moira) / double(CLOCKS_PER_SEC),
           reference(),
           double(ref) / double(CLOCKS_PER_SEC));
}

void runSingleTest(Setup &s, u16 op, u16 ext)
{
    s.ext1 = ext;
    runSingleTest(s, op);
}

void runSingleTest(Setup &s, u16 op)
{
    lastSetup = &s;
    Result refResult, moiraResult;
    
    opcode = op;
    
    // Prepare the test case with the selected instruction
    setupTestInstruction(s, pc, op);
    
    // Reset the sandbox
    auto info = moiracpu->getInstrInfo(op);
    bool enable = doExec(op) && !skip(s) && !skipAccessCheck(op);
    bool ignore = (info.I == Instr::CLR && info.S == Long) ||
    (info.I == Instr::CHK && cpuModel != CPUModel::M68000);
    sandbox.prepare(enable, ignore);
    misalignedRead = false;

    // Reset all CPUs
    ref().reset(s);
    resetMoira(s);

    if (VERBOSE) {
        disassembleBinutils(refResult);
        printf("$%04x ($%04x): %s\n", s.pc, op, refResult.dasmBinutils);
    }

    // Run the reference CPU
    runReference(s, refResult);
    // Skip the case if the reference raised an address error (not covered by this test)
    if (ref().aborted()) return;
    refclk[0] += refResult.elapsed[0];

    // Run Moira
    runMoira(s, moiraResult);
    moiraclk[0] += moiraResult.elapsed[0];

    // Compare
    compare(s, refResult, moiraResult);
}

void runDasmSingleTest(Setup &s, u16 op, u16 ext)
{
    s.ext1 = ext;
    runDasmSingleTest(s, op);
}

void runDasmSingleTest(Setup &s, u16 op)
{
    if (!doDasm(op)) return;

    Result refResult, moiraResult;

    // Prepare the test case with the selected instruction
    setupTestInstruction(s, pc, op);

    // Run both disassemblers
    disassembleReference(s, refResult);
    disassembleMoira(s, moiraResult);
    refclk[1] += refResult.elapsed[1];
    moiraclk[1] += moiraResult.elapsed[1];

    // Compare
    if (!compareDasm(refResult, moiraResult)) {

        printf("\nDISASSEMBLER MISMATCH FOUND\n");
        dumpDasm(s, refResult, moiraResult);
        bugReport();
    }
}

// Executes the instruction on the reference CPU (Musashi or UAE)
void runReference(Setup &s, Result &r)
{
    r.oldpc = s.pc;
    r.opcode = get16(refMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;

    if (!doExec(r.opcode)) return;

    ref().execute(r);
}

// Executes the instruction on Moira
void runMoira(Setup &s, Result &r)
{
    r.oldpc = moiracpu->getPC();
    r.opcode = get16(moiraMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;

    if (doExec(r.opcode)) {

        u32 pc = moiracpu->getPC();
        u16 op = get16(moiraMem, pc);

        int64_t cycles = moiracpu->getClock();

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

// Disassembles the instruction with the reference disassemblers (Binutils plus Musashi or UAE)
void disassembleReference(Setup &s, Result &r)
{
    r.oldpc = s.pc;
    r.opcode = get16(refMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;
    r.dasmMusashi[0] = r.dasmUAE[0] = 0;
    r.dasmCntMusashi = r.dasmCntUAE = 0;

    disassembleBinutils(r);

    if (dasmMusashi) disassembleMusashi(s, r);
    if (dasmUAE) disassembleUAE(s, r);
}

// Disassembles the instruction with Moira in all supported syntax flavors
void disassembleMoira(Setup &s, Result &r)
{
    r.oldpc = s.pc;
    r.opcode = get16(moiraMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;
    r.dasmMoiraUAE[0] = 0;
    r.dasmCntMusashi = r.dasmCntMoiraUAE = 0;

    // Disassemble the instruction in GNU format (binutils)
#ifdef MOTOROLA
    moiracpu->setDasmSyntax(Syntax::GNU);
#else
    moiracpu->setDasmSyntax(Syntax::GNU_MIT);
#endif
    moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 10, .plainZero = true });
    r.dasmCntBinutils = moiracpu->disassemble(r.dasmBinutils, r.oldpc);

    // Disassemble the instruction in Moira's own syntax
    r.dasmMoiraSyn[0] = 0;
    moiracpu->setDasmSyntax(DASM_MIT ? Syntax::MOIRA_MIT : Syntax::MOIRA);
    moiracpu->setDasmLetterCase(LetterCase::MIXED_CASE);
    moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
    r.dasmCntMoiraSyn = moiracpu->disassemble(r.dasmMoiraSyn, r.oldpc);

    // Disassemble the instruction in Musashi format
    if (dasmMusashi) {
        moiracpu->setDasmSyntax(Syntax::MUSASHI);
        moiracpu->setDasmLetterCase(LetterCase::MIXED_CASE);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        clock_t elapsed = clock();
        r.dasmCntMusashi = moiracpu->disassemble(r.dasmMusashi, r.oldpc);
        r.elapsed[1] = clock() - elapsed;
    }

    // Disassemble the instruction in UAE format
    if (dasmUAE) {
        moiracpu->setDasmSyntax(Syntax::UAE);
        moiracpu->setDasmLetterCase(LetterCase::LOWER_CASE);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        clock_t elapsed = clock();
        r.dasmCntMoiraUAE = moiracpu->disassemble(r.dasmMoiraUAE, r.oldpc);
        r.elapsed[1] = clock() - elapsed;
    }
}

bool skipAccessCheck(u16 op)
{
    return ref().skipAccessCheck(op);
}

bool skip(const Setup &s)
{
    return ref().skipSetup(s);
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

    for (int i = 0; i < 8; i++) r.fpr[i] = moiracpu->fpu.fpr[i];
    r.fpcr = moiracpu->fpu.getFPCR();
    r.fpsr = moiracpu->fpu.getFPSR();
    r.fpiar = moiracpu->fpu.getFPIAR();

    r.crp = moiracpu->mmu.mmu030.crp;
    r.srp = moiracpu->mmu.mmu030.srp;
    r.tc = moiracpu->mmu.mmu030.tc;
    r.tt0 = moiracpu->mmu.mmu030.tt0;
    r.tt1 = moiracpu->mmu.mmu030.tt1;
    r.mmusr = moiracpu->mmu.mmu030.mmusr;
}

void dumpDasm(Setup &s, Result &rRef, Result &rMoira)
{
    if (DASM_STAGES) {
        printf("\nMoira syntax:         [%d] '%s'\n", rMoira.dasmCntMoiraSyn, rMoira.dasmMoiraSyn);
    }
    if (dasmUAE) {
        printf("\nInstruction:      UAE: [%d] '%s'", rRef.dasmCntUAE, rRef.dasmUAE);
        printf("\n                Moira: [%d] '%s'\n", rMoira.dasmCntMoiraUAE, rMoira.dasmMoiraUAE);
    }
    if (dasmMusashi) {
        printf("\nInstruction:  Musashi: [%d] '%s'", rRef.dasmCntMusashi, rRef.dasmMusashi);
        printf("\n                Moira: [%d] '%s'\n", rMoira.dasmCntMusashi, rMoira.dasmMusashi);
    }
    printf("\n             Binutils: [%d] '%s'", rRef.dasmCntBinutils, rRef.dasmBinutils);
    printf("\n                Moira: [%d] '%s'\n", rMoira.dasmCntBinutils, rMoira.dasmBinutils);
    printf("\n");
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

// Decides whether the results of a test case are excluded from the comparison
bool skipCompare(const Setup &s, const Result &rRef, const Result &rMoira)
{
    // Cases that can be recognized by looking at the setup
    if (skip(s)) return true;

    // Cases that can only be recognized by looking at the results
    // Cases that the reference excludes
    if (ref().skipResult(s, rRef, rMoira)) return true;

    // MMU opcodes: Compare only instructions that execute on both sides. Exception handling of
    // invalid extension words differs (UAE raises a privilege violation for some of them).
    if (isMmuOpcode(s.opcode)) {
        auto completed = [](const Result &r) { return r.pc > 0x1000 && r.pc <= 0x1010; };
        if (!completed(rRef) || !completed(rMoira)) return true;

        // UAE fetches the extension words of indexed addressing modes incorrectly
        if ((s.opcode >> 3 & 7) == 6) return true;

        // UAE reads misaligned long words incorrectly
        if (misalignedRead) return true;

        // UAE applies the (An)+ and -(An) side effect to PFLUSH and PLOAD
        if ((s.opcode >> 3 & 7) == 3 || (s.opcode >> 3 & 7) == 4) return true;

        // PFLUSH without effective address: UAE still skips the extension words of the EA field
        if ((s.ext1 >> 13) == 1 && ((s.ext1 >> 8 & 31) == 4 || (s.ext1 >> 8 & 31) == 0x10) && (s.opcode & 0x3F) > 0x17) return true;

        // Table walks with translation disabled are meaningless (covered by the XLAT stage)
        u16 ext = s.ext1, mode = ext >> 8 & 31;
        if (!(s.tc & 0x80000000) && ((ext >> 13) == 4 || ((ext >> 13) == 1 && (mode == 0 || mode == 2)))) return true;

        // PMOVE to TC with an invalid configuration raises the MMU configuration exception, which Moira doesn't emulate
        auto invalidTC = [](u32 tc) {
            if (!(tc & 0x80000000)) return false;
            int ps = tc >> 20 & 15, is = tc >> 16 & 15, sum = ps + is;
            for (int i = 12; i >= 0; i -= 4) { int b = tc >> i & 15; if (!b) break; sum += b; }
            return ps < 8 || sum != 32;
        };
        if (invalidTC(rRef.tc)) return true;
    }

    return false;
}

void compare(Setup &s, Result &rRef, Result &rMoira)
{
    bool error = false;

    if (doExec(s.opcode)) {

        if (!skipCompare(s, rRef, rMoira)) {

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
                    printMemoryDiff();
                    error = true;
                }
            }
            if (mmuCompare && !compareMMU(rRef, rMoira)) {
                printf("\nMMU REGISTER MISMATCH FOUND");
                error = true;
            }
            if (sandbox.getErrors()) {
                printf("\nSANDBOX ERRORS REPORTED");
                if (lastSetup) {
                    printf("\nMemory at $0 (setup / reference / Moira):\n");
                    for (int q = 0; q < 16; q += 4)
                        printf("  %02x %02x %02x %02x  /  %02x %02x %02x %02x  /  %02x %02x %02x %02x\n",
                               lastSetup->mem[q], lastSetup->mem[q+1], lastSetup->mem[q+2], lastSetup->mem[q+3],
                               refMem[q], refMem[q+1], refMem[q+2], refMem[q+3],
                               moiraMem[q], moiraMem[q+1], moiraMem[q+2], moiraMem[q+3]);
                }
                error = true;
            }
        }
    }

    if (error) {

        printf("\n");

        // Disassemble the instruction again to get proper debug output
        memcpy(refMem, s.mem, sizeof(refMem));
        memcpy(moiraMem, s.mem, sizeof(moiraMem));
        disassembleReference(s, rRef);
        disassembleMoira(s, rMoira);
        dumpDasm(s, rRef, rMoira);

        printf("Setup:   ");
        dumpSetup(s);

        printf("%-7s: ", reference());
        dumpResult(rRef);

        printf("Moira:   ");
        dumpResult(rMoira);

        if (isMmuOpcode(s.opcode)) {
            printf("CRP:   %016llx / %016llx / %016llx (setup / %s / Moira)\n",
                   (unsigned long long)s.crp, (unsigned long long)rRef.crp, (unsigned long long)rMoira.crp, reference());
            printf("SRP:   %016llx / %016llx / %016llx\n",
                   (unsigned long long)s.srp, (unsigned long long)rRef.srp, (unsigned long long)rMoira.srp);
            printf("TC:    %08x / %08x / %08x\n", s.tc, rRef.tc, rMoira.tc);
            printf("TT0:   %08x / %08x / %08x\n", s.tt0, rRef.tt0, rMoira.tt0);
            printf("TT1:   %08x / %08x / %08x\n", s.tt1, rRef.tt1, rMoira.tt1);
            printf("MMUSR: %04x / %04x / %04x\n\n", s.mmusr & 0xFFFF, rRef.mmusr & 0xFFFF, rMoira.mmusr & 0xFFFF);
        }

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
    if (strstr(uae, "(68020+)") || strstr(uae, "(68040+)")) return true;   // Not available on the selected model
    if (starts(uae, "fmovecr") || strstr(uae, "#+") || strstr(uae, "#-") || (starts(uae, "fmove.p") && strstr(uae, ",#$")) || strstr(uae, "/#$")) {
        return true;   // Floating-point constants are printed in a different notation
    }   // Addressing mode not available on the selected model
    if (starts(moira, "dc.w") || isdigit((unsigned char)moira[0])) return true;

    return strcmp(uae, moira) == 0;
}

static bool noUAEDasmForCoprocessor(u16 opcode)
{
    switch (cpuModel) {
        case CPUModel::M68EC020: case CPUModel::M68EC030: case CPUModel::M68EC040: case CPUModel::M68LC040:
            return opcode >= 0xF000;
        default:
            return false;
    }
}

bool compareDasm(Result &rRef, Result &rMoira)
{
    auto I = moiracpu->getInstrInfo(rRef.opcode).I;

    bool skipMusashi = !dasmMusashi || rRef.opcode >= 0xF000;
    bool skipBinutils = false;

    switch (cpuModel) {

        case CPUModel::M68010:

            skipBinutils |= I == Instr::CMPI;
            break;
            
        case CPUModel::M68EC020:
        case CPUModel::M68EC030:
        case CPUModel::M68EC040:
        case CPUModel::M68LC040:

            skipBinutils |= rRef.opcode >= 0xF000;
            break;

        default:
            break;
    }

    // Binutils decodes FPU instructions only for the 68040 (ENABLE_M68881 is off)
    skipBinutils |= (rRef.opcode & 0xFE00) == 0xF200 && moiracpu->has6888x();

    // Compare with the Musashi disassembler
    if (!skipMusashi) {

        if (rRef.dasmCntMusashi != rMoira.dasmCntMusashi) return false;
        if (strcmp(rRef.dasmMusashi, rMoira.dasmMusashi) != 0) return false;
    }

    // Compare with UAE's disassembler (UAE decodes coprocessor opcodes regardless of the model)
    if (dasmUAE && !noUAEDasmForCoprocessor(rRef.opcode) && !dasmEqualUAE(rRef.dasmUAE, rMoira.dasmMoiraUAE)) return false;

    // Moira's own syntax has no reference. Make sure that it produces some output. (The
    // length may differ from the GNU flavor, because the latter mimics Binutils' view on
    // instructions that are not available on the selected CPU.)
    if (DASM_STAGES) {
        if (!rMoira.dasmMoiraSyn[0] || rMoira.dasmCntMoiraSyn <= 0) return false;
    }

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
    return ref().equalPC(rRef, rMoira);
}

bool compareSR(Result &rRef, Result &rMoira)
{
    assert(rRef.opcode == rMoira.opcode);
    Instr instr = moiracpu->getInstrInfo(rRef.opcode).I;
    if (instr == Instr::DIVU || instr == Instr::DIVS) {

        // The reference CPU differs (and is likely wrong). Ignore it
        return true;
    }

    // Compare all bits that are defined
    u32 mask = ref().srMask(instr);
    return (rRef.sr & mask) == (rMoira.sr & mask);
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

bool compareFPR(Result &rRef, Result &rMoira) { return ref().equalFPR(rRef, rMoira); }
bool compareFPCR(Result &rRef, Result &rMoira) { return ref().equalFPCR(rRef, rMoira); }
bool compareFPSR(Result &rRef, Result &rMoira) { return ref().equalFPSR(rRef, rMoira); }
bool compareFPIAR(Result &rRef, Result &rMoira) { return ref().equalFPIAR(rRef, rMoira); }
bool compareMMU(Result &rRef, Result &rMoira)
{
    return rRef.crp == rMoira.crp && rRef.srp == rMoira.srp &&
           rRef.tc == rMoira.tc && rRef.tt0 == rMoira.tt0 && rRef.tt1 == rMoira.tt1 &&
           (rRef.mmusr & 0xFFFF) == (rMoira.mmusr & 0xFFFF);
}
bool compareMemory() { return memcmp(refMem, moiraMem, sizeof(refMem)) == 0; }

// Prints the first differing bytes of the two memory images
void printMemoryDiff()
{
    int shown = 0;
    for (size_t i = 0; i < sizeof(refMem) && shown < 16; i++) {
        if (refMem[i] != moiraMem[i]) {
            printf("\n  Memory[%05zx]: %s %02x  Moira %02x", i, ref().name(), refMem[i], moiraMem[i]);
            shown++;
        }
    }
}

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

    // Some references are only reliable for certain models
    if (ref().ignoreCycles(cpuModel)) return true;

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
        if (cpuModel != CPUModel::M68000 && cpuModel != CPUModel::M68010) return true;
    }

    // WHY DO WE IGNORE THESE?
    if (cpuModel == CPUModel::M68010) {

        if (I == Instr::CLR && S == Byte && M == Mode::AL) return true;
        if (I == Instr::CLR && S == Word && M == Mode::AL) return true;
        if (I == Instr::TRAPV) return true;
    }

    return rRef.cycles == rMoira.cycles;
}

// Writes the setup of the failed test to the file named in RUNNER_DUMP. Replay it with RUNNER_REPLAY.
static void dumpFailedSetup()
{
    const char *name = getenv("RUNNER_DUMP");
    if (!name || !lastSetup) return;
    if (FILE *f = fopen(name, "wb")) {
        int model = int(cpuModel);
        fwrite(&model, sizeof(model), 1, f);
        fwrite(lastSetup, sizeof(Setup), 1, f);
        fclose(f);
        printf("Setup written to %s\n", name);
    }
}

void bugReport()
{
    dumpFailedSetup();
    printf("Please report the bug at https://github.com/dirkwhoffmann/Moira\n");
    printf("Thanks you!\n\n");
    exit(1);
}

} // namespace runner
