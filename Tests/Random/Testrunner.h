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

/* Base class of all test runners. It owns the Moira instance under test and the
 * state of the test that is currently running. A concrete runner decides which
 * stages are executed by implementing runStages(). There is exactly one runner
 * per process.
 *
 *   ExecRunner  Executes random instructions on Moira and on a reference CPU.
 *   DasmRunner  Disassembles every opcode and compares the output with reference
 *               disassemblers.
 */
class TestRunner {

public:

    TestRunner();
    virtual ~TestRunner();

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

    // Set while MMU instructions are compared against a reference
    bool mmuCompare = false;

    // The setup of the test that is currently running (see RUNNER_DUMP)
    Setup *lastSetup = nullptr;

    // Disassembler configuration (used by the DasmRunner and for error reports of the ExecRunner)
    bool mit = false;                  // Compare the MIT syntax flavor (Binutils compiled without MOTOROLA)
    bool dasmBinutils = false;         // Compare with Musashi's disassembler
    bool dasmMusashi = false;          // Compare with Musashi's disassembler
    bool dasmUAE = false;              // Compare with UAE's disassembler
    bool checksMoiraSyntax = false;    // Check Moira's own syntax for plausibility

    // Speed comparison
    clock_t refclk[2] = {0,0};
    clock_t moiraclk[2] = {0,0};
    clock_t dasmclk[3] = {0,0,0};      // Disassembler time of the references (Binutils, Musashi, UAE)

    void resetClocks();


    //
    // Interface to be implemented by a concrete runner
    //

    // Text shown in the start-up banner
    virtual const char *description() const = 0;
    virtual const char *referenceName() const = 0;
    virtual void printRanges() const = 0;

    // Returns true if the CPU model can be tested by this runner
    virtual bool supports(CPUModel model) const { return true; }

    // Runs all stages of one test round for the selected CPU model. The FPU models
    // that can be combined with the CPU are passed in.
    virtual void runStages(long round, const std::vector<FPUModel> &fpus) = 0;

    // Replays a test that has been dumped with RUNNER_DUMP. Returns true if a test was replayed.
    virtual bool replay() { return false; }

    // Hooks that are called when a model is selected
    virtual void setupReferences() { }            // Before Moira is configured
    virtual void selectFpuReferences() { }        // After the FPU model has changed
    virtual void modelSelected() { }              // After everything is configured

    // Lets the runner adjust a test case before it is run
    virtual void prepare(Setup &s) { }


    //
    // Preparing a test
    //

    const std::vector<CPUModel> &allModels();

    void selectModel(CPUModel model);
    void selectFpu(FPUModel fpu);
    const char *selectedModel();

    void setupMoira();
    void setupTestEnvironment(Setup &s, long round);
    void setupTestInstruction(Setup &s, u32 pc, u16 opcode);

    void resetMoira(Setup &s);

    bool isFpuOpcode(u16 op);
    bool isMmuOpcode(u16 op);
    bool isIllegalOpcode(u16 op);


    //
    // Running the tests
    //

    void run(unsigned seed, long rounds, const std::vector<CPUModel> &cpus, const std::vector<FPUModel> &fpus);


    //
    // Disassembling
    //

    void disassembleReference(Setup &s, Result &r);
    void disassembleMoira(Setup &s, Result &r);
    void dumpDasm(Setup &s, Result &rRef, Result &rMoira);


    //
    // Reporting
    //

    void dumpSetup(Setup &s);
    void dumpResult(Result &r);
    void dumpFailedSetup();
    void bugReport();

private:

    std::vector<CPUModel> models;
    bool modelsBuilt = false;
};

// Shared entry point of all runner executables: parses the command line and runs the tests
int runnerMain(TestRunner &runner, int argc, char **argv);

}
