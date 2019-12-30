// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef MOIRA_BASE_H
#define MOIRA_BASE_H

#include "MoiraTypes.h"

namespace moira {

// Flags
#define SKIP_LAST_READ true

/*
template<Size>   u32  MSBIT();
template<Size>   u32  CLIP (u64 data);
template<Size>   u32  CLEAR(u64 data);
template<Size>   i32  SEXT (u64 data);
template<Size>   bool NBIT (u64 data);
template<Size>   bool CARRY(u64 data);
template<Size>   bool ZERO (u64 data);
template<Size S> u32  WRITE(u32 d1, u32 d2) { return CLEAR<S>(d1) | CLIP<S>(d2); }
*/

template<Size S> u32 MSBIT() {
    if (S == Byte) return 0x00000080;
    if (S == Word) return 0x00008000;
    if (S == Long) return 0x80000000;
}

template<Size S> u32 CLIP(u64 data) {
    if (S == Byte) return data & 0x000000FF;
    if (S == Word) return data & 0x0000FFFF;
    if (S == Long) return data & 0xFFFFFFFF;
}

template<Size S> u32 CLEAR(u64 data) {
    if (S == Byte) return data & 0xFFFFFF00;
    if (S == Word) return data & 0xFFFF0000;
    if (S == Long) return data & 0x00000000;
}

template<Size S> i32 SEXT(u64 data) {
    if (S == Byte) return (i8)data;
    if (S == Word) return (i16)data;
    if (S == Long) return (i32)data;
}

template<Size S> bool NBIT(u64 data) {
    if (S == Byte) return data & 0x00000080;
    if (S == Word) return data & 0x00008000;
    if (S == Long) return data & 0x80000000;
}

template<Size S> bool CARRY(u64 data) {
    if (S == Byte) return data & 0x000000100;
    if (S == Word) return data & 0x000010000;
    if (S == Long) return data & 0x100000000;
}

template<Size S> bool ZERO(u64 data) {
    if (S == Byte) return !(data & 0x000000FF);
    if (S == Word) return !(data & 0x0000FFFF);
    if (S == Long) return !(data & 0xFFFFFFFF);
}

template<Size S> u32 WRITE(u32 d1, u32 d2) {
    if (S == Byte) return (d1 & 0xFFFFFF00) | (d2 & 0x000000FF);
    if (S == Word) return (d1 & 0xFFFF0000) | (d2 & 0x0000FFFF);
    if (S == Long) return d2;
}

}
#endif
