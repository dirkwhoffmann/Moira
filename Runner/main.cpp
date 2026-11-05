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

#include "Testrunner.h"

// Each Moira configuration provides an adapter for the runner (see MoiraAdapter.cpp)
namespace moiraUAE::runner     { std::unique_ptr<::runner::MoiraAdapter> create(); }
namespace moiraMusashi::runner { std::unique_ptr<::runner::MoiraAdapter> create(); }

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
    if (uae) runner::run(*moiraUAE::runner::create());
#else
    if (uae) printf("UAE is not available in this build.\n");
#endif
    if (musashi) runner::run(*moiraMusashi::runner::create());

    return 0;
}
