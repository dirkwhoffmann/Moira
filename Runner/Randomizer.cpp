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



/*
u32
Randomizer::rand()
{
    return rand(mode);
}

u32
Randomizer::rand(int numberType)
{
    u32 r = ::rand();

    switch (numberType) {

        case 0:  return 0x0;
        case 1:  return 0x1;
        case 2:  return 0x7F;
        case 3:  return 0x80;
        case 4:  return 0xFF;
        case 5:  return 0x7FFF;
        case 6:  return 0x8000;
        case 7:  return 0xFFFF;
        case 8:  return 0x80000000;
        case 9:  return 0x7FFFFFFF;
        case 10: return 0xFFFFFFFF;

        case 11: // 8-bit number

            return
            (r & 7) == 0 ? 0x00 :
            (r & 7) == 1 ? 0x01 :
            (r & 7) == 2 ? 0x80 :
            (r & 7) == 3 ? 0x7F :
            (r & 7) == 4 ? 0xFF : u8(r);

        case 12: // 16-bit number

            return
            (r & 7) == 0 ? 0x0000 :
            (r & 7) == 1 ? 0x0001 :
            (r & 7) == 2 ? 0x8000 :
            (r & 7) == 3 ? 0x7FFF :
            (r & 7) == 4 ? 0xFFFF : u16(r);

        case 13: // 32-bit number

            return
            (r & 7) == 0 ? 0x00000000 :
            (r & 7) == 1 ? 0x00000001 :
            (r & 7) == 2 ? 0x80000000 :
            (r & 7) == 3 ? 0x7FFFFFFF :
            (r & 7) == 4 ? 0xFFFFFFFF : u32(r);

        case 14: // Bit fields
        {
            u32 mask2 = std::rotl(u32(0xFFFF), ::rand() & 15);
            u32 mask3 = std::rotl(u32(0xFFFF), ::rand() & 15);
            return std::rotl(u32(mask2 & mask3), ::rand() & 15);
        }

        case 15: // Repeating patterns

            return
            (r & 7) == 0 ? 0xF0F0F0F0 :
            (r & 7) == 1 ? 0x0F0F0F0F :
            (r & 7) == 2 ? 0xFF00FF00 :
            (r & 7) == 3 ? 0x00FF00FF :
            (r & 7) == 4 ? 0xFFFF0000 :
            (r & 7) == 5 ? 0xAAAAAAAA :
            (r & 7) == 6 ? 0xCCCCCCCC : 0x11111111;

        default: // Real random number

            return ::rand();
    }
}
*/
