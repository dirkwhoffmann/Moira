// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Shared.h"
#include "Sandbox.h"

void
Sandbox::prepare(bool enable, bool ignore)
{
    enabled = enable;
    ignored = ignore;
    recordCnt = replayCnt = 0;
    for (bool &u : used) u = false;
    errors = 0;
}

void
Sandbox::record(AccessType type, u32 addr, u64 cycle, u32 fc, u16 value)
{
    if (byteGranular && type == POKE16) {
        record(POKE8, addr, cycle, fc, u8(value >> 8));
        record(POKE8, (addr + 1) & addrMask, cycle, fc, u8(value));
        return;
    }

    access[recordCnt] = AccessRecord { type, addr, value, cycle, fc };

    recordCnt++;
    assert(recordCnt < 256);
}

void
Sandbox::replayPoke(AccessType type, u32 addr, u64 cycle, u32 fc, u16 value)
{
    if (!enabled) return;

    if (byteGranular && type == POKE16) {
        replayPoke(POKE8, addr, cycle, fc, u8(value >> 8));
        replayPoke(POKE8, (addr + 1) & addrMask, cycle, fc, u8(value));
        return;
    }

    // Ignore some opcodes that are handled differently by Moira
    if (ignored) return;

    for (int i = 0; i < recordCnt; i++) {

        // Enable for strict checking
        if (byteGranular) { if (used[i]) continue; }
        else if (i != replayCnt) continue;

        // Check for a matching entry
        if (access[i].type != type) continue;
        if (access[i].addr != addr) continue;
        if (access[i].value != value) continue;
        if (access[i].fc != fc) continue;
        if (checkCycles) {
            if (access[i].cycle != cycle) continue;
        }

        // Match found
        used[i] = true;
        replayCnt++;
        if (replayCnt > recordCnt) { break; }
        return;
    }

    error(type, addr, cycle, fc, value);
}

void
Sandbox::error(AccessType type, u32 addr, u64 cycle, u32 fc, u16 value)
{
    if (errors++ == 0) {

        printf("\nACCESS %d DOESN'T MATCH:\n", replayCnt);
        printf("i: %2d  ", replayCnt);
        printf("Type: %s  ", accessTypeStr[type]);
        printf("Addr: %4x  ", addr);
        printf("Cycle: %3lu  ", (long)cycle);
        printf("FC: %d  ", fc);
        if (type == POKE8 || type == POKE16) printf("Value: %4x  ", value);
        printf("\n\n");

        printf("ACCESS RECORD:\n");
        for (int i = 0; i < recordCnt; i++) {
            printf("i: %2d  ", i);
            printf("Type: %s  ", accessTypeStr[access[i].type]);
            printf("Addr: %4x  ", access[i].addr);
            printf("Cycle: %3lu  ", (long)access[i].cycle);
            printf("FC: %d  ", access[i].fc);
            printf("Value: %4x  ", access[i].value);
            printf("\n");
        }
    }
}
