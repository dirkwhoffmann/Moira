// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "ExecRunner.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <string>

namespace runner {

using namespace moira;

const char *ExecRunner::description() const
{
    return "Runs random instructions on Moira and on a reference CPU and compares\n"
           "registers, memory and cycle counts after each instruction.\n";
}

void ExecRunner::printRanges() const
{
    common::printSetting("Exec range", "opcodes up to $EFFF plus FPU and MMU opcodes");
}

void ExecRunner::setupReferences()
{
    ref().setup(cpuModel, fpuModel);
}

void ExecRunner::selectFpuReferences()
{
    ref().selectFpu(cpuModel, fpuModel);
}

void ExecRunner::modelSelected()
{
    sandbox.checkCycles = ref().checksCycles(cpuModel);
    sandbox.byteGranular = ref().byteGranular(cpuModel);
}

bool ExecRunner::replay()
{
    // Replay a single test that has been dumped with RUNNER_DUMP
    const char *name = getenv("RUNNER_REPLAY");
    if (!name) return false;

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
    return true;
}

void ExecRunner::runStages(long round, const std::vector<FPUModel> &fpus)
{
    // CPU and MMU
    runCPU(round);
    runMMU(round);
    if (ref().testsTranslation()) {
        if (cpuModel == CPUModel::M68030) runMMUTranslate(round);
        if (cpuModel == CPUModel::M68040) runMMUTranslate040(round);
    }

    // FPU (for all FPUs that can be combined with this CPU)
    for (FPUModel fpu : fpus) {

        if (!isCompatible(cpuModel, fpu)) continue;
        selectFpu(fpu);
        runFPU(round);

        // Without a reference FPU, there is nothing more to report
        if (!ref().hasFpu()) break;
    }
}


void ExecRunner::runCPU(long round)
{
    static const int CPU_SETUPS = 32;
    
    Setup setup;
    resetClocks();

    printf("%11s CPU : CORE ", selectedModel()); fflush(stdout);
    common::Progress progress;

    for (int i = 0; i < CPU_SETUPS; i++) {

        setupTestEnvironment(setup, round * CPU_SETUPS + i);

        for (int op = 0x0000; op <= 0xFFFF; op++) {

            progress.update(100.0 * (i * 0x10000 + op) / (CPU_SETUPS * 0x10000));

            // Skip illegal opcodes if requested
            if constexpr (SKIP_ILLEGAL) { if (isIllegalOpcode(u16(op))) continue; }

            // Skip instructions covered by runFPU
            if (isFpuOpcode(u16(op))) continue;

            // Execute both CPU cores
            runSingleTest(setup, u16(op));
        }
    }
    progress.finish();
    passed(moiraclk[0], refclk[0]);
}


void ExecRunner::runFPU(long round)
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
    common::Progress progress;

    for (int i = 0; i < FPU_SETUPS; i++) {

        setupTestEnvironment(setup, round * FPU_SETUPS + i);
        setupFpuEnvironment(setup);

        for (int op = 0xF200; op <= 0xF23F; op++) {

            progress.update(100.0 * (i * 64 + (op - 0xF200)) / (FPU_SETUPS * 64));
            
            for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

                runSingleTest(setup, u16(op), u16(ext));
            }
        }
    }
    progress.finish();
    passed(moiraclk[0], refclk[0]);
}


void ExecRunner::setupFpuEnvironment(Setup &s)
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


void ExecRunner::passed(clock_t moira, clock_t refTime)
{
    printf(" PASSED  (Moira: %.2fs  %s: %.2fs)\n",
           double(moira) / double(CLOCKS_PER_SEC),
           ref().name(),
           double(refTime) / double(CLOCKS_PER_SEC));
}


void ExecRunner::runSingleTest(Setup &s, u16 op, u16 ext)
{
    s.ext1 = ext;
    runSingleTest(s, op);
}


void ExecRunner::runSingleTest(Setup &s, u16 op)
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


// Executes the instruction on the reference CPU (Musashi or UAE)
void ExecRunner::runReference(Setup &s, Result &r)
{
    r.oldpc = s.pc;
    r.opcode = get16(refMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;

    if (!doExec(r.opcode)) return;

    ref().execute(r);
}


// Executes the instruction on Moira
void ExecRunner::runMoira(Setup &s, Result &r)
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


bool ExecRunner::skipAccessCheck(u16 op)
{
    return ref().skipAccessCheck(op);
}


bool ExecRunner::skip(const Setup &s)
{
    return ref().skipSetup(s);
}


void ExecRunner::recordMoiraRegisters(Result &r)
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

} // namespace runner
