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
    for(int i = 0; i < 8; i++) reg.d[i] = 0;
    reg.d[0] = reg.d[1];
    reset();
}

void
Moira::reset()
{
    /* portable68
       status_code.reset();

        irqPendingLevel = irqSamplingLevel = 0;

        rmwCycle = false;
        stop = false;
        doubleFault = false;
        trace = false;
        illegalMode = NONE;
        reg_s = 0x2700;
        sync(16);
        reg_a[7] = reg_ssp = readLong(0);
        reg_pc = readLong(4);
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
    irc = delegate->moiraRead16(pc + 2);
}

void
Moira::readExtensionWord()
{
    pc += 2;
    irc = delegate->moiraRead16(pc);

}

void
Moira::disassemble(uint16_t addr, char *str)
{
    printf("disassemble %x\n", addr);

    uint16_t opcode = 42; // read16()

    (this->*dasm[opcode])(opcode, str, true);
}
