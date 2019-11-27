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

struct Dn { int value; };
struct An { int value; };
struct Disp8 { i8 value; };
struct Disp16 { i16 value; };
struct Index { int value; };
struct Sz { Size s; };
struct Im { Size s; u16 ext1; u16 ext2; };
struct Ea { Mode m; u16 opcode; u16 ext1; u16 ext2; };

class StrWriter
{
    char *ptr;
    bool hex;

public:

    StrWriter(char *p, bool h) : ptr(p), hex(h) { };

    // StrWriter& operator<<(char c);
    StrWriter& operator<<(const char *str);
    StrWriter& operator<<(u8 value);
    StrWriter& operator<<(u16 value);
    StrWriter& operator<<(u32 value);
    StrWriter& operator<<(Dn dn);
    StrWriter& operator<<(An an);
    StrWriter& operator<<(Disp8 d);
    StrWriter& operator<<(Disp16 d);
    StrWriter& operator<<(Index i);
    StrWriter& operator<<(Sz sz);
    StrWriter& operator<<(Im im);
    StrWriter& operator<<(Ea ea);

    void finish() { *ptr = 0; }
};


#endif
