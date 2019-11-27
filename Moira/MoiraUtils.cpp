// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "MoiraUtils.h"

template<> u32 CLIP<Byte>(u64 data) { return data & 0xff; }
template<> u32 CLIP<Word>(u64 data) { return data & 0xffff; }
template<> u32 CLIP<Long>(u64 data) { return data & 0xffffffff; }

/*
template<> u64 MASK<Byte>() { return 0xff; }
template<> u64 MASK<Word>() { return 0xffff; }
template<> u64 MASK<Long>() { return 0xffffffff; }
*/

template<> bool MSBIT<Byte>(u64 data) { return data & 0x80; }
template<> bool MSBIT<Word>(u64 data) { return data & 0x8000; }
template<> bool MSBIT<Long>(u64 data) { return data & 0x80000000; }

template<> u32 SIGN<Byte>(u64 data) { return  (i8)data; }
template<> u32 SIGN<Word>(u64 data) { return (i16)data; }
template<> u32 SIGN<Long>(u64 data) { return (i32)data; }

template<> u8 BYTES<Byte>() { return 1; }
template<> u8 BYTES<Word>() { return 2; }
template<> u8 BYTES<Long>() { return 4; }
