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

class MAMERef : public Reference {

public:

    const char *name() const override { return "MAME"; }

    // MAME's microcoded core is a 68000
    bool supports(CPUModel model) const override { return model == CPUModel::M68000; }

    void setup(CPUModel cpu, FPUModel) override { setupMAME(cpu); }
    void reset(const Setup &s) override { resetMAME(s); }
    void execute(Result &r) override { executeMAME(r); }

    // Random exception vectors (odd handler addresses) would cause chains of address errors,
    // so all handlers start at a valid address.
    void prepare(Setup &s) const override;

    // Address errors are covered by the exception tests
    bool aborted() const override { return addressErrorMAME(); }

    bool checksCycles(CPUModel model) const override { return model == CPUModel::M68000; }

    // The 68000 has a 24 bit address bus. MAME doesn't keep the upper bits of the PC.
    bool equalPC(const Result &ref, const Result &moira) const override;
};

}
