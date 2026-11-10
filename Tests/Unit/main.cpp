// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include "Moira.h"
#include <cstdio>

int main(int argc, char **argv)
{
    printf("Moira - A Motorola 68k CPU emulator\n\n");
    printf("Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de\n");
    printf("Published under the terms of the MIT License\n\n");
    printf("Unit tests for selected Moira subsystems (bus errors, FPU, MMU).\n");
    printf("The tests check Moira against fixed expectations and the UAE FPU implementation.\n\n");
    printf("Core under test : Moira %s\n", moira::Moira::build().c_str());
    printf("Reference       : Hand-written expectations, UAE softfloat (FPU)\n\n");

    doctest::Context context(argc, argv);
    return context.run();
}
