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


moira::Moira *moiracpu;
Tester_68k *tester;
uint8_t mem[0x10000];

int main()
{
    moiracpu = new moira::Moira();
    tester = new Tester_68k();

    if (MIMIC_MUSASHI) {

        // Run Moira against Musashi
        run();
        // dasmTest();

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

