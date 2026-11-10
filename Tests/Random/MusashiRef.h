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

class MusashiRef : public Reference {

public:

    const char *name() const override { return "Musashi"; }

    void setup(CPUModel cpu, FPUModel) override { setupMusashi(cpu); }
    void reset(const Setup &s) override { resetMusashi(s); }
    void execute(Result &r) override { executeMusashi(r); }

    // Musashi's FPU is not a reliable reference
    bool hasFpu() const override { return false; }

    bool skipSetup(const Setup &s) const override;
    bool skipAccessCheck(u16 op) const override;
};

}
