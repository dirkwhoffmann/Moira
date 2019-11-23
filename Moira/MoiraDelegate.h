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

#include <stdint.h>

class MoiraDelegate {

    virtual uint8_t moiraRead8(uint32_t addr) = 0;
    virtual uint16_t moiraRead16(uint32_t addr) = 0;
    virtual void moiraWrite8(uint32_t addr, uint8_t value) = 0;
    virtual void moiraWrite16(uint32_t addr, uint16_t value) = 0;
};

#endif
