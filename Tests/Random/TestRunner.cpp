// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "TestRunner.h"
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

static TestRunner *theRunner = nullptr;

TestRunner::TestRunner() { theRunner = this; moiracpu = new TestCPU(); }
TestRunner::~TestRunner() { delete moiracpu; theRunner = nullptr; }
TestRunner &TestRunner::get() { return *theRunner; }

void TestRunner::resetClocks() { refclk[0] = refclk[1] = moiraclk[0] = moiraclk[1] = 0; dasmclk[0] = dasmclk[1] = dasmclk[2] = 0; }


const std::vector<CPUModel> &TestRunner::allModels()
{
    if (!modelsBuilt) {
        for (auto model : common::allCpuModels()) if (supports(model)) models.push_back(model);
        modelsBuilt = true;
    }
    return models;
}

const char *TestRunner::selectedModel()
{
    return modelName(cpuModel);
}

void TestRunner::selectModel(CPUModel model)
{
    cpuModel = model;

    // Only the 68040 has a built-in FPU. External FPUs are installed with selectFpu.
    fpuModel = (cpuModel == CPUModel::M68040) ? FPUModel::BUILTIN : FPUModel::NONE;

    setupReferences();
    setupBinutils(cpuModel);
    setupMoira();

    // Configure the sandbox
    sandbox.addrMask = (cpuModel == CPUModel::M68EC020) ? 0xFFFFFF : 0xFFFFFFFF;

    modelSelected();
}

void TestRunner::setupMoira()
{
    moiracpu->setModel(cpuModel);
    moiracpu->setFpuModel(fpuModel);
}

void TestRunner::selectFpu(FPUModel fpu)
{
    fpuModel = fpu;

    selectFpuReferences();
    setupMoira();
}

void TestRunner::run(unsigned seed, long rounds, const std::vector<CPUModel> &cpus, const std::vector<FPUModel> &fpus)
{
    portableSrand(seed);

    if (replay()) return;

    for (long i = 1; i <= rounds; i++) {

        printf("\nRound %ld:\n\n", i);

        for (CPUModel cpu : cpus) {

            selectModel(cpu);
            runStages(i, fpus);
        }
    }
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


void TestRunner::setupTestEnvironment(Setup &s, long round)
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

    // The FPU and MMU registers are set by setupFpuEnvironment() and the MMU tests. All other
    // stages must not see indeterminate values there, because they are applied to both CPUs
    // (a random TC value could enable the address translation).
    for (auto &f : s.fpr) f = MFloat80();
    s.fpcr = s.fpsr = s.fpiar = 0;
    s.crp = s.srp = 0;
    s.tc = s.tt0 = s.tt1 = 0;
    s.mmusr = 0;

    for (int i = 0; i < 8; i++) s.d[i] = randomizer.rand();
    for (int i = 0; i < 8; i++) s.a[i] = randomizer.rand();

    for (unsigned i = 0; i < sizeof(s.mem); i++) {
        s.mem[i] = u8(randomizer.rand());
    }
}


void TestRunner::setupTestInstruction(Setup &s, u32 pc, u16 opcode)
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
    prepare(s);

    set16(s.mem, pc, opcode);
    set16(s.mem, pc + 2, s.ext1);
    set16(s.mem, pc + 4, s.ext2);
    set16(s.mem, pc + 6, s.ext3);

    memcpy(refMem, s.mem, sizeof(refMem));
    memcpy(moiraMem, s.mem, sizeof(moiraMem));
}


void TestRunner::resetMoira(Setup &s)
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



bool TestRunner::isMmuOpcode(u16 op)
{
    return mmuCompare && (op & 0xFFC0) == 0xF000;
}


bool TestRunner::isFpuOpcode(u16 op)
{
    return (op & 0xFF00) == 0xF200 && moiracpu != nullptr && moiracpu->fpu.isPresent();
}


bool TestRunner::isIllegalOpcode(u16 op)
{
    return moiracpu->getInstrInfo(op).I == Instr::ILLEGAL;
}


void TestRunner::dumpSetup(Setup &s)
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


void TestRunner::dumpResult(Result &r)
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


// Writes the setup of the failed test to the file named in RUNNER_DUMP. Replay it with RUNNER_REPLAY.
void TestRunner::dumpFailedSetup()
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


void TestRunner::bugReport()
{
    dumpFailedSetup();
    common::printBugReport();
    exit(1);
}

// Disassembles the instruction with the reference disassemblers (Binutils plus Musashi or UAE)
void TestRunner::disassembleReference(Setup &s, Result &r)
{
    r.oldpc = s.pc;
    r.opcode = get16(refMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;
    r.dasmMusashi[0] = r.dasmUAE[0] = 0;
    r.dasmCntMusashi = r.dasmCntUAE = 0;

    clock_t t0 = clock();
    disassembleBinutils(r);
    dasmclk[0] += clock() - t0;

    if (dasmMusashi) { t0 = clock(); disassembleMusashi(s, r); dasmclk[1] += clock() - t0; }
    if (dasmUAE) { t0 = clock(); disassembleUAE(s, r); dasmclk[2] += clock() - t0; }
}

// Disassembles the instruction with Moira in all supported syntax flavors
void TestRunner::disassembleMoira(Setup &s, Result &r)
{
    r.oldpc = s.pc;
    r.opcode = get16(moiraMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;
    clock_t tStart = clock();
    r.dasmMoiraUAE[0] = 0;
    r.dasmCntMusashi = r.dasmCntMoiraUAE = 0;

    // Disassemble the instruction in GNU format (binutils)
    moiracpu->setDasmSyntax(mit ? Syntax::GNU_MIT : Syntax::GNU);
    moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 10, .plainZero = true });
    r.dasmCntBinutils = moiracpu->disassemble(r.dasmBinutils, r.oldpc);

    // Disassemble the instruction in Moira's own syntax
    r.dasmMoiraSyn[0] = 0;
    moiracpu->setDasmSyntax(mit ? Syntax::MOIRA_MIT : Syntax::MOIRA);
    moiracpu->setDasmLetterCase(LetterCase::MIXED_CASE);
    moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
    r.dasmCntMoiraSyn = moiracpu->disassemble(r.dasmMoiraSyn, r.oldpc);

    // Disassemble the instruction in Musashi format
    if (dasmMusashi) {
        moiracpu->setDasmSyntax(Syntax::MUSASHI);
        moiracpu->setDasmLetterCase(LetterCase::MIXED_CASE);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        r.dasmCntMusashi = moiracpu->disassemble(r.dasmMusashi, r.oldpc);
    }

    // Disassemble the instruction in UAE format
    if (dasmUAE) {
        moiracpu->setDasmSyntax(Syntax::UAE);
        moiracpu->setDasmLetterCase(LetterCase::LOWER_CASE);
        moiracpu->setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        r.dasmCntMoiraUAE = moiracpu->disassemble(r.dasmMoiraUAE, r.oldpc);
    }

    // Time spent in all syntax flavors
    r.elapsed[1] = clock() - tStart;
}

void TestRunner::dumpDasm(Setup &s, Result &rRef, Result &rMoira)
{
    if (checksMoiraSyntax) {
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

} // namespace runner
