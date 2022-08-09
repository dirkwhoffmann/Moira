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

// Numerical values
struct Int { i32 raw; Int(i32 v) : raw(v) { } };
struct UInt { u32 raw; UInt(u32 v) : raw(v) { } };
struct UInt8 { u8  raw; UInt8(u8  v) : raw(v) { } };
struct UInt16 { u16 raw; UInt16(u16 v) : raw(v) { } };
struct UInt32 { u32 raw; UInt32(u32 v) : raw(v) { } };

// Immediate operands
struct Imu { u32 raw; Imu(u32 v) : raw(v) { } };
template <Size S>
struct Ims { i32 raw; Ims(i32 v) : raw(v) { } };
struct Imd { u32 raw; Imd(u32 v) : raw(v) { } };

// Instruction names
template <Instr I> struct Ins { };
template <Size S> struct Sz { };

// Registers
struct Dn { int raw; Dn(int v) : raw(v) { } };
struct An { int raw; An(int v) : raw(v) { } };  // TODO: Rename to AnPc
struct Anr { int raw; Anr(int v) : raw(v) { } };  // TODO: Rename to An
struct Rn { int raw;  Rn(int v) : raw(v) { } };  // TODO: Rename to RnPc
struct Rnr { int raw; Rnr(int v) : raw(v) { } }; // TODO: Rename to Rn
struct Ccr { };
struct Sr { };
struct Usp { };

// Register lists
struct RegList { u16 raw; RegList(u16 v) : raw(v) { } };
struct RegRegList { u16 raw; RegRegList(u16 v) : raw(v) { } };

// Addressing modes
template <Mode M, Size S> struct Ea {
    u32 pc; u16 reg; u8 dw; u8 ow; u32 ext1; i32 ext2; i32 ext3;
};
template <Mode M, Size S> struct Ai { const Ea<M,S> &ea; };
template <Mode M, Size S> struct Pi { const Ea<M,S> &ea; };
template <Mode M, Size S> struct Pd { const Ea<M,S> &ea; };
template <Mode M, Size S> struct Di { const Ea<M,S> &ea; };
template <Mode M, Size S> struct Ix { const Ea<M,S> &ea; };
template <Mode M, Size S> struct Aw { const Ea<M,S> &ea; };
template <Mode M, Size S> struct Al { const Ea<M,S> &ea; };
template <Mode M, Size S> struct DiPc { const Ea<M,S> &ea; };
template <Mode M, Size S> struct Im { const Ea<M,S> &ea; };
template <Mode M, Size S> struct Ip { const Ea<M,S> &ea; };
template <Mode M, Size S> struct IxMus { const Ea<M,S> &ea; };
template <Mode M, Size S> struct IxMot { const Ea<M,S> &ea; };
template <Mode M, Size S> struct IxMit { const Ea<M,S> &ea; };

struct Scale { int raw; Scale(int v) : raw(v) { } };

struct Cn { u16 raw; Cn(u16 v) : raw(v) { } };
struct Cnd { int raw; Cnd(int v) : raw(v) { } };
struct Cpcc { int raw; Cpcc(int v) : raw(v) { } };

// Indentation
struct Tabulator { int raw;  Tabulator(int v) : raw(v) { } };

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

    StrWriter& operator<<(const char *);
    StrWriter& operator<<(int);
    StrWriter& operator<<(unsigned int);
    StrWriter& operator<<(Int);
    StrWriter& operator<<(UInt);
    StrWriter& operator<<(UInt8);
    StrWriter& operator<<(UInt16);
    StrWriter& operator<<(UInt32);
    StrWriter& operator<<(Dn);
    StrWriter& operator<<(An);
    StrWriter& operator<<(Anr);
    template <Mode M, Size S> StrWriter& operator<<(Ai<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(Pi<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(Pd<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(Di<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(Ix<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(Aw<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(Al<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(DiPc<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(Im<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(Ip<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(IxMus<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(IxMot<M,S>);
    template <Mode M, Size S> StrWriter& operator<<(IxMit<M,S>);

    StrWriter& operator<<(Rn);
    StrWriter& operator<<(Rnr);
    StrWriter& operator<<(Ccr);
    StrWriter& operator<<(Cn);
    StrWriter& operator<<(Cnd);
    StrWriter& operator<<(Cpcc);
    StrWriter& operator<<(Imu);
    template <Size S> StrWriter& operator<<(Ims<S>);
    StrWriter& operator<<(Imd);
    StrWriter& operator<<(Scale);
    StrWriter& operator<<(Sr);
    StrWriter& operator<<(Usp);
    StrWriter& operator<<(Tabulator);
    StrWriter& operator<<(RegRegList);
    StrWriter& operator<<(RegList);
    template <Instr I> StrWriter& operator<<(Ins<I>);
    template <Size S> StrWriter& operator<<(Sz<S>);
    template <Mode M, Size S> StrWriter& operator<<(const Ea<M,S> &);
    template <Instr I, Mode M, Size S> StrWriter& operator<<(const Av<I,M,S> &);
    StrWriter& operator<<(Sep);
    StrWriter& operator<<(Finish);

private:

    // DEPRECATED
    template <Mode M, Size S> void briefExtension(const Ea<M,S> &);
    template <Mode M, Size S> void fullExtension(const Ea<M,S> &);
    template <Mode M, Size S> void fullExtensionVda68k(const Ea<M,S> &);
};

}
