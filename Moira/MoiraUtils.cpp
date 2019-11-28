// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "MoiraUtils.h"

template<> u32 CLIP<Byte>(u32 data) { return data & 0xff; }
template<> u32 CLIP<Word>(u32 data) { return data & 0xffff; }
template<> u32 CLIP<Long>(u32 data) { return data; }

template<> u32 CLEAR<Byte>(u32 data) { return data & 0xffffff00; }
template<> u32 CLEAR<Word>(u32 data) { return data & 0xffff0000; }
template<> u32 CLEAR<Long>(u32 data) { return 0; }

// template<Size S> u32 SUBS(u32 d1, u32 d2) { return CLEAR<S>(d1) | CLIP<S>(d2); }


/*
template<> u64 MASK<Byte>() { return 0xff; }
template<> u64 MASK<Word>() { return 0xffff; }
template<> u64 MASK<Long>() { return 0xffffffff; }
*/

template<> bool MSBIT<Byte>(u32 data) { return data & 0x80; }
template<> bool MSBIT<Word>(u32 data) { return data & 0x8000; }
template<> bool MSBIT<Long>(u32 data) { return data & 0x80000000; }

template<> i32 SIGN<Byte>(u32 data) { return  (i8)data; }
template<> i32 SIGN<Word>(u32 data) { return (i16)data; }
template<> i32 SIGN<Long>(u32 data) { return (i32)data; }

template<> u8 BYTES<Byte>() { return 1; }
template<> u8 BYTES<Word>() { return 2; }
template<> u8 BYTES<Long>() { return 4; }
