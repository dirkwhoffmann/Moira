//
//  moira.cpp
//  Moira
//
//  Created by Dirk Hoffmann on 22.12.19.
//  Copyright © 2019 Dirk Hoffmann. All rights reserved.
//

#include "Moira.h"
#include "tester.h"

extern uint8_t mem[0x10000];
extern Tester_68k tester;

u8
Moira::read8(u32 addr)
{
    return sandbox.replayPeek(PEEK8, addr, getClock());
}

u16
Moira::read16(u32 addr)
{
    return sandbox.replayPeek(PEEK16, addr, moiracpu->getClock());
}

u16
Moira::readDasm(u32 addr)
{
    u16 res = mem[addr] << 8;
    res |= mem[addr + 1];
    return res;
}

u8
Moira::readIPL()
{
    return 0;
}

void
Moira::write8(u32 addr, u8  val)
{
    sandbox.replayPoke(POKE8, addr, getClock(), val);
    tester.memWrite2(addr, val);
}

void
Moira::write16 (u32 addr, u16 val)
{
    sandbox.replayPoke(POKE16, addr, moiracpu->getClock(), val);
    tester.memWordWrite2(addr, val);
}
