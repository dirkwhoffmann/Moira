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
#include <ctime>
#include <algorithm>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

static void usage(const char *prog)
{
    printf("Usage: %s [--seed <n>] [--rounds <n>] [--cpu <model>]... [--fpu <model>]...\n", prog);
    printf("  --seed <n>      Seed of the random number generator (default: derived from the date)\n");
    printf("  --rounds <n>    Number of test rounds (default: 1)\n");
    printf("  --cpu <model>   CPU model to test (default: all models). May be repeated.\n");
    printf("                  Models:");
    for (auto model : runner::allModels()) printf(" %s", runner::modelName(model));
    printf("\n");
    printf("  --fpu <model>   FPU model to test in the FPU stage (default: all models). May be repeated.\n");
    printf("                  Models:");
    for (auto model : runner::allFpuModels()) printf(" %s", runner::fpuModelName(model));
    printf("\n");
}

int main(int argc, char **argv)
{
    // Without an explicit seed, the seed is derived from the current date
    std::time_t t = std::time(0);
    std::tm *now = std::localtime(&t);
    unsigned seed = 1900 + now->tm_year + now->tm_yday;

    long rounds = 1;
    std::vector<moira::CPUModel> cpus;
    std::vector<moira::FPUModel> fpus;

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

            if (auto model = runner::parseModel(argv[++i])) {
                const auto &all = runner::allModels();
                if (std::find(all.begin(), all.end(), *model) != all.end()) { cpus.push_back(*model); continue; }
            }

        } else if (!strcmp(argv[i], "--fpu") && i + 1 < argc) {

            if (auto model = runner::parseFpuModel(argv[++i])) { fpus.push_back(*model); continue; }
        }

        usage(argv[0]);
        return 1;
    }

    printf("Moira - A Motorola 68k CPU emulator\n\n");
    printf("Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de\n");
    printf("Published under the terms of the MIT License\n\n");
    if (DASM_STAGES) {
        printf("Compares the disassembler output of Moira with reference disassemblers.\n");
        printf("Every opcode is disassembled in several syntaxes and the texts are compared.\n\n");
    } else {
        printf("Runs random instructions on Moira and on a reference CPU and compares\n");
        printf("registers, memory and cycle counts after each instruction.\n\n");
    }
    printf("Core under test : Moira %s\n", moira::Moira::build().c_str());
    printf("      Reference : %s\n", DASM_STAGES ? (DASM_MIT ? "Binutils (MIT)" : "Binutils, Musashi, UAE") : runner::reference());
    printf("\n");
    printf("    Test rounds : %ld\n", rounds);
    printf("    Random seed : %d\n", seed);
    printf("     Exec range : %s\n", TOSTRING(doExec(opcode)));
    printf("     Dasm range : %s\n", TOSTRING(doDasm(opcode)));

    runner::run(seed, rounds,
                cpus.empty() ? runner::allModels() : cpus,
                fpus.empty() ? runner::allFpuModels() : fpus);

    printf("\n");
    printf("All tests completed\n");
    return 0;
}
