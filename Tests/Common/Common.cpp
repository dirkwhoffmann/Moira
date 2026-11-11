// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Common.h"
#include <algorithm>
#include <bit>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace common {

//
// Output
//

void
printBanner(const char *description)
{
    printf("Moira - A Motorola 68k CPU emulator\n\n");
    printf("Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de\n");
    printf("Published under the terms of the MIT License\n\n");
    printf("%s\n", description);
}

void
printSetting(const char *key, const char *fmt, ...)
{
    printf("%15s : ", key);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}

void
printBugReport()
{
    printf("Please report the bug at https://github.com/dirkwhoffmann/Moira\n");
    printf("Thank you!\n\n");
}


//
// Progress output
//

void
Progress::update(double percent)
{
    int needed = int(percent * dots / 100.0);
    if (needed > dots) needed = dots;
    if (needed <= printed) return;

    while (printed < needed) { putchar('.'); printed++; }
    fflush(stdout);
}


//
// Command line parsing
//

Args &
Args::flag(const char *name, bool &target, const char *help)
{
    options.push_back({ name, "", help, [&target](const char *) { target = true; return true; } });
    return *this;
}

Args &
Args::integer(const char *name, long long &target, const char *help)
{
    return custom(name, "<n>", [&target](const char *s) {
        char *end = nullptr;
        long long v = strtoll(s, &end, 0);
        if (end == s || *end) return false;
        target = v;
        return true;
    }, help);
}

Args &
Args::integer(const char *name, long &target, const char *help)
{
    return custom(name, "<n>", [&target](const char *s) {
        char *end = nullptr;
        long long v = strtoll(s, &end, 0);
        if (end == s || *end) return false;
        target = long(v);
        return true;
    }, help);
}

Args &
Args::integer(const char *name, int &target, const char *help)
{
    return custom(name, "<n>", [&target](const char *s) {
        char *end = nullptr;
        long long v = strtoll(s, &end, 0);
        if (end == s || *end) return false;
        target = int(v);
        return true;
    }, help);
}

Args &
Args::number(const char *name, double &target, const char *help)
{
    return custom(name, "<x>", [&target](const char *s) {
        char *end = nullptr;
        double v = strtod(s, &end);
        if (end == s || *end) return false;
        target = v;
        return true;
    }, help);
}

Args &
Args::text(const char *name, std::string &target, const char *help, const char *arg)
{
    return custom(name, arg, [&target](const char *s) { target = s; return true; }, help);
}

Args &
Args::list(const char *name, std::vector<std::string> &target, const char *help, const char *arg)
{
    return custom(name, arg, [&target](const char *s) { target.push_back(s); return true; }, help);
}

Args &
Args::custom(const char *name, const char *arg, Handler handler, const char *help)
{
    options.push_back({ name, arg, help, handler });
    return *this;
}

bool
Args::parse()
{
    for (int i = 1; i < argc; i++) {

        const Option *match = nullptr;
        for (auto &o : options) if (o.name == argv[i]) { match = &o; break; }

        if (!match) { usage(); return false; }

        if (match->arg.empty()) {

            match->handler(nullptr);

        } else {

            if (i + 1 >= argc || !match->handler(argv[++i])) { usage(); return false; }
        }
    }
    return true;
}

void
Args::usage() const
{
    // Synopsis
    printf("Usage: %s", argv[0]);
    for (auto &o : options) {
        printf(" [%s%s%s]", o.name.c_str(), o.arg.empty() ? "" : " ", o.arg.c_str());
    }
    printf("\n");

    // Option descriptions. A help text may span several lines.
    size_t width = 0;
    for (auto &o : options) {
        width = std::max(width, o.name.size() + (o.arg.empty() ? 0 : 1 + o.arg.size()));
    }
    for (auto &o : options) {

        std::string left = o.name + (o.arg.empty() ? "" : " " + o.arg);
        const char *p = o.help.c_str();
        bool first = true;

        do {
            const char *nl = strchr(p, '\n');
            int len = nl ? int(nl - p) : int(strlen(p));
            printf("  %-*s  %.*s\n", int(width), first ? left.c_str() : "", len, p);
            p = nl ? nl + 1 : nullptr;
            first = false;
        } while (p);
    }
}


//
// Random numbers
//

namespace {

u32 rstate[34];     // The last 34 values of the feedback sequence (ring buffer)
int rpos = 0;       // Slot of the oldest value

}

void
portableSrand(unsigned seed)
{
    // Mirrors glibc's srandom_r() for the default state size (degree 31)
    int32_t r[344];
    if (seed == 0) seed = 1;
    r[0] = int32_t(seed);
    for (int i = 1; i < 31; i++) {
        int64_t word = 16807LL * r[i - 1] % 2147483647;
        if (word < 0) word += 2147483647;
        r[i] = int32_t(word);
    }
    for (int i = 31; i < 34; i++) r[i] = r[i - 31];
    for (int i = 34; i < 344; i++) r[i] = int32_t(u32(r[i - 31]) + u32(r[i - 3]));
    for (int i = 0; i < 34; i++) rstate[i] = u32(r[310 + i]);
    rpos = 0;
}

int
portableRand()
{
    u32 v = rstate[(rpos + 34 - 31) % 34] + rstate[(rpos + 34 - 3) % 34];
    rstate[rpos] = v;
    rpos = (rpos + 1) % 34;
    return int(v >> 1);
}

unsigned
defaultSeed()
{
    std::time_t t = std::time(0);
    std::tm *now = std::localtime(&t);
    return 1900 + now->tm_year + now->tm_yday;
}

u32
Randomizer::rand()
{
    u32 r = portableRand();
    u32 s = portableRand();

    u32 s1 = (s >> 0) & 0xF;
    u32 s2 = (s >> 4) & 0x7;

    switch (s1) {

        case 0: return 0;           // Constants
        case 1: return 1;

        case 2: case 3: case 4:     // 8-bit number

            return
            s2 == 2 ? 0x80 :
            s2 == 3 ? 0x7F :
            s2 == 4 ? 0xFF : u8(r);

        case 5: case 6: case 7:     // 16-bit number

            return
            s2 == 2 ? 0x8000 :
            s2 == 3 ? 0x7FFF :
            s2 == 4 ? 0xFFFF : u16(r);

        case 8: case 9: case 10:     // 32-bit number

            return
            s2 == 2 ? 0x80000000 :
            s2 == 3 ? 0x7FFFFFFF :
            s2 == 4 ? 0xFFFFFFFF : u32(r);

        case 11:                     // Bit field
        {
            u32 mask2 = std::rotl(u32(0xFFFF), portableRand() & 15);
            u32 mask3 = std::rotl(u32(0xFFFF), portableRand() & 15);
            return std::rotl(u32(mask2 & mask3), portableRand() & 15);
        }

        case 12:                    // Repeating patterns

            return
            s2 == 0 ? 0xF0F0F0F0 :
            s2 == 1 ? 0x0F0F0F0F :
            s2 == 2 ? 0xFF00FF00 :
            s2 == 3 ? 0x00FF00FF :
            s2 == 4 ? 0xFFFF0000 :
            s2 == 5 ? 0xAAAAAAAA :
            s2 == 6 ? 0xCCCCCCCC : 0x11111111;

        default:                    // Real random number

            return r;
    }
}

}
