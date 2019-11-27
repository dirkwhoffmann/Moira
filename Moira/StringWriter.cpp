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

static void sprintd_signed(char *&s, i64 value)
{
    if (value < 0) { *s++ = '-'; value *= -1; }
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

static void sprintx_signed(char *&s, i64 value)
{
    if (value < 0) { *s++ = '-'; value *= -1; }
    sprintx(s, value, hexDigits(value));
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
    // assert(dn.value < 8);

    *ptr++ = 'D';
    *ptr++ = '0' + dn.value;
    return *this;
}

StrWriter&
StrWriter::operator<<(An an)
{
    *ptr++ = 'A';
    *ptr++ = '0' + an.value;
    return *this;
}

StrWriter&
StrWriter::operator<<(Disp8 d)
{
    hex ? sprintx_signed(ptr, d.value) : sprintd_signed(ptr, d.value);
    return *this;
}

StrWriter&
StrWriter::operator<<(Disp16 d)
{
    hex ? sprintx_signed(ptr, d.value) : sprintd_signed(ptr, d.value);
    return *this;
}

StrWriter&
StrWriter::operator<<(Index v)
{
    if (v.value < 8) {
        *ptr++ = 'D'; *ptr++ = '0' + v.value;
    } else {
        *ptr++ = 'A'; *ptr++ = '0' + v.value - 8;
    }
    return *this;
}

StrWriter&
StrWriter::operator<<(Sz sz)
{
    switch (sz.s) {

        case Byte: *this << ".b"; break;
        case Word: *this << ".s"; break;
        case Long: *this << ".l"; break;
        default:   assert(false);
    }
    return *this;
}

StrWriter&
StrWriter::operator<<(Im im)
{
    *this << "#";
    switch (im.s) {
        case Byte: *this << (u8)im.ext1; break;
        case Word: *this << im.ext1; break;
        case Long: *this << ((u32)(im.ext1 << 16) | im.ext2); break;
        default:   assert(false);
    }
    return *this;
}

StrWriter&
StrWriter::operator<<(Ea ea)
{
    switch (ea.m) {

        case 0: // Dn
            *this << Dn{ea.opcode & 7};
            break;

        case 1: // An
            *this << An{ea.opcode & 7};
            break;

        case 2: // (An)
            *this << "(" << An{ea.opcode & 7} << ")";
            break;

        case 3: // (An)+
            *this << "(" << An{ea.opcode & 7} << ")+";
            break;

        case 4: // -(An)
            *this << "-(" << An{ea.opcode & 7} << ")";
            break;

        case 5: // (d,An)
            *this << "(" << Disp16{(i16)ea.ext1};
            *this << "," << An{ea.opcode & 7} << ")";
            break;

        case 6: // (d,An,Xi)
            *this << "(" << Disp8{(i8)ea.ext1};
            *this << "," << An{ea.opcode & 7};
            *this << "," << Index{ea.ext1 >> 12};
            *this << ((ea.ext1 & 0x800) ? ".L)" : ".W)");
            break;

        case 7: // ABS.W
            *this << ea.ext1 << ".w";
            break;

        case 8: // ABS.L
            *this << (u32)(ea.ext1 << 16 | ea.ext2) << ".l";
            break;

        case 9: // (d,PC)
            *this << "(" << Disp16{(i16)ea.ext1} << ",PC)";
            break;

        case 10: // (d,PC,Xi)
            *this << "(" << Disp8{(i8)ea.ext1};
            *this << ",PC," << Index{ea.ext1 >> 12};
            *this << ((ea.ext1 & 0x800) ? ".L)" : ".W)");
            break;

        case 11: // Imm
            assert(false);
            break;

        default:
            assert(false);
            }

            return *this;
            }
