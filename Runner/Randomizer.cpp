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
#include <ctime>

int
Randomizer::init()
{
    std::time_t t = std::time(0);
    std::tm* now = std::localtime(&t);

    auto seed = now->tm_year + now->tm_yday;

    srand(seed);
    return seed;
}

u32
Randomizer::rand()
{
    u32 r = ::rand();
    u32 s = ::rand();

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
            u32 mask2 = std::rotl(u32(0xFFFF), ::rand() & 15);
            u32 mask3 = std::rotl(u32(0xFFFF), ::rand() & 15);
            return std::rotl(u32(mask2 & mask3), ::rand() & 15);
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
