//
//  moira.cpp
//  Moira
//
//  Created by Dirk Hoffmann on 22.12.19.
//  Copyright © 2019 Dirk Hoffmann. All rights reserved.
//

#include "Moira.h"
#include "tester.h"
#include "globals.h"

using namespace moira;

u8
Moira::read8(u32 addr)
{
    return mem[addr & 0xFFFF];
    // return sandbox.replayPeek(PEEK8, addr, getClock());
}

u16
Moira::read16(u32 addr)
{
    return mem[addr & 0xFFFFFF] << 8 | mem[(addr + 1) & 0xFFFFFF];
    // return sandbox.replayPeek(PEEK16, addr, moiracpu->getClock());
}

u16
Moira::read16Dasm(u32 addr)
{
    return mem[addr & 0xFFFFFF] << 8 | mem[(addr + 1) & 0xFFFFFF];
}

u16
Moira::read16Reset(u32 addr)
{
    switch (addr) {
        case 0: return 0x0000;
        case 2: return 0x2000;
        case 4: return 0x0000;
        case 6: return 0x1000;
    }
    // return sandbox.replayPeek(PEEK16, addr, getClock());
    return mem[addr & 0xFFFFFF] << 8 | mem[(addr + 1) & 0xFFFFFF];
}

u8
Moira::readIPL()
{
    return 0;
    // return sandbox.replayPoll(getClock()); 
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
