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
#include <cstring>
#include <algorithm>
#include <ctime>
#include <string>
#include <strings.h>

namespace runner {

using namespace moira;

// The reference core (determined by the compatibility mode of the tested Moira)
static constexpr bool useUAE     = (compat == Compatibility::UAE);
static constexpr bool useMusashi = (compat == Compatibility::MUSASHI);

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
        for (auto &m : modelTable) result.push_back(m.model);
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

void selectModel(CPUModel model)
{
    cpuModel = model;

    // Only the 68040 has a built-in FPU. External FPUs are installed with selectFpu.
    fpuModel = (cpuModel == CPUModel::M68040) ? FPUModel::BUILTIN : FPUModel::NONE;

    if (useMusashi) setupMusashi(cpuModel);
    if (useUAE) setupUAE(cpuModel, fpuModel);
    setupBinutils(cpuModel);
    setupMoira();

    // Configure the sandbox
    sandbox.checkCycles = useUAE && cpuModel == CPUModel::M68000;
    sandbox.addrMask = (cpuModel == CPUModel::M68EC020) ? 0xFFFFFF : 0xFFFFFFFF;
    sandbox.byteGranular = useUAE && cpuModel != CPUModel::M68000 && cpuModel != CPUModel::M68010;
}

void setupMoira()
{
    moiracpu->setModel(cpuModel);
    moiracpu->setFpuModel(fpuModel);
}

void selectFpu(FPUModel fpu)
{
    fpuModel = fpu;

    if (useUAE) setupUAE(cpuModel, fpuModel);
    setupMoira();
}

void setupTestEnvironment(Setup &s, long round)
{
    s.round = round;
    s.supervisor = round % 2;
    s.ccr = u8(rand());
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

    moiracpu->mmu.crp = s.crp;
    moiracpu->mmu.srp = s.srp;
    moiracpu->mmu.tc = s.tc;
    moiracpu->updateMmuFlag();
    moiracpu->mmu.tt0 = s.tt0;
    moiracpu->mmu.tt1 = s.tt1;
    moiracpu->mmu.mmusr = s.mmusr;
    for (auto &e : moiracpu->mmu.atc) e.valid = false;
}

void run(unsigned seed, long rounds, const std::vector<CPUModel> &cpus, const std::vector<FPUModel> &fpus)
{
    srand(seed);

    if (!moiracpu) moiracpu = new TestCPU();

    for (long i = 1; i <= rounds; i++) {

        printf("\nRound %ld:\n\n", i);

        for (CPUModel cpu : cpus) {

            // Select the CPU core
            selectModel(cpu);

            // Execute tests
            if constexpr (CHECK_CPU) runCPU(i);
            if constexpr (CHECK_MMU) { runMMU(i); if (useUAE && cpu == CPUModel::M68030) runMMUTranslate(i); }

            // Test the disassembler
            if constexpr (CHECK_DASM) {
                runDasmCPU(i);
                runDasmMMU(i);
            }

            // Run the FPU tests for all FPUs that can be combined with this CPU
            if constexpr (CHECK_FPU) {

                for (FPUModel fpu : fpus) {

                    if (!isCompatible(cpu, fpu)) continue;
                    selectFpu(fpu);
                    runFPU(i);
                    if constexpr (CHECK_DASM) runDasmFPU(i);

                    // Without a reference FPU, there is nothing more to report
                    if (useMusashi) break;
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
        int kind = rand() % 8;

        if (kind < 4) {
            // Short descriptor
            w = (u32(rand()) & 0xFFFFFFF0) | (rand() % 4) | (rand() & 0xC);
            if (rand() % 6 == 0) w &= ~3u;
        } else if (kind < 6) {
            // Upper half of a long descriptor (limit, flags, DT)
            u32 limit = (rand() % 3 == 0) ? 0x7FFF : u32(rand() % 70);
            w = (rand() % 6 == 0 ? 0x80000000 : 0) | limit << 16 | (rand() & 0x0100) | (rand() & 0x1C) | (rand() % 4);
            if (rand() % 6 == 0) w &= ~3u;
        } else {
            // Lower half of a long descriptor (address)
            w = (rand() & 0xFFF0) | (rand() % 3 == 0 ? u32(rand()) << 16 : 0);
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
    int ps = 8 + rand() % 8;
    int is = rand() % 3 == 0 ? rand() % 6 : 0;
    int rest = 32 - ps - is;

    do {
        t.levels = 2 + rand() % 3;
        int left = rest;
        for (int i = 0; i < 4; i++) t.bits[i] = 0;
        for (int i = 0; i < t.levels; i++) {
            int minLeft = t.levels - i - 1;
            int maxBits = std::min(6, left - minLeft);
            int b = (i == t.levels - 1) ? left : 1 + rand() % maxBits;
            t.bits[i] = b;
            left -= b;
        }
        if (left != 0) t.levels = 0;
    } while (t.levels == 0 || t.bits[t.levels - 1] > 6 || t.bits[t.levels - 1] < 1);

    t.tc = 0x80000000 | (rand() % 2 ? 0x02000000 : 0) | (rand() % 4 == 0 ? 0x01000000 : 0) | ps << 20 | is << 16
         | t.bits[0] << 12 | t.bits[1] << 8 | t.bits[2] << 4 | t.bits[3];

    auto rootPointer = [&]() {
        u64 limit = rand() % 3 == 0 ? 0x7FFF : rand() % 70;
        u64 lu = rand() % 6 == 0;
        u64 dt = 1 + rand() % 3;
        return lu << 63 | limit << 48 | dt << 32 | u64(rand() & 0xFFF0);
    };
    auto transparentTranslation = [&]() {
        if (rand() % 2) return u32(0);
        u32 mask = (0xFF00 >> (rand() % 9)) & 0xFF;
        return u32(rand() & 0xFF) << 24 | mask << 16 | 0x8000 | u32(rand() % 2) << 10
             | u32(rand() % 2) << 9 | u32(rand() % 2) << 8 | u32(rand() % 8) << 4 | u32(rand() % 8);
    };
    t.tt0 = transparentTranslation();
    t.tt1 = transparentTranslation();
    t.crp = rootPointer();
    t.srp = rootPointer();
    return t;
}

}

static bool mmuCompare = false;

void runMMU(long round)
{
    if (!moiracpu->hasMMU()) return;

    // Let UAE execute MMU instructions and compare the MMU registers
    mmuCompare = useUAE && cpuModel == CPUModel::M68030;
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
        moiracpu->mmu.crp = t.crp;
        moiracpu->mmu.srp = t.srp;
        moiracpu->mmu.tc = t.tc;
        moiracpu->mmu.tt0 = t.tt0;
        moiracpu->mmu.tt1 = t.tt1;

        u32 lastAddr = 0; u8 lastFC = 1;

        for (int j = 0; j < ADDRESSES; j++) {

            // Flush the ATCs regularly. This keeps the number of cached pages below
            // the ATC size, so that the (pseudo-LRU) replacement strategy doesn't matter.
            if (j % 16 == 0) {
                flushATCUAE();
                for (auto &e : moiracpu->mmu.atc) e.valid = false;
            }

            // Pick an address that is likely to be covered by the tables
            u32 addr = u32(rand()) << (rand() % 17);
            if (rand() % 4 == 0) addr &= 0xFFFF;
            bool super = rand() % 2, data = rand() % 2, write = data && rand() % 2;
            u8 fc = (super ? 4 : 0) | (data ? 1 : 2);

            // UAE's ATC lookup is undefined for write accesses if the root pointer
            // terminates the walk, because the M bit has nowhere to live.
            u64 rp = (super && (t.tc & 0x02000000)) ? t.srp : t.crp;
            if (write && ((rp >> 32) & 3) == 1) write = false;   // (also with FCL: FC lookup is skipped)

            // Run PTEST on both MMUs (no side effects)
            {
                u32 level = rand() % 6;
                u32 paddr = addr, pfc = fc;
                if (level == 0) {

                    // UAE compares the unmasked address with the ATC entries. Use the
                    // page of an earlier access to get ATC hits.
                    paddr = (rand() % 4 ? lastAddr : addr) & ~((1u << (t.tc >> 20 & 0xF)) - 1);
                    if (rand() % 4) pfc = lastFC;
                }
                bool pwrite = rand() % 2;
                u32 statusRef = 0, descrRef = 0, descrMoira = 0;
                ptestUAE(paddr, pfc, pwrite, level, &statusRef, &descrRef);
                moiracpu->setSupervisorMode(super);
                sandbox.prepare(false, true);
                u32 statusMoira = moiracpu->mmu.test(paddr, pfc, pwrite, level, descrMoira);

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

void runFPU(long round)
{
    // Musashi's FPU is not a reliable reference
    if (useMusashi) {
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
            u64 man = (u64(randomizer.rand()) << 32 | randomizer.rand()) | (1ULL << 63);
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
    s.crp = u64(randomizer.rand()) << 32 | randomizer.rand();
    s.srp = u64(randomizer.rand()) << 32 | randomizer.rand();
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
    if (useMusashi) resetMusashi(s);
    if (useUAE) resetUAE(s);
    resetMoira(s);

    if (VERBOSE) {
        disassembleBinutils(refResult);
        printf("$%04x ($%04x): %s\n", s.pc, op, refResult.dasmBinutils);
    }

    // Run the reference CPU
    runReference(s, refResult);
    // Moira is configured without address error emulation (MOIRA_EMULATE_ADDRESS_ERROR)
    if (useUAE && (lastExceptionUAE() == 3) && !MOIRA_EMULATE_ADDRESS_ERROR) return;
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

    switch (compat) {

        case Compatibility::MUSASHI: executeMusashi(r); break;
        case Compatibility::UAE:     executeUAE(r); break;

        default:
            assert(false);
    }
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

    switch (compat) {

        case Compatibility::MUSASHI: disassembleMusashi(s, r); break;
        case Compatibility::UAE:     disassembleUAE(s, r); break;

        default:
            assert(false);
    }
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

    // Disassemble the instruction in Musashi format
    if (useMusashi) {
        moiracpu->setDasmSyntax(Syntax::MUSASHI);
        moiracpu->setDasmLetterCase(LetterCase::MIXED_CASE);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        clock_t elapsed = clock();
        r.dasmCntMusashi = moiracpu->disassemble(r.dasmMusashi, r.oldpc);
        r.elapsed[1] = clock() - elapsed;
    }

    // Disassemble the instruction in UAE format
    if (useUAE) {
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
    // UAE only writes back the bytes of a bit field that are affected,
    // whereas Moira writes back the whole long word. The final memory
    // contents are still compared.
    if (!useUAE) return false;
    if ((op & 0xF8C0) == 0xE8C0) return true;

    // FPU opcodes may be legal for one side only (final memory contents are still compared)
    if ((op & 0xFE00) == 0xF200) return true;

    // The same applies to MMU opcodes
    if (isMmuOpcode(op)) return true;

    // N and V are undefined for CHK2, which shows up in the exception frame
    auto instr = moiracpu->getInstrInfo(op).I;
    return instr == Instr::CHK2 || instr == Instr::CMP2;
}

bool skip(const Setup &s)
{
    bool result;

    u16 op = s.opcode;
    Instr instr = moiracpu->getInstrInfo(op).I;

    if (useUAE) {

        // The FPCR precision value 3 is reserved. UAE and Moira round tiny results differently in this mode.
        if ((op & 0xFE00) == 0xF200 && (s.fpcr & 0xC0) == 0xC0) return true;

        // UAE doesn't implement the module instructions
        if (instr == Instr::CALLM || instr == Instr::RTM) return true;

        // RTE processes random stack frames (format errors, bus error frames)
        if (instr == Instr::RTE && cpuModel != CPUModel::M68000 && cpuModel != CPUModel::M68010) return true;

        // The 68010 supports the bus error stack frame (format $8), which Moira doesn't process.
        // The format word sits at offset 6 of the frame (the supervisor stack pointer is $2000).
        if (instr == Instr::RTE && cpuModel == CPUModel::M68010 && (get16(refMem, 0x2006) >> 12) == 8) return true;

        // LINK.l A7 pushes A7-4 on the 68040 (Moira's 040 models run on the 68020 core)
        if (instr == Instr::LINK && (op & 7) == 7 && cpuModel >= CPUModel::M68EC040) return true;

        // Zero divide uses a different stack frame on the 68040 (Moira's 040 models run on the 68020 core)
        if ((instr == Instr::DIVL || instr == Instr::DIVU || instr == Instr::DIVS) &&
            cpuModel >= CPUModel::M68EC040) return true;

        // UAE emulates FPU instructions that the 68040 traps (Moira follows real hardware)
        if ((op & 0xFFC0) == 0xF200 && cpuModel == CPUModel::M68040) {
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

        if (cpuModel >= CPUModel::M68EC020 && indexed) {
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

    if (cpuModel != CPUModel::M68000) {

        result |= instr == Instr::RTE;
    }

    return result;
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

    r.crp = moiracpu->mmu.crp;
    r.srp = moiracpu->mmu.srp;
    r.tc = moiracpu->mmu.tc;
    r.tt0 = moiracpu->mmu.tt0;
    r.tt1 = moiracpu->mmu.tt1;
    r.mmusr = moiracpu->mmu.mmusr;
}

void dumpDasm(Setup &s, Result &rRef, Result &rMoira)
{
    if (useUAE) {
        printf("\nInstruction:      UAE: [%d] '%s'", rRef.dasmCntUAE, rRef.dasmUAE);
        printf("\n                Moira: [%d] '%s'\n", rMoira.dasmCntMoiraUAE, rMoira.dasmMoiraUAE);
    }
    if (useMusashi) {
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
    // FPU opcodes where only one side raises line F (reserved bits set in the opcode or extension word)
    if (useUAE && (s.opcode & 0xFE00) == 0xF200 && ((rRef.pc >> 12) == 1) != ((rMoira.pc >> 12) == 1)) return true;

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

    bool skipMusashi = !useMusashi || rRef.opcode >= 0xF000;
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
    if (useUAE && cpuModel >= CPUModel::M68EC040 &&
        (instr == Instr::ABCD || instr == Instr::SBCD || instr == Instr::NBCD)) {
        return (rRef.sr & ~0x0A) == (rMoira.sr & ~0x0A);
    }

    // Z, V and C are undefined for CHK on the 68020 and above
    if (useUAE && instr == Instr::CHK && cpuModel != CPUModel::M68000 && cpuModel != CPUModel::M68010) {
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

// Returns the command field of an FPU instruction that operates on the registers (or -1)
static int fpuCommand(u16 op)
{
    if ((op & 0xFFC0) != 0xF200) return -1;

    u16 ext = get16(refMem, pc + 2);
    u16 cod = ext >> 13;
    return (cod == 0 || cod == 2) ? (ext & 0x7F) : -1;
}

// Returns true if the instruction is a sine, cosine, or tangent function. The reference
// reduces the argument with a pi of limited precision, so its results degrade for large
// arguments (Moira reduces exactly). FpuRefTests checks the accuracy.
static bool isTrigonometric(u16 op)
{
    int cmd = fpuCommand(op);
    return cmd == 0x0E || cmd == 0x0F || cmd == 0x1D || (cmd >= 0x30 && cmd <= 0x37);
}

// Returns true if the instruction is a transcendental function. Moira's implementation
// differs from the reference in the last bits, so these results are compared with a tolerance.
static bool isTranscendental(u16 op)
{
    switch (fpuCommand(op)) {

        case 0x02: case 0x06: case 0x08: case 0x09: case 0x0A: case 0x0C: case 0x0D:
        case 0x10: case 0x11: case 0x12: case 0x14: case 0x15: case 0x16: case 0x19:
        case 0x1C:
            return true;

        default:
            return isTrigonometric(op);
    }
}

static bool identicalFPR(Result &rRef, Result &rMoira)
{
    for (int i = 0; i < 8; i++) if (!rRef.fpr[i].identical(rMoira.fpr[i])) return false;
    return true;
}

// Maps a finite value to an integer such that adjacent values differ by 1
static __int128 ulpIndex(const MFloat80 &value)
{
    __int128 e = value.biasedExponent();
    u64 man = value.mantissa();
    __int128 pos = (e << 63) + (e ? __int128(man & ~(1ULL << 63)) : __int128(man >> 1));
    return value.sign() ? -pos : pos;
}

bool compareFPR(Result &rRef, Result &rMoira)
{
    // The reference loses up to 13 bits for small arguments of the hyperbolic functions.
    // With directed rounding, the results can also differ by an ulp of the selected precision.
    __int128 ulpTolerance = 1 << 14;
    if ((rRef.fpcr & 0xC0) == 0x40) ulpTolerance = __int128(1) << 40;     // Single precision

    for (int i = 0; i < 8; i++) {

        if (rRef.fpr[i].identical(rMoira.fpr[i])) continue;

        // On underflow, UAE returns a zero significand with the minimum exponent of the
        // format instead of a true zero (same exemption as in FpuRefTests)
        if (useUAE) {
            auto &ref = rRef.fpr[i], &moira = rMoira.fpr[i];
            bool pseudoZero = ref.mantissa() == 0 && ref.biasedExponent() != 0 && ref.biasedExponent() != 0x7FFF;
            if (pseudoZero && moira.isZero() && moira.sign() == ref.sign()) continue;

            // The transcendental functions are accurate to only a few ulps
            if (isTranscendental(rRef.opcode) && !ref.isNaN() && !ref.isInfinite() && !moira.isNaN() && !moira.isInfinite()) {
                __int128 diff = ulpIndex(ref) - ulpIndex(moira);
                if ((diff < 0 ? -diff : diff) <= ulpTolerance) continue;
                if (isTrigonometric(rRef.opcode)) continue;
            }
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

    // If the reference degrades, the result may even differ in sign
    if (useUAE && isTrigonometric(rRef.opcode) && !identicalFPR(rRef, rMoira)) {
        ref &= ~0x0C000000; moira &= ~0x0C000000;
    }

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
            printf("\n  Memory[%05zx]: UAE %02x  Moira %02x", i, refMem[i], moiraMem[i]);
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

    /* UAE's cores are only used as a timing reference for the 68000. The
     * 68010 timing is partially copied from the 68000, and the 68020+ cores
     * aren't cycle-exact.
     */
    if (useUAE && cpuModel != CPUModel::M68000) return true;

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

void bugReport()
{
    printf("Please report the bug at https://github.com/dirkwhoffmann/Moira\n");
    printf("Thanks you!\n\n");
    exit(1);
}

} // namespace runner
