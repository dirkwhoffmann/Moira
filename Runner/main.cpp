// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Shared.h"
#include <string.h>

// One test runner instance per Moira configuration (see Testrunner.h)
namespace runnerUAE     { void run(); }
namespace runnerMusashi { void run(); }

int main(int argc, char **argv)
{
    bool all = argc < 2;
    bool uae = all || !strcmp(argv[1], "uae");
    bool musashi = all || !strcmp(argv[1], "musashi");

    if (!uae && !musashi) {
        printf("Usage: %s [uae|musashi]\n", argv[0]);
        return 1;
    }

#ifdef HAVE_UAE
    if (uae) runnerUAE::run();
#else
    if (uae) printf("UAE is not available in this build.\n");
#endif
    if (musashi) runnerMusashi::run();

    return 0;
}
