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
StrWriter::operator<<(Align align)
{
    while (ptr < base + align.column) *ptr++ = ' ';
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

template <Instr I> StrWriter&
StrWriter::operator<<(Ins<I> i)
{
    *this << instrStr[I];
    return *this;
}

template <Cond C> StrWriter&
StrWriter::operator<<(Cnd<C> c)
{
    *this << condStr[C];
    return *this;
}

template <> StrWriter&
StrWriter::operator<<(Sz<Byte>)
{
    *this << ".b";
    return *this;
}

template <> StrWriter&
StrWriter::operator<<(Sz<Word>)
{
    *this << ".w";
    return *this;
}

template <> StrWriter&
StrWriter::operator<<(Sz<Long>)
{
    *this << ".l";
    return *this;
}

template <> StrWriter&
StrWriter::operator<<(Im<Byte> im)
{
    *this << "#" << (u8)im.ext1;
    return *this;
}

template <> StrWriter&
StrWriter::operator<<(Im<Word> im)
{
    *this << "#" << im.ext1;
    return *this;
}

template <> StrWriter&
StrWriter::operator<<(Im<Long> im)
{
    *this << "#" << ((u32)(im.ext1 << 16) | im.ext2);
    return *this;
}

template <Mode M, Size S> StrWriter&
StrWriter::operator<<(Ea<M,S> ea)
{
    switch (M) {

        case 0: // Dn
        {
            *this << Dn{ea.reg};
            return *this;
        }
        case 1: // An
        {
            *this << An{ea.reg};
            return *this;
        }
        case 2: // (An)
        {
            *this << "(" << An{ea.reg} << ")";
            return *this;
        }
        case 3:  // (An)+
        {
            *this << "(" << An{ea.reg} << ")+";
            return *this;
        }
        case 4: // -(An)
        {
            *this << "-(" << An{ea.reg} << ")";
            return *this;
        }
        case 5: // (d,An)
        {
            *this << "(" << Disp16{(i16)ea.ext1};
            *this << "," << An{ea.reg} << ")";
            return *this;
        }
        case 6: // (d,An,Xi)
        {
            *this << "(" << Disp8{(i8)ea.ext1};
            *this << "," << An{ea.reg};
            *this << "," << Index{ea.ext1 >> 12};
            *this << ((ea.ext1 & 0x800) ? ".l)" : ".w)");
            return *this;
        }
        case 7: // ABS.W
        {
            *this << ea.ext1;
            *this << ".w";
            return *this;
        }
        case 8: // ABS.L
        {
            *this << (u32)(ea.ext1 << 16 | ea.ext2);
            *this << ".l";
            return *this;
        }
        case 9: // (d,PC)
        {
            *this << "(" << Disp16{(i16)ea.ext1} << ",PC)";
            return *this;
        }
        case 10: // (d,PC,Xi)
        {
            *this << "(" << Disp8{(i8)ea.ext1};
            *this << ",PC," << Index{ea.ext1 >> 12};
            *this << ((ea.ext1 & 0x800) ? ".l)" : ".w)");
            return *this;
        }
        case 11: // Imm
        {
            switch (S) {
                case Byte: *this << "#" << (u8)ea.ext1; break;
                case Word: *this << "#" << ea.ext1; break;
                case Long: *this << "#" << ((u32)(ea.ext1 << 16) | ea.ext2); break;
            }
            return *this;
        }
    }
}
