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
    std::optional<unsigned> seed;
    
    if (argc == 1) {
        
        std::time_t t = std::time(0);
        std::tm* now = std::localtime(&t);
        seed = 1900 + now->tm_year + now->tm_yday;
    
    }
    
    if (argc == 3 && !strcmp(argv[1], "--seed")) {

        char *end = nullptr;
        unsigned value = (unsigned)strtoul(argv[2], &end, 0);
        if (*end == '\0') seed = value;
    }
    
    if (!seed) {
        
        printf("Usage: %s [--seed <n>]\n", argv[0]);
        return 1;
    }
    
    runner::run(*seed);
    return 0;
}
