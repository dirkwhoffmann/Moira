// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include <cmath>

namespace moira {

//
// Wrapper structures controlling the output format
//

struct Int        { i32 raw;        Int(i32 v) : raw(v) { } };
struct UInt       { u32 raw;       UInt(u32 v) : raw(v) { } };
struct UInt8      { u8  raw;      UInt8(u8  v) : raw(v) { } };
struct UInt16     { u16 raw;     UInt16(u16 v) : raw(v) { } };
struct UInt32     { u32 raw;     UInt32(u32 v) : raw(v) { } };
struct Dn         { int raw;         Dn(int v) : raw(v) { } };
struct An         { int raw;         An(int v) : raw(v) { } };
struct Anr        { int raw;        Anr(int v) : raw(v) { } };
struct Rn         { int raw;         Rn(int v) : raw(v) { } };
struct Rnr        { int raw;        Rnr(int v) : raw(v) { } };
struct Cn         { u16 raw;         Cn(u16 v) : raw(v) { } };
struct Cnd        { int raw;        Cnd(int v) : raw(v) { } };
struct Cpcc       { int raw;       Cpcc(int v) : raw(v) { } };
struct Imu        { u32 raw;        Imu(u32 v) : raw(v) { } };
// struct Ims        { i32 raw;        Ims(i32 v) : raw(v) { } };
template <Size S> struct Ims { i32 raw; Ims(i32 v) : raw(v) { } };
struct Imd        { u32 raw;        Imd(u32 v) : raw(v) { } };
struct Scale      { int raw;      Scale(int v) : raw(v) { } };
struct Align      { int raw;      Align(int v) : raw(v) { } };
struct RegList    { u16 raw;    RegList(u16 v) : raw(v) { } };
struct RegRegList { u16 raw; RegRegList(u16 v) : raw(v) { } };
template <Instr I> struct Ins { };
template <Size S> struct Sz { };
template <Mode M, Size S> struct Ea {
    u32 pc; u16 reg; u8 dw; u8 ow; u32 ext1; i32 ext2; i32 ext3;
};
template <Instr I, Mode M, Size S> struct Av { u32 ext1 = 0; };

struct Sep        { };
struct Finish     { };

class StrWriter
{
    
public:

    char comment[32];       // Appended to the end of the disassembled string
    Moira *moira;
    char *base;             // Start address of the destination string
    char *ptr;              // Current writing position
    DasmStyle style;
    DasmNumberFormat nf;

public:

    StrWriter(Moira *m, char *p, DasmStyle s, DasmNumberFormat n) {

        moira = m;
        base = ptr = p;
        style = s;
        nf = n;
        comment[0] = 0;
    };

    //
    // Printing instruction fragments
    //

    StrWriter& operator<<(const char *str);
    StrWriter& operator<<(int i);
    StrWriter& operator<<(unsigned int i);
    StrWriter& operator<<(Int i);
    StrWriter& operator<<(UInt u);
    StrWriter& operator<<(UInt8 value);
    StrWriter& operator<<(UInt16 value);
    StrWriter& operator<<(UInt32 value);
    StrWriter& operator<<(Dn dn);
    StrWriter& operator<<(An an);
    StrWriter& operator<<(Anr an);
    StrWriter& operator<<(Rn rn);
    StrWriter& operator<<(Rnr rn);
    StrWriter& operator<<(Cn cn);
    StrWriter& operator<<(Cnd cnd);
    StrWriter& operator<<(Cpcc cpcc);
    StrWriter& operator<<(Imu im);
    template <Size S> StrWriter& operator<<(Ims<S> im);
    StrWriter& operator<<(Imd im);
    StrWriter& operator<<(Scale s);
    StrWriter& operator<<(Align align);
    StrWriter& operator<<(RegRegList l);
    StrWriter& operator<<(RegList l);
    template <Instr I> StrWriter& operator<<(Ins<I> i);
    template <Size S> StrWriter& operator<<(Sz<S> sz);
    template <Mode M, Size S> StrWriter& operator<<(const Ea<M,S> &ea);
    template <Instr I, Mode M, Size S> StrWriter& operator<<(const Av<I,M,S> &av);
    StrWriter& operator<<(Sep sep);
    StrWriter& operator<<(Finish finish);

private:

    template <Mode M, Size S> void briefExtension(const Ea<M,S> &ea);
    template <Mode M, Size S> void fullExtension(const Ea<M,S> &ea);
    template <Mode M, Size S> void fullExtensionVda68k(const Ea<M,S> &ea);
};

}
