// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "Moira.h"
#include "Common.h"
#include <cstdio>

int main(int argc, char **argv)
{
    common::printBanner("Unit tests for selected Moira subsystems (bus errors, FPU, MMU).\n"
                        "The tests check Moira against fixed expectations and the UAE FPU implementation.\n");
    common::printSetting("Core under test", "Moira %s", moira::Moira::build().c_str());
    common::printSetting("Reference", "Hand-written expectations, UAE softfloat (FPU)");
    printf("\n");

    doctest::Context context(argc, argv);
    return context.run();
}
