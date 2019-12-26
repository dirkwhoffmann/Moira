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
#include "stdint.h"

namespace moira {

// Integer data types
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using f32 = float;
using f64 = double;

// Flags
#define SKIP_LAST_READ true

template<Size>   u32  MASK();
template<Size>   u32  MSBIT();
template<Size>   i32  SIGN (u64 data);
template<Size>   u32  CLIP (u64 data);
template<Size>   u32  CLEAR(u64 data);
template<Size>   u32  SEXT (u64 data);
template<Size>   bool NBIT (u64 data);
template<Size>   bool CARRY(u64 data);
template<Size>   bool ZERO (u64 data);
template<Size S> u32  SUBS (u32 d1, u32 d2) { return CLEAR<S>(d1) | CLIP<S>(d2); }

}
#endif
