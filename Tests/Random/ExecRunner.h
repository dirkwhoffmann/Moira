// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "TestRunner.h"
#include "Reference.h"

namespace runner {

/* Executes random instructions on Moira and on a reference CPU and compares
 * registers, memory and cycle counts after each instruction.
 */
class ExecRunner : public TestRunner {

    Reference &reference;

public:

    explicit ExecRunner(Reference &r) : reference(r)
    {
        // For error reports, the instruction is disassembled by the reference
        dasmMusashi = r.hasMusashiDasm();
        dasmUAE = r.hasUAEDasm();
    }

    // The reference CPU
    Reference &ref() const { return reference; }


    //
    // TestRunner interface
    //

    const char *description() const override;
    const char *referenceName() const override { return reference.name(); }
    void printRanges() const override;
    bool supports(CPUModel model) const override { return reference.supports(model); }
    void runStages(long round, const std::vector<FPUModel> &fpus) override;
    bool replay() override;
    void setupReferences() override;
    void selectFpuReferences() override;
    void modelSelected() override;
    void prepare(Setup &s) override { reference.prepare(s); }


    //
    // Stages
    //

    void runCPU(long round);
    void runMMU(long round);
    void runMMUTranslate(long round);
    void runMMUTranslate040(long round);
    void runFPU(long round);
    void passed(clock_t moira, clock_t refTime);

    void setupFpuEnvironment(Setup &s);


    //
    // Performing a test
    //

    void runSingleTest(Setup &s, u16 op);
    void runSingleTest(Setup &s, u16 op, u16 ext);

    void runReference(Setup &s, Result &r);
    void runMoira(Setup &s, Result &r);

    // Limits the range of executed instructions
    bool doExec(u16 op) { return op <= 0xEFFF || isFpuOpcode(op) || isMmuOpcode(op); }

    bool skip(const Setup &s);                                                  // Skips a test case based on its setup
    bool skipCompare(const Setup &s, const Result &rRef, const Result &rMoira); // Skips the comparison (includes skip)
    bool skipAccessCheck(u16 op);


    //
    // Analysing a test result
    //

    void recordMoiraRegisters(Result &r);

    void compare(Setup &s, Result &rRef, Result &rMoira);
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
};

}
