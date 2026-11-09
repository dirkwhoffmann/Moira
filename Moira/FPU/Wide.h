// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "../MoiraTypes.h"
#include <bit>

namespace MOIRA_NAMESPACE {

/* Helpers for 128-bit integer arithmetic. They are shared by the transcendental
 * functions (FpuEnvMath.cpp) and the packed decimal conversion (FpuFormats).
 */
using u128 = unsigned __int128;

inline u128 makeU128(u64 hi, u64 lo) { return u128(hi) << 64 | lo; }
inline u64 hi64(u128 x) { return u64(x >> 64); }
inline u64 lo64(u128 x) { return u64(x); }

// Counts the leading zero bits (the result is 128 for x = 0)
inline int clz128(u128 x)
{
    return hi64(x) ? std::countl_zero(hi64(x)) : 64 + std::countl_zero(lo64(x));
}

// Multiplies two 128-bit values. The 256-bit product is returned as hi:lo.
inline void mul128To256(u128 a, u128 b, u128 &hi, u128 &lo)
{
    u128 p00 = u128(lo64(a)) * lo64(b), p01 = u128(lo64(a)) * hi64(b);
    u128 p10 = u128(hi64(a)) * lo64(b), p11 = u128(hi64(a)) * hi64(b);

    u128 mid = (p00 >> 64) + lo64(p01) + lo64(p10);
    lo = makeU128(lo64(mid), lo64(p00));
    hi = p11 + (p01 >> 64) + (p10 >> 64) + (mid >> 64);
}

// Returns the upper 128 bits of the 256-bit product
inline u128 mulHigh128(u128 a, u128 b)
{
    u128 hi, lo;
    mul128To256(a, b, hi, lo);
    return hi;
}

// Shifts right and sets the least significant bit if any one-bit was shifted out
inline u128 shiftRightJam128(u128 a, int count)
{
    if (count <= 0) return a;
    if (count >= 128) return a != 0;
    return (a >> count) | ((a << (128 - count)) != 0);
}

}
