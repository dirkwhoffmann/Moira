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

// Returns true if the records at i and i+1 are the two halves of a long word write (in either order)
bool
Sandbox::swappable(int i) const
{
    if (i + 1 >= recordCnt) return false;
    const AccessRecord &a = access[i], &b = access[i + 1];

    return a.type == POKE16 && b.type == POKE16 && a.fc == b.fc && (((a.addr + 2) ^ b.addr) & 0xFFFFFF) == 0 || (((b.addr + 2) ^ a.addr) & 0xFFFFFF) == 0;
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

    // In strict mode, accesses must arrive in the recorded order. The only exception is a long word write, whose
    // two word accesses may be swapped (the 68000 writes the low word first, some references the high word first).
    int first = 0;
    while (first < recordCnt && used[first]) first++;

    for (int i = 0; i < recordCnt; i++) {

        if (byteGranular) { if (used[i]) continue; }
        else if (i != first && !(swapLongWrites && i == first + 1 && swappable(first))) continue;

        // Check for a matching entry
        if (access[i].type != type) continue;
        if (access[i].addr != addr) continue;
        if (access[i].value != value) continue;
        if (access[i].fc != fc) continue;
        if (checkCycles) {

            // A swapped access takes the time slot of the access it replaces
            if (access[byteGranular ? i : first].cycle != cycle) continue;
        }

        // Match found
        used[i] = true;
        replayCnt++;
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
