// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef SANDBOX_H
#define SANDBOX_H

#include "MoiraTypes.h"

using moira::u8;
using moira::u16;
using moira::u32;
using moira::u64;

enum AccessType {
    PEEK8,
    PEEK16,
    POKE8,
    POKE16,
    POLL,
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

    AccessRecord access[64];
    int recordCnt;
    int replayCnt;
    long errors;

public:

    Sandbox(); 
    void prepare();
    long getErrors() { return errors; }
    void record(AccessType type, u32 addr, u64 cycle, u32 fc, u16 value);
    void recordPoll(u64 cycle, u32 fc, u8 value);
    u32  replayPeek(AccessType type, u32 addr, u64 cycle, u32 fc);
    void replayPoke(AccessType type, u32 addr, u64 cycle, u32 fc, u16 value);
    u8   replayPoll(u64 cycle, u32 fc);

    void error(AccessType type, u32 addr, u64 cycle, u32 fc, u16 value = 0);
};

#endif
