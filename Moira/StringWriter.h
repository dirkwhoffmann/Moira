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

struct Align { int column; };
struct Dn { int value; };
struct An { int value; };
struct Rn { int value; const char symbol; };
struct Disp8 { i8 value; };
struct Disp16 { i16 value; };
struct Index { int value; };
struct RegList { u8 members; char symbol; };
template <Instr I> struct Ins { };
template <Cond C> struct Cnd { };
template <Size S> struct Sz { };
template <Size S> struct Im { u16 ext1; u16 ext2; };
template <Mode M, Size S> struct Ea { u16 reg; u16 ext1; u16 ext2; };

class StrWriter
{
    char *base;
    char *ptr;
    bool hex;

public:

    StrWriter(char *p, bool h) : base(p), ptr(p), hex(h) { };

    // StrWriter& operator<<(char c);
    StrWriter& operator<<(const char *str);
    StrWriter& operator<<(u8 value);
    StrWriter& operator<<(u16 value);
    StrWriter& operator<<(u32 value);
    StrWriter& operator<<(Align align);
    StrWriter& operator<<(Dn dn);
    StrWriter& operator<<(An an);
    StrWriter& operator<<(Rn rn);
    StrWriter& operator<<(Disp8 d);
    StrWriter& operator<<(Disp16 d);
    StrWriter& operator<<(Index i);
    StrWriter& operator<<(RegList l);
    template <Instr I> StrWriter& operator<<(Ins<I> i);
    template <Cond C> StrWriter& operator<<(Cnd<C> c);
    template <Size S> StrWriter& operator<<(Sz<S> sz);
    template <Size S> StrWriter& operator<<(Im<S> im);
    template <Mode M, Size S> StrWriter& operator<<(Ea<M,S> ea);

    void finish() { *ptr = 0; }
};


#endif
