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

namespace runner {

/* Disassembles every opcode and compares the output of Moira with reference
 * disassemblers. The Motorola flavor compares Moira with Binutils, Musashi and
 * UAE. The MIT flavor compares Moira with Binutils only (which then needs to be
 * compiled without MOTOROLA).
 */
class DasmRunner : public TestRunner {

public:

    explicit DasmRunner(bool mitSyntax)
    {
        mit = mitSyntax;
        
        dasmBinutils = true;
        dasmMusashi  = !mitSyntax;
        dasmUAE      = !mitSyntax;
        
        checksMoiraSyntax = true;
    }


    //
    // TestRunner interface
    //

    const char *description() const override;
    const char *referenceName() const override { return mit ? "Binutils (MIT)" : "Binutils, Musashi, UAE"; }
    void printRanges() const override;
    void runStages(long round, const std::vector<FPUModel> &fpus) override;
    void setupReferences() override;
    void selectFpuReferences() override;


    //
    // Stages
    //

    void runDasmCPU(long round);
    void runDasmMMU(long round);
    void runDasmFPU(long round);
    void passedDasm();


    //
    // Performing a test
    //

    void runDasmSingleTest(Setup &s, u16 op);
    void runDasmSingleTest(Setup &s, u16 op, u16 ext);


    //
    // Analysing a test result
    //

    bool compareDasm(Result &rRef, Result &rMoira);
};

}
