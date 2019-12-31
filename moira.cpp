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
        return mem[addr & 0xFFFF];
    } else {
        return sandbox.replayPeek(PEEK8, addr, getClock());
    }
}

u16
Moira::read16(u32 addr)
{
    if (MUSASHI) {
        return mem[addr & 0xFFFF] << 8 | mem[(addr + 1) & 0xFFFF];
    } else {
        return sandbox.replayPeek(PEEK16, addr, moiracpu->getClock());
    }
}

u16
Moira::read16Dasm(u32 addr)
{
    assert(MUSASHI);
    return mem[addr & 0xFFFF] << 8 | mem[(addr + 1) & 0xFFFF];
}

u16
Moira::read16Reset(u32 addr)
{
    if (MUSASHI) {
        switch (addr) {
            case 0: return 0x0000;
            case 2: return 0x2000;
            case 4: return 0x0000;
            case 6: return 0x1000;
        }
        return mem[addr & 0xFFFF] << 8 | mem[(addr + 1) & 0xFFFF];
    } else {
        return sandbox.replayPeek(PEEK16, addr, getClock());
    }
}

u8
Moira::readIPL()
{
    if (MUSASHI) {
        return 0;
    } else {
        return sandbox.replayPoll(getClock());
    }
}

void
Moira::write8(u32 addr, u8  val)
{
    sandbox.replayPoke(POKE8, addr, getClock(), val);
}

void
Moira::write16 (u32 addr, u16 val)
{
    sandbox.replayPoke(POKE16, addr, moiracpu->getClock(), val);
}
