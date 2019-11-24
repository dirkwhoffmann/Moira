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
struct Ea { Mode m; u16 opcode; u16 ext1; u16 ext2; };

class StrWriter
{
    char *ptr;
    bool hex;

public:

    StrWriter(char *p, bool h) : ptr(p), hex(h) { };

    StrWriter& operator<<(char c);
    StrWriter& operator<<(const char *str);
    StrWriter& operator<<(u8 value);
    StrWriter& operator<<(u16 value);
    StrWriter& operator<<(u32 value);
    StrWriter& operator<<(Dn dn);
    StrWriter& operator<<(An an);
    StrWriter& operator<<(Ea ea);

    void finish() { *ptr = 0; }
};


#endif
