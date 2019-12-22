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

#include "types.h"

namespace moira {

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
};

class Sandbox {

    AccessRecord access[64];
    int recordCnt;
    int replayCnt;

public:

    void prepare();
    void record(AccessType type, u32 addr, u64 cycle, u16 value);
    void recordPoll(u64 cycle, u8 value);
    u32  replayPeek(AccessType type, u32 addr, u64 cycle);
    void replayPoke(AccessType type, u32 addr, u64 cycle, u16 value);
    u8   replayPoll(u64 cycle);

    void error(AccessType type, u32 addr, u64 cycle, u16 value = 0);
};

}
#endif
