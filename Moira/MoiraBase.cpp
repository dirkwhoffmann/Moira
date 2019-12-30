// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "MoiraBase.h"

namespace moira {


template<> u32  MSBIT <Byte> ()         { return 0x00000080; }
template<> u32  MSBIT <Word> ()         { return 0x00008000; }
template<> u32  MSBIT <Long> ()         { return 0x80000000; }

template<> i32  SIGN  <Byte> (u64 data) { return (i8)data; }
template<> i32  SIGN  <Word> (u64 data) { return (i16)data; }
template<> i32  SIGN  <Long> (u64 data) { return (i32)data; }

template<> u32  CLIP  <Byte> (u64 data) { return data & 0x000000FF; }
template<> u32  CLIP  <Word> (u64 data) { return data & 0x0000FFFF; }
template<> u32  CLIP  <Long> (u64 data) { return data & 0xFFFFFFFF; }

template<> u32  CLEAR <Byte> (u64 data) { return data & 0xFFFFFF00; }
template<> u32  CLEAR <Word> (u64 data) { return data & 0xFFFF0000; }
template<> u32  CLEAR <Long> (u64 data) { return data & 0x00000000; }

template<> u32  SEXT  <Byte> (u64 data) { return (i8)data;          }
template<> u32  SEXT  <Word> (u64 data) { return (i16)data;         }
template<> u32  SEXT  <Long> (u64 data) { return (i32)data;         }

template<> bool NBIT  <Byte> (u64 data) { return data & 0x00000080; }
template<> bool NBIT  <Word> (u64 data) { return data & 0x00008000; }
template<> bool NBIT  <Long> (u64 data) { return data & 0x80000000; }

template<> bool CARRY <Byte> (u64 data) { return data & 0x000000100; }
template<> bool CARRY <Word> (u64 data) { return data & 0x000010000; }
template<> bool CARRY <Long> (u64 data) { return data & 0x100000000; }

template<> bool ZERO  <Byte> (u64 data) { return (data & 0x000000FF) == 0; }
template<> bool ZERO  <Word> (u64 data) { return (data & 0x0000FFFF) == 0; }
template<> bool ZERO  <Long> (u64 data) { return (data & 0xFFFFFFFF) == 0; }

}

