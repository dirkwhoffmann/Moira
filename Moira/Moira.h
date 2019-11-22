// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include "MoiraBase.h"

class Moira {

    // Jump table storing all instruction handlers
    void (Moira::*exec[65536])(uint16_t);

    // Jump table storing all disassebler handlers
    void (Moira::*dasm[65536])(uint16_t, char *, bool);

    // Jump table storing all time information handlers
    int (Moira::*sync[65536])(uint16_t, int);

    // Disassembler
    bool hex = true;
    char str[256];

    // Instruction logic
#include "MoiraLogic.h"

    // Instruction handlers
#include "MoiraExec.h"
#include "MoiraDasm.h"
#include "MoiraSync.h"


public:
    
    Moira();

    void init();
    void process(uint16_t reg_ird);


    //
    // Running the disassembler
    //

    void disassemble(uint16_t addr, char *str);
};
