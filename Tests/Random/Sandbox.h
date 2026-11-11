// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned long long u64;    // Same type as moira::u64 (avoids ambiguities)

enum AccessType {
    PEEK8,
    PEEK16,
    POKE8,
    POKE16
};

static const char *accessTypeStr[] = {
    "Peek8 ", "Peek16",
    "Poke8 ", "Poke16",
    "Poll  "
};

struct AccessRecord {

    AccessType type;
    u32 addr;
    u16 value;
    u64 cycle;
    u32 fc;
};

class Sandbox {

    bool enabled = false;
    bool ignored = false;

    AccessRecord access[256];
    bool used[256];
    int recordCnt;
    int replayCnt;
    long errors;

public:

    // Set to false if the reference doesn't provide exact access timing
    bool checkCycles = true;

    // Compare writes byte by byte (misaligned accesses are split differently)
    bool byteGranular = false;
    u32 addrMask = 0xFFFFFFFF;

    Sandbox() { };
    // enable: check the accesses. ignore: skip the replay (handled differently by Moira)
    void prepare(bool enable, bool ignore);
    long getErrors() { return errors; }
    int records() const { return recordCnt; }
    void record(AccessType type, u32 addr, u64 cycle, u32 fc, u16 value);
    void replayPoke(AccessType type, u32 addr, u64 cycle, u32 fc, u16 value);

    void error(AccessType type, u32 addr, u64 cycle, u32 fc, u16 value = 0);
};
