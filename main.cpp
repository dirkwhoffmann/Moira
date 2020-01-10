// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "testrunner.h"

#include "Moira.h"

int main()
{
    moiracpu = new TestCPU();
    tester = new Tester_68k();

    if (MIMIC_MUSASHI) {

        // Run Moira against Musashi
        run();

    } else {

        // Run the portable68000 unit tests

        try {
            tester->runTestCases();
            tester->printErrorCounter();

        } catch(Exception& exception) {
            exception.handle();
        }
    }

    return 0;
}

