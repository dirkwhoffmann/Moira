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

CPU::CPU()
{
    init();
}


void
CPU::power()
{
    // Initialize data and address registers
    for(int i = 0; i < 8; i++) reg.d[i] = reg.a[i] = 0;
    reset();
}

void
CPU::reset()
{
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
    pc =
    memory->moiraReadAfterReset16(4) << 16 | memory->moiraReadAfterReset16(6);

    // Fill the prefetch queue
    irc = memory->moiraReadAfterReset16(pc);
    prefetch();
}

void
CPU::process(u16 reg_ird)
{
    pc += 2;
    (this->*exec[reg_ird])(reg_ird);
}

u16
CPU::getSR()
{
    return sr.t << 15 | sr.s << 13 | sr.ipl << 8 | getCCR();
}

void
CPU::setSR(u16 value)
{
    bool s = (value >> 13) & 1;
    u8 ipl = (value >>  8) & 7;

    sr.ipl = ipl;

    setCCR((u8)value);
    setSupervisorMode(s);
}

u8
CPU::getCCR()
{
    return sr.c << 0 | sr.v << 1 | sr.z << 2 | sr.n << 3 | sr.x << 4;
}

void
CPU::setCCR(u8 value)
{
    sr.c = (value >> 0) & 1;
    sr.v = (value >> 1) & 1;
    sr.z = (value >> 2) & 1;
    sr.n = (value >> 3) & 1;
    sr.x = (value >> 4) & 1;
}

void
CPU::setSupervisorMode(bool enable)
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
CPU::prefetch()
{
    ird = irc;
    irc = memory->moiraRead16(pc + 2);
}

void
CPU::fullPrefetch()
{
    assert(false);
}

void
CPU::readExtensionWord()
{
    pc += 2;
    irc = memory->moiraRead16(pc);
}

void
CPU::jumpToVector(u8 nr)
{
    // Update the program counter
    pc = read<Long>(4 * nr);

    // Update the prefetch queue
    ird = read<Word>(pc);
    irc = read<Word>(pc + 2);
}

void
CPU::disassemble(u32 addr, char *str, bool hex)
{
    u16 opcode = memory->moiraSpyRead16(addr);
    u16 ext1   = memory->moiraSpyRead16(addr + 2);
    u16 ext2   = memory->moiraSpyRead16(addr + 4);

    StrWriter writer{str, hex};

    (this->*dasm[opcode])(writer, addr, opcode);
    writer.finish();

    printf("%02x: [%04x,%04x,%04x] %s\n", addr, opcode, ext1, ext2, str);
}
