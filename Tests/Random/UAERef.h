// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "Reference.h"

namespace runner {

class UAERef : public Reference {

public:

    const char *name() const override { return "UAE"; }

    void setup(CPUModel cpu, FPUModel fpu) override { setupUAE(cpu, fpu); }
    void selectFpu(CPUModel cpu, FPUModel fpu) override { setupUAE(cpu, fpu); }
    void reset(const Setup &s) override { resetUAE(s); }
    void execute(Result &r) override { executeUAE(r); }

    // Moira is configured without address error emulation (MOIRA_EMULATE_ADDRESS_ERROR)
    bool aborted() const override { return lastExceptionUAE() == 3 && !MOIRA_EMULATE_ADDRESS_ERROR; }

    bool hasUAEDasm() const override { return true; }
    bool comparesMMU(CPUModel model) const override { return model == CPUModel::M68030; }
    bool testsTranslation() const override { return true; }

    bool checksCycles(CPUModel model) const override { return model == CPUModel::M68000; }
    bool byteGranular(CPUModel model) const override { return model != CPUModel::M68000 && model != CPUModel::M68010; }

    bool skipSetup(const Setup &s) const override;

    bool skipAccessCheck(u16 op) const override;

    bool skipResult(const Setup &s, const Result &rRef, const Result &rMoira) const override;

    // UAE clears the lowest PC bit after a jump to an odd address. Moira
    // is configured without address error emulation and keeps it.
    bool equalPC(const Result &ref, const Result &moira) const override;

    u32 srMask(Instr instr) const override;

    bool equalFPR(const Result &rRef, const Result &rMoira) const override;

    // UAE stores the reserved low bits of FPCR
    bool equalFPCR(const Result &ref, const Result &moira) const override;

    bool equalFPSR(const Result &rRef, const Result &rMoira) const override;

    bool equalFPIAR(const Result &rRef, const Result &rMoira) const override;

    // UAE's cores are only used as a timing reference for the 68000. The 68010 timing
    // is partially copied from the 68000, and the 68020+ cores aren't cycle-exact.
    bool ignoreCycles(CPUModel model) const override { return model != CPUModel::M68000; }
};

}
