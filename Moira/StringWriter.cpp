// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "StringWriter.h"
#include "assert.h"
#include <math.h>
#include <stdio.h>


static int decDigits(u64 value) { return value ? 1 + log10(value) : 1; }
static int binDigits(u64 value) { return value ? 1 + log2(value) : 1; }
static int hexDigits(u64 value) { return (binDigits(value) + 3) / 4; }

static void sprintd(char *&s, u64 value, int digits)
{
    for (int i = digits - 1; i >= 0; i--) {
        u8 digit = value % 10;
        s[i] = '0' + digit;
        value /= 10;
    }
    s += digits;
}

static void sprintd(char *&s, u64 value)
{
    sprintd(s, value, decDigits(value));
}

static void sprintx(char *&s, u64 value, int digits)
{
    *s++ = '$';
    for (int i = digits - 1; i >= 0; i--) {
        u8 digit = value % 16;
        s[i] = (digit <= 9) ? ('0' + digit) : ('a' + digit - 10);
        value /= 16;
    }
    s += digits;
}

static void sprintx(char *&s, u64 value)
{
    sprintx(s, value, hexDigits(value));
}

StrWriter&
StrWriter::operator<<(char c)
{
    *ptr++ = c;
    return *this;
}

StrWriter&
StrWriter::operator<<(const char *str)
{
    while (*str) { *ptr++ = *str++; };
    return *this;
}

StrWriter&
StrWriter::operator<<(u8 value)
{
    hex ? sprintx(ptr, value) : sprintd(ptr, value);
    return *this;
}

StrWriter&
StrWriter::operator<<(u16 value)
{
    hex ? sprintx(ptr, value) : sprintd(ptr, value);
    return *this;
}

StrWriter&
StrWriter::operator<<(u32 value)
{
    hex ? sprintx(ptr, value) : sprintd(ptr, value);
    return *this;
}

StrWriter&
StrWriter::operator<<(Dn dn)
{
    assert(dn.n < 8);
    *ptr++ = 'D'; *ptr++ = '0' + dn.n;
    return *this;
}

StrWriter&
StrWriter::operator<<(An an)
{
    assert(an.n < 8);
    *ptr++ = 'A'; *ptr++ = '0' + an.n;
    return *this;
}

StrWriter&
StrWriter::operator<<(Ea ea)
{
    switch (ea.m) {

        case 0: // Dn
            *this << Dn{ ea.opcode & 7 };
            break;

        case 1: // An
            *this << An{ ea.opcode & 7 };
            break;

        case 2: // (An)
            *this << "(" << An{ ea.opcode & 7 } << ")";
            break;

        case 3: // (An)+
            *this << "(" << An{ ea.opcode & 7 } << ")+";
            break;

        case 4: // -(An)
            *this << "-(" << An{ ea.opcode & 7 } << ")";
            break;

        case 5: // (d,An)
            assert(false);
            break;

        case 6: // (d,An,Xi)
            assert(false);
            break;

        case 7: // ABS.W
            *this << ea.ext1 << ".w";
            break;

        case 8: // ABS.L
            printf("ext1 = %x ext2 = %x %x\n", ea.ext1, ea.ext2, (u32)(ea.ext1 << 16 | ea.ext2));
            *this << (u32)(ea.ext1 << 16 | ea.ext2) << ".l";
            break;

        case 9: // (d,PC)
            assert(false);
            break;

        case 10: // (d,PC,Xi)
            assert(false);
            break;

        case 11: // Imm
            assert(false);
            break;

        default:
            assert(false);
    }

    return *this;
}
