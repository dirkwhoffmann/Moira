// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include <stdio.h>
#include "Moira.h"

namespace moira {

void
Sandbox::prepare()
{
    recordCnt = replayCnt = 0;
}

void
Sandbox::record(AccessType type, u32 addr, u16 value, u64 cycle)
{
    access[recordCnt] = AccessRecord { type, addr, value, cycle };
    recordCnt++;
    assert(recordCnt < 64);
}

u32
Sandbox::replayPeek(AccessType type, u32 addr, u64 cycle)
{
    for (int i = 0; i < recordCnt; i++) {

        // Enable for strict checking
        if (i != replayCnt) continue;

        // Check for a matching entry
        if (access[i].type != type) continue;
        if (access[i].addr != addr) continue;
        if (access[i].cycle != cycle) continue;

        // Match found
        replayCnt++;
        if (replayCnt > recordCnt) { break; }
        return access[i].value;
    }

    error(type, addr, cycle);
    return 0;
}

void
Sandbox::replayPoke(AccessType type, u32 addr, u64 cycle, u16 value)
{
    for (int i = 0; i < recordCnt; i++) {

        // Enable for strict checking
        if (i != replayCnt) continue;

        // Check for a matching entry
        if (access[i].type != type) continue;
        if (access[i].addr != addr) continue;
        if (access[i].value != value) continue;
        if (access[i].cycle != cycle) continue;

        // Match found
        replayCnt++;
        if (replayCnt > recordCnt) { break; }
        return;
    }

    error(type, addr, cycle);
}

void
Sandbox::error(AccessType type, u32 addr, u64 cycle)
{
   const char *accessTypeStr[] = { "Peek 8", "Peek 16", "Poke 8", "Poke 16" };

    printf("ACCESS %d DOESN'T MATCH:\n\n", replayCnt);
    printf("      Type: %s\n", accessTypeStr[type]);
    printf("   Address: %x\n", addr);
    printf("     Cycle: %lld\n\n", cycle);
    printf("ACCESS RECORD:\n");

    for (int i = 0; i < recordCnt; i++) {
        printf("%i: %d %x %lld %x\n", i,
            access[i].type, access[i].addr,
            access[i].cycle, access[i].value);
        assert(false);
    }
}

}
