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
    // Initialize data and address registers
    for(int i = 0; i < 8; i++) reg.d[i] = reg.a[i] = 0;
    reset();
}

void
Moira::reset()
{
    // Reset the status register
    sr.c = 0;
    sr.v = 0;
    sr.z = 0;
    sr.n = 0;
    sr.x = 0;
    sr.s = 1;
    sr.ipl = 7;

    // Read the initial stack pointer from memory
    reg.usp = reg.usp =
    memory->moiraReadAfterReset16(0) << 16 | memory->moiraReadAfterReset16(2);

    // Read the initial program counter from memory
    pc =
    memory->moiraReadAfterReset16(4) << 16 | memory->moiraReadAfterReset16(6);

    // Fill the prefetch queue
    irc = memory->moiraRead16(pc);
    prefetch();
}

void
Moira::process(u16 reg_ird)
{
    printf("Processing opcode %x\n", reg_ird);

    pc += 2;
    (this->*exec[reg_ird])(reg_ird);
}

u16
Moira::getSR()
{
    return (sr.s << 13) | (sr.ipl << 8) | getCCR();
}

void
Moira::setSR(u16 value)
{
    bool s   = (value >> 13) & 1;
    bool ipl = (value >>  8) & 7;

    sr.s = s;
    sr.ipl = ipl;

    if (sr.s ^ s) {
        // Supervisior flag has changed
    }
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
Moira::prefetch()
{
    ird = irc;
    irc = memory->moiraRead16(pc + 2);
}

void
Moira::readExtensionWord()
{
    pc += 2;
    irc = memory->moiraRead16(pc);
}

void
Moira::disassemble(u32 addr, char *str, bool hex)
{
    u16 opcode = memory->moiraSpyRead16(addr);
    u16 ext1   = memory->moiraSpyRead16(addr + 2);
    u16 ext2   = memory->moiraSpyRead16(addr + 4);

    StrWriter writer{str, hex};

    printf("disassemble [%x,%x,%x] at %x\n", opcode, ext1, ext2, addr);

    (this->*dasm[opcode])(writer, opcode, ext1, ext2);
    writer.finish();
}
