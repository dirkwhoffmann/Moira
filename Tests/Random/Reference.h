// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Testrunner.h"

namespace runner {

// Shortcut for the runner of this process
inline TestRunner &tr() { return TestRunner::get(); }

/* A facade for the reference CPU (Musashi, UAE or MAME). It hides the glue layer
 * of the individual cores and collects everything that is specific to a reference:
 * how it is driven, which test cases it cannot handle, and which known differences
 * to Moira are tolerated when the results are compared.
 */
class Reference {

public:

    virtual ~Reference() = default;

    virtual const char *name() const = 0;

    //
    // Driving the core
    //

    // Selects the CPU model. Returns false if the core doesn't support it.
    virtual bool supports(CPUModel) const { return true; }
    virtual void setup(CPUModel cpu, FPUModel fpu) = 0;
    virtual void selectFpu(CPUModel cpu, FPUModel fpu) { }
    virtual void prepare(Setup &s) const { }      // Adjusts a test case before it is run
    virtual void reset(const Setup &s) = 0;
    virtual void execute(Result &r) = 0;

    // Returns true if the core raised an address error (the case is not compared)
    virtual bool aborted() const { return false; }

    // Returns true if the core is a reliable FPU reference
    virtual bool hasFpu() const { return true; }

    // Returns true if MMU registers are compared for this CPU model
    virtual bool comparesMMU(CPUModel) const { return false; }

    // Sandbox configuration
    virtual bool checksCycles(CPUModel) const { return false; }
    virtual bool byteGranular(CPUModel) const { return false; }

    //
    // Tolerating known differences
    //

    // Excludes a test case based on its setup (the reference cannot handle it)
    virtual bool skipSetup(const Setup &s) const { return false; }

    // Excludes the memory access check of an opcode
    virtual bool skipAccessCheck(u16 op) const { return false; }

    // Excludes a test case based on the results
    virtual bool skipResult(const Setup &s, const Result &ref, const Result &moira) const { return false; }

    // Comparison hooks. The defaults demand identical values.
    virtual bool equalPC(const Result &ref, const Result &moira) const { return ref.pc == moira.pc; }
    virtual u32 srMask(Instr instr) const { return 0xFFFFFFFF; }
    virtual bool equalFPR(const Result &ref, const Result &moira) const;
    virtual bool equalFPCR(const Result &ref, const Result &moira) const { return ref.fpcr == moira.fpcr; }
    virtual bool equalFPSR(const Result &ref, const Result &moira) const { return ref.fpsr == moira.fpsr; }
    virtual bool equalFPIAR(const Result &ref, const Result &moira) const { return ref.fpiar == moira.fpiar; }
    virtual bool ignoreCycles(CPUModel) const { return false; }
};

// Returns the reference CPU that matches the compatibility mode of the tested Moira
Reference &ref();

}
