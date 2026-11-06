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

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static void usage(const char *prog)
{
    printf("Usage: %s [--seed <n>] [--rounds <n>] [--cpu <model>]...\n", prog);
    printf("  --seed <n>      Seed of the random number generator (default: derived from the date)\n");
    printf("  --rounds <n>    Number of test rounds (default: 1)\n");
    printf("  --cpu <model>   CPU model to test (default: all models). May be repeated.\n");
    printf("                  Models:");
    for (auto model : runner::allModels()) printf(" %s", runner::modelName(model));
    printf("\n");
}

int main(int argc, char **argv)
{
    // Without an explicit seed, the seed is derived from the current date
    std::time_t t = std::time(0);
    std::tm *now = std::localtime(&t);
    unsigned seed = 1900 + now->tm_year + now->tm_yday;

    long rounds = 1;
    std::vector<moira::Model> models;

    for (int i = 1; i < argc; i++) {

        if (!strcmp(argv[i], "--seed") && i + 1 < argc) {

            char *end = nullptr;
            seed = (unsigned)strtoul(argv[++i], &end, 0);
            if (*end == '\0') continue;

        } else if (!strcmp(argv[i], "--rounds") && i + 1 < argc) {

            char *end = nullptr;
            rounds = strtol(argv[++i], &end, 0);
            if (*end == '\0' && rounds > 0) continue;

        } else if (!strcmp(argv[i], "--cpu") && i + 1 < argc) {

            if (auto model = runner::parseModel(argv[++i])) { models.push_back(*model); continue; }
        }

        usage(argv[0]);
        return 1;
    }

    printf("Moira CPU tester. (C) Dirk W. Hoffmann, 2019 - 2026\n");
    printf("Core %s\n\n", moira::Moira::build().c_str());
    printf("The test program runs Moira against a reference CPU with randomly generated data.\n");
    printf("\n");
    printf("    Test rounds : %ld\n", rounds);
    printf("    Random seed : %d\n", seed);
    printf("     Exec range : %s\n", TOSTRING(doExec(opcode)));
    printf("     Dasm range : %s\n", TOSTRING(doDasm(opcode)));
    printf("      Reference : %s\n", runner::reference());

    runner::run(seed, rounds, models.empty() ? runner::allModels() : models);

    printf("\n");
    printf("All tests completed\n");
    return 0;
}
