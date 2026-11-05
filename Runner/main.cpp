// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Testrunner.h"
#include <string.h>

int main(int argc, char **argv)
{
    // Without an explicit seed, the seed is derived from the current date
    if (argc == 1) { runner::run(); return 0; }

    if (argc == 3 && !strcmp(argv[1], "--seed")) {

        char *end = nullptr;
        unsigned seed = (unsigned)strtoul(argv[2], &end, 0);
        if (!*end) { runner::run(seed); return 0; }
    }

    printf("Usage: %s [--seed <n>]\n", argv[0]);
    return 1;
}
