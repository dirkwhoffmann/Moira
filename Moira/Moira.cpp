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
    // Reset status register flags
    flags.c = 0;
    flags.v = 0;
    flags.z = 0;
    flags.n = 0;
    flags.x = 0;
    flags.s = 1;

    // Reset Interrupt Priority Level
    ipl = 7;

    // Read the initial stack pointers from memory0
    reg.usp = reg.usp =
    memory->moiraReadAfterReset16(0) << 16 | memory->moiraReadAfterReset16(2);

    // Read the initial program counter from memory
    pc =
    memory->moiraReadAfterReset16(4) << 16 | memory->moiraReadAfterReset16(6);

    // Perform a full prefetch cycle

    // TODO

    //
    /* portable68

        fullprefetch();
     */
}

void
Moira::process(uint16_t reg_ird)
{
    printf("Processing opcode: %X\n", reg_ird);

    (this->*exec[reg_ird])(reg_ird);
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
Moira::disassemble(uint16_t addr, char *str)
{
    printf("disassemble %x\n", addr);

    uint16_t opcode = 42; // read16()

    (this->*dasm[opcode])(opcode, str, true);
}
