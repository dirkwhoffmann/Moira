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
#include "MoiraTable.tpp"
#include "MoiraLogic.tpp"
#include "MoiraExec.tpp"
#include "MoiraDasm.tpp"
#include "MoiraSync.tpp"

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

