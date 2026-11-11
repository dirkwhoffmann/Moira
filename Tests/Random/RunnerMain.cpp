// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "TestRunner.h"
#include <string.h>
#include <algorithm>

int runner::runnerMain(TestRunner &runner, int argc, char **argv)
{
    long long seed = common::defaultSeed();
    long long rounds = 1;
    std::vector<moira::CPUModel> cpus;
    std::vector<moira::FPUModel> fpus;

    std::string cpuHelp = "CPU model to test (default: all models). May be repeated.\nModels:";
    for (auto model : runner.allModels()) cpuHelp += std::string(" ") + runner::modelName(model);
    
    std::string fpuHelp = "FPU model to test in the FPU stage (default: all models). May be repeated.\nModels:";
    for (auto model : runner::allFpuModels()) fpuHelp += std::string(" ") + runner::fpuModelName(model);

    common::Args args(argc, argv);
    args.integer("--seed", seed, "Seed of the random number generator (default: derived from the date)");
    args.custom("--rounds", "<n>", [&](const char *s) {
        char *end = nullptr;
        rounds = strtoll(s, &end, 0);
        return end != s && *end == '\0' && rounds > 0;
    }, "Number of test rounds (default: 1)");
    args.custom("--cpu", "<model>", [&](const char *s) {
        auto model = runner::parseModel(s);
        if (!model) return false;
        const auto &all = runner.allModels();
        if (std::find(all.begin(), all.end(), *model) == all.end()) return false;
        cpus.push_back(*model);
        return true;
    }, cpuHelp.c_str());
    args.custom("--fpu", "<model>", [&](const char *s) {
        auto model = runner::parseFpuModel(s);
        if (!model) return false;
        fpus.push_back(*model);
        return true;
    }, fpuHelp.c_str());
    if (!args.parse()) return 1;

    common::printBanner(runner.description());
    common::printSetting("Core under test", "Moira %s", moira::Moira::build().c_str());
    common::printSetting("Reference", "%s", runner.referenceName());
    printf("\n");
    common::printSetting("Test rounds", "%lld", rounds);
    common::printSetting("Random seed", "%d", (int)seed);
    runner.printRanges();

    runner.run((unsigned)seed, rounds,
                cpus.empty() ? runner.allModels() : cpus,
                fpus.empty() ? runner::allFpuModels() : fpus);

    printf("\n");
    printf("All tests completed\n");
    return 0;
}
