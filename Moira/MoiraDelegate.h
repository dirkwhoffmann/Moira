// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef MOIRA_DELEGATE_H
#define MOIRA_DELEGATE_H

#include "types.h"

using namespace moira;

class MoiraDelegate {

public:

    virtual u8 moiraRead8(u32 addr) = 0;
    virtual u16 moiraRead16(u32 addr) = 0;
    virtual void moiraWrite8(u32 addr, u8 value) = 0;
    virtual void moiraWrite16(u32 addr, u16 value) = 0;

    virtual u16 moiraSpyRead16(u32 addr) = 0;
    virtual u16 moiraReadAfterReset16(u32 addr) = 0;
};

#endif
