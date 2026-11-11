// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <optional>
#include <vector>

#include "Shared.h"

#include "TestCPU.h"

namespace runner {

using namespace moira;

// Model tables (independent of a runner instance)
const char *modelName(CPUModel model);
std::optional<CPUModel> parseModel(const char *name);

const std::vector<FPUModel> &allFpuModels();
const char *fpuModelName(FPUModel model);
std::optional<FPUModel> parseFpuModel(const char *name);

// Returns true if the FPU can be installed in a system with the given CPU
bool isCompatible(CPUModel cpu, FPUModel fpu);

// Returns true if the two disassembler outputs match (UAE syntax)
bool dasmEqualUAE(const char *uae, const char *moira);

/* The test runner. It owns the Moira instance under test and the state of the
 * test that is currently running. There is exactly one instance per process.
 */
class TestRunner {

public:

    TestRunner();
    ~TestRunner();

    // Returns the runner of this process (used by the reference facades)
    static TestRunner &get();


    //
    // State
    //

    // The Moira instance under test
    TestCPU *moiracpu = nullptr;

    // The currently tested CPU type and instruction
    CPUModel cpuModel = CPUModel::M68000;
    FPUModel fpuModel = FPUModel::NONE;
    u16 opcode = 0;

    // Set by the memory interface of Moira if an odd address is read
    bool misalignedRead = false;

    // Set while MMU instructions are compared against UAE
    bool mmuCompare = false;

    // The setup of the test that is currently running (see RUNNER_DUMP)
    Setup *lastSetup = nullptr;

    // Speed comparison
    clock_t refclk[2] = {0,0};
    clock_t moiraclk[2] = {0,0};
    clock_t dasmclk[3] = {0,0,0};      // Disassembler time of the references (Binutils, Musashi, UAE)

    void resetClocks();


    //
    // Preparing a test
    //

    const std::vector<CPUModel> &allModels();

    void selectModel(CPUModel model);
    void selectFpu(FPUModel fpu);
    const char *selectedModel();

    // Returns the name of the reference CPU (determined by the compatibility mode)
    const char *reference();

    void setupMoira();
    void setupTestEnvironment(Setup &s, long round);
    void setupFpuEnvironment(Setup &s);
    void setupTestInstruction(Setup &s, u32 pc, u16 opcode);

    void resetMoira(Setup &s);


    //
    // Performing a test
    //

    void run(unsigned seed, long rounds, const std::vector<CPUModel> &cpus, const std::vector<FPUModel> &fpus);

    void runCPU(long round);
    void runMMU(long round);
    void runMMUTranslate(long round);
    void runMMUTranslate040(long round);
    void runFPU(long round);
    void runDasmCPU(long round);
    void runDasmMMU(long round);
    void runDasmFPU(long round);
    void passed(clock_t moira, clock_t ref);
    void passedDasm();

    void runSingleTest(Setup &s, u16 op);
    void runSingleTest(Setup &s, u16 op, u16 ext);

    void runDasmSingleTest(Setup &s, u16 op);
    void runDasmSingleTest(Setup &s, u16 op, u16 ext);

    void runReference(Setup &s, Result &r);
    void runMoira(Setup &s, Result &r);
    void disassembleReference(Setup &s, Result &r);
    void disassembleMoira(Setup &s, Result &r);

    bool skip(const Setup &s);                                           // Skips a test case based on its setup
    bool skipCompare(const Setup &s, const Result &rRef, const Result &rMoira); // Skips the comparison (includes skip)
    bool skipAccessCheck(u16 op);

    bool isFpuOpcode(u16 op);
    bool isMmuOpcode(u16 op);
    bool isIllegalOpcode(u16 op);


    //
    // Analysing a test result
    //

    void recordMoiraRegisters(Result &r);

    void compare(Setup &s, Result &rRef, Result &rMoira);
    bool compareDasm(Result &rRef, Result &rMoira);
    bool compareD(Result &rRef, Result &rMoira);
    bool compareA(Result &rRef, Result &rMoira);
    bool comparePC(Result &rRef, Result &rMoira);
    bool compareSR(Result &rRef, Result &rMoira);
    bool compareUSP(Result &rRef, Result &rMoira);
    bool compareISP(Result &rRef, Result &rMoira);
    bool compareMSP(Result &rRef, Result &rMoira);
    bool compareVBR(Result &rRef, Result &rMoira);
    bool compareSFC(Result &rRef, Result &rMoira);
    bool compareDFC(Result &rRef, Result &rMoira);
    bool compareCAxR(Result &rRef, Result &rMoira);
    bool compareCycles(Result &rRef, Result &rMoira);
    bool compareFPR(Result &rRef, Result &rMoira);
    bool compareFPCR(Result &rRef, Result &rMoira);
    bool compareFPSR(Result &rRef, Result &rMoira);
    bool compareFPIAR(Result &rRef, Result &rMoira);
    bool compareMMU(Result &rRef, Result &rMoira);
    bool compareMemory();
    void printMemoryDiff();

    void dumpDasm(Setup &s, Result &rRef, Result &rMoira);
    void dumpSetup(Setup &s);
    void dumpResult(Result &r);
    void dumpFailedSetup();
    void bugReport();
};

}
