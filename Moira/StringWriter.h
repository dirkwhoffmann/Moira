// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef STRING_WRITER_H
#define STRING_WRITER_H

#include "MoiraDelegate.h"
#include "MoiraUtils.h"

struct Dn { int n; };
struct An { int n; };
struct Ea { Mode m; uint32_t addr; uint16_t opcode; };

class StrWriter
{
    char *ptr;
    bool hex;

public:

    StrWriter(char *p, bool h) : ptr(p), hex(h) { };

    StrWriter& operator<<(char c);
    StrWriter& operator<<(const char *str);
    StrWriter& operator<<(uint8_t value);
    StrWriter& operator<<(uint16_t value);
    StrWriter& operator<<(uint32_t value);
    StrWriter& operator<<(Dn dn);
    StrWriter& operator<<(An an);
    StrWriter& operator<<(Ea ea);

    void finish() { *ptr = 0; }
};


#endif
