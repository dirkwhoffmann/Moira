// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Randomizer.h"
#include <bit>
#include <cstdlib>

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
