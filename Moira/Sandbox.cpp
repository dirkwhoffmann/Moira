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
Sandbox::record(AccessType type, u32 addr, u64 cycle, u16 value)
{
    access[recordCnt] = AccessRecord { type, addr, value, cycle };

    printf("Recording i: %2d  ", recordCnt);
    printf("Type: %s  ", accessTypeStr[type]);
    printf("Addr: %4x  ", addr);
    printf("Value: %4x  ", value);
    printf("Cycle: %lld  ", cycle);
    printf("\n");

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
        /*
        printf("i: %2d  ", replayCnt);
        printf("Type: %s  ", accessTypeStr[type]);
        printf("Addr: %4x  ", addr);
        printf("Cycle: %lld  ", cycle);
        printf("\n");
        */

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

    error(type, addr, cycle, value);
}

void
Sandbox::error(AccessType type, u32 addr, u64 cycle, u16 value)
{
    printf("\nACCESS %d DOESN'T MATCH:\n", replayCnt);
    printf("i: %2d  ", replayCnt);
    printf("Type: %s  ", accessTypeStr[type]);
    printf("Addr: %4x  ", addr);
    printf("Cycle: %lld  ", cycle);
    if (type == PEEK8 || type == PEEK16) printf("Value: %4x  ", value);
    printf("\n\n");
    printf("ACCESS RECORD:\n");
    for (int i = 0; i < recordCnt; i++) {
        printf("i: %2d  ", i);
        printf("Type: %s  ", accessTypeStr[access[i].type]);
        printf("Addr: %4x  ", access[i].addr);
        printf("Cycle: %lld  ", access[i].cycle);
        if (access[i].type == PEEK8 || access[i].type == PEEK16)
            printf("Value: %4x  ", access[i].value);
        printf("\n");
    }
    assert(false);
}

}
