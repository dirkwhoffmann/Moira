// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "testrunner.h"


bool isMul(uint16_t opcode)
{
    bool result = false;

    if ((opcode & 0b1111000111000000) == 0b1100000111000000) result = true;
    if ((opcode & 0b1111000111000000) == 0b1100000011000000) result = true;

    return result;
}

bool isDiv(uint16_t opcode)
{
    if ((opcode & 0b1111000111000000) == 0b1000000111000000) return true;
    if ((opcode & 0b1111000111000000) == 0b1000000011000000) return true;
    return false;
}

bool isMulOrDiv(uint16_t opcode)
{
    return isDiv(opcode) || isMul(opcode);
}

bool isAbcd(uint16_t opcode)
{
    if ((opcode & 0b1111000111111000) == 0b1000000100000000) return true;
    if ((opcode & 0b1111000111111000) == 0b1000000100001000) return true;
    return false;
}

bool isSbcd(uint16_t opcode)
{
    if ((opcode & 0b1111000111111000) == 0b1100000100000000) return true;
    if ((opcode & 0b1111000111111000) == 0b1100000100001000) return true;
    return false;
}

bool isNbcd(uint16_t opcode)
{
    if ((opcode & 0b1111111111000000) == 0b0100100000000000) return true;
    return false;
}

bool isBcd(uint16_t opcode)
{
    return isAbcd(opcode) || isSbcd(opcode) || isNbcd(opcode);
}
