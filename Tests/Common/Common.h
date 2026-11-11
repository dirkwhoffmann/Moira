// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "MoiraTypes.h"
#include <stdint.h>
#include <optional>
#include <functional>
#include <string>
#include <vector>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

namespace common {

//
// Output
//

// Prints the logo, the copyright notice and the tool description
void printBanner(const char *description);

// Prints a line of the settings block (the key is right-aligned)
void printSetting(const char *key, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

// Prints the message that asks the user to report a bug
void printBugReport();


//
// CPU models
//

// All CPU models in ascending order
const std::vector<moira::CPUModel> &allCpuModels();

// Converts between models and names (68000, 68010, EC020, 68020, EC030, 68030, EC040, LC040, 68040)
const char *modelName(moira::CPUModel model);
std::optional<moira::CPUModel> parseModel(const char *name);


//
// Progress output
//

/* Prints the progress of a long-running test as a row of dots. A progress of
 * 100% corresponds to 32 dots. The tool reports its progress in percent as
 * often as it likes. Dots are only printed (and flushed) if they are needed.
 */
class Progress {

public:

    static constexpr int dots = 32;

    // Reports the current progress (0 ... 100)
    void update(double percent);

    // Completes the row
    void finish() { update(100.0); }

private:

    int printed = 0;
};


//
// Command line parsing
//

/* Declarative command line parser. Options are registered together with a
 * help text. The usage message is generated from the registered options.
 *
 *     Args args(argc, argv);
 *     args.integer("--rounds", rounds, "Number of test rounds");
 *     args.flag("--check", check, "Check the reference");
 *     if (!args.parse()) return 1;
 *
 * If parse() fails, it prints the usage message and returns false.
 */
class Args {

public:

    // Handles the argument of an option. Returns false if it is invalid.
    typedef std::function<bool(const char *)> Handler;

    Args(int argc, char **argv) : argc(argc), argv(argv) { }

    // Option without argument
    Args &flag(const char *name, bool &target, const char *help);

    // Options with argument
    Args &integer(const char *name, long long &target, const char *help);
    Args &integer(const char *name, long &target, const char *help);
    Args &integer(const char *name, int &target, const char *help);
    Args &number(const char *name, double &target, const char *help);
    Args &text(const char *name, std::string &target, const char *help, const char *arg = "<text>");
    Args &list(const char *name, std::vector<std::string> &target, const char *help, const char *arg = "<text>");

    // Option with an application-defined argument
    Args &custom(const char *name, const char *arg, Handler handler, const char *help);

    bool parse();
    void usage() const;

private:

    struct Option {
        std::string name;
        std::string arg;        // Empty for flags
        std::string help;
        Handler handler;
    };

    int argc;
    char **argv;
    std::vector<Option> options;
};


//
// Random numbers
//

/* Portable pseudo-random number generator. It reproduces the sequence of glibc's
 * rand() (TYPE_3 additive feedback generator, 31-bit results). The C library's
 * rand() is implementation defined, so using it would make the test runs depend
 * on the platform (Linux, macOS and Windows would execute different test cases).
 */
void portableSrand(unsigned seed);
int portableRand();

// Returns the seed used when none is given (derived from the current date)
unsigned defaultSeed();

// Generates random numbers that favor interesting values (0, 1, 0x80, 0x7FFF, ...)
class Randomizer {

public:

    u32 rand();
};

}
