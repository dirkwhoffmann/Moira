// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "SSTRunner.h"
#include <algorithm>

namespace sstrunner {

const char *PlainRunner::description() const
{
    return "Runs the instructions of the SingleStepTests on Moira and on a reference CPU and\n"
           "compares the results. The prerecorded results are ignored. Compares registers, memory,\n"
           "cycle counts and bus cycles (if the reference reports them). Cycle counts and bus cycles\n"
           "are only compared for the 68000.\n";
}

void PlainRunner::runTest(const sst::Test &t)
{
    // Some references cannot execute all instructions
    if (ref.core->unsupported(t)) { skipped++; skipNote = ref.core->unsupportedNote(); return; }

    {
        // Run the test as it is. If the reference ends in an address error, the aborted access is excluded.
        ref.load(t.initial);
        ref.cpu.execute();
        auto log = ref.cpu.log();
        bool ae = std::any_of(log.begin(), log.end(), [](auto &c) { return !c.write && c.fc == 5 && c.addr == 12; });
        const char *kd = "plain";
        if (ae) {
            u16 code = ref.mem.peek16(ref.cpu.state().ssp);
            kd = ((code & 7) == 2 || (code & 7) == 6) ? "ae-fetch" : (code & 0x10) ? "ae-read" : "ae-write";
        }
        mame::BusCycle none { 0, true, 0, 0, 0, 0 };
        runCase(t, t.initial, -1, 0, none, kd, false, !ae, 0, "");
    }
}

}
