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

Randomizer::Randomizer()
{
    srand(0);
}

void
Randomizer::init(int testrun)
{
    // In the first few rounds, use a single constant
    if (testrun <= 10) {

        mode = testrun;
        return;
    }

    // Iterate through all remaining modes
    switch(testrun % 8) {

        case 0: mode = 11; break;
        case 1: mode = 12; break;
        case 2: mode = 13; break;
        case 3: mode = 14; break;
        case 4:
        case 5: mode = 15; break;
        case 6:
        case 7: mode = 16; break;
    }
}

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
