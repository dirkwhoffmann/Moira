// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <assert.h>

#include "Moira.h"
#include "MoiraInit.cpp"
#include "MoiraLogic.cpp"
#include "MoiraExec.cpp"
#include "MoiraDasm.cpp"
#include "MoiraSync.cpp"

Moira::Moira()
{
    init();
}


void
Moira::power()
{
    printf("Moira::power()\n");
    // Initialize data and address registers
    for(int i = 0; i < 8; i++) reg.d[i] = reg.a[i] = 0;
    reset();
}

void
Moira::reset()
{
    printf("Moira::reset()\n");

    // Reset the status register
    sr.t = 0;
    sr.s = 1;
    sr.x = 0;
    sr.n = 0;
    sr.z = 0;
    sr.v = 0;
    sr.c = 0;
    sr.ipl = 7;

    // Read the initial stack pointer from memory
    reg.usp = reg.usp =
    memory->moiraReadAfterReset16(0) << 16 | memory->moiraReadAfterReset16(2);

    // Read the initial program counter from memory
    reg.pc =
    memory->moiraReadAfterReset16(4) << 16 | memory->moiraReadAfterReset16(6);

    // Fill the prefetch queue
    irc = memory->moiraReadAfterReset16(reg.pc);
    prefetch();
}

void
Moira::process(u16 reg_ird)
{
    reg.pc += 2;
    (this->*exec[reg_ird])(reg_ird);
}

u16
Moira::getSR()
{
    return sr.t << 15 | sr.s << 13 | sr.ipl << 8 | getCCR();
}

void
Moira::setSR(u16 value)
{
    bool s = (value >> 13) & 1;
    u8 ipl = (value >>  8) & 7;

    sr.ipl = ipl;

    setCCR((u8)value);
    setSupervisorMode(s);
}

u8
Moira::getCCR()
{
    return sr.c << 0 | sr.v << 1 | sr.z << 2 | sr.n << 3 | sr.x << 4;
}

void
Moira::setCCR(u8 value)
{
    sr.c = (value >> 0) & 1;
    sr.v = (value >> 1) & 1;
    sr.z = (value >> 2) & 1;
    sr.n = (value >> 3) & 1;
    sr.x = (value >> 4) & 1;
}

void
Moira::setSupervisorMode(bool enable)
{
    if (sr.s == enable) return;

    if (enable) {
        sr.s = 1;
        reg.usp = reg.a[7];
        reg.a[7] = reg.ssp;
    } else {
        sr.s = 0;
        reg.ssp = reg.a[7];
        reg.a[7] = reg.usp;
    }
}

void
Moira::prefetch()
{
    ird = irc;
    irc = memory->moiraRead16(reg.pc + 2);
}

void
Moira::fullPrefetch()
{
    assert(false);
}

void
Moira::readExtensionWord()
{
    reg.pc += 2;
    irc = memory->moiraRead16(reg.pc);
}

void
Moira::jumpToVector(u8 nr)
{
    // Update the program counter
    reg.pc = read<Long>(4 * nr);

    // Update the prefetch queue
    ird = read<Word>(reg.pc);
    irc = read<Word>(reg.pc + 2);
}

int
Moira::disassemble(u32 addr, char *str, bool hex)
{
    u32 pc     = addr;
    u16 opcode = memory->moiraSpyRead16(pc);

    StrWriter writer(str, hex);

    (this->*dasm[opcode])(writer, pc, opcode);
    writer << Finish{};

    return pc - addr + 2;
}

