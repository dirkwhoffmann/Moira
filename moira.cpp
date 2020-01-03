// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "testrunner.h"

#include "Moira.h"
#include "tester.h"

using namespace moira;

void
Moira::sync(int cycles)
{
    clock += cycles;
}

u8
Moira::read8(u32 addr)
{
    if (MUSASHI) {
        return get8(moiraMem, addr);
    } else {
        return sandbox.replayPeek(PEEK8, addr, getClock());
    }
}

u16
Moira::read16(u32 addr)
{
    if (MUSASHI) {
        return get16(moiraMem, addr);
    } else {
        return sandbox.replayPeek(PEEK16, addr, moiracpu->getClock());
    }
}

u16
Moira::read16Dasm(u32 addr)
{
    assert(MUSASHI);
    return get16(moiraMem, addr);
}

u16
Moira::read16OnReset(u32 addr)
{
    if (MUSASHI) {
        switch (addr) {
            case 0: return 0x0000;
            case 2: return 0x2000;
            case 4: return 0x0000;
            case 6: return 0x1000;
        }
        return get16(moiraMem, addr);
    } else {
        return sandbox.replayPeek(PEEK16, addr, getClock());
    }
}

void
Moira::write8(u32 addr, u8  val)
{
    sandbox.replayPoke(POKE8, addr, getClock(), val);
    set8(moiraMem, addr, val);
}

void
Moira::write16 (u32 addr, u16 val)
{
    sandbox.replayPoke(POKE16, addr, moiracpu->getClock(), val);
    set16(moiraMem, addr, val);
}

void
Moira::willPollIrq()
{
    if (!MUSASHI) {
        (void)sandbox.replayPoll(getClock());
    }
}
