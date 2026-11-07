// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "FpuFormats.h"
#include "BigNum.h"
#include <algorithm>
#include <bit>
#include <vector>

namespace MOIRA_NAMESPACE {

namespace {

int bcd(u32 w, int shift) { return int((w >> shift) & 0xF); }

}


//
// Integers
//

i32
FpuFormats::toInteger(const Float80 &value, int bits)
{
    auto &sf = value.env();
    i64 hi = (1LL << (bits - 1)) - 1;
    i64 lo = -hi - 1;

    if (value.isNaN()) {

        // The result consists of the most significant bits of the (quieted) mantissa
        u64 man = value.mantissa();
        if (value.isSignalingNaN()) { sf.raiseFlags(FloatFlags::SNAN); man |= 1ULL << 62; }
        sf.raiseFlags(FloatFlags::INVALID);
        return (i32)(u32)(man >> (64 - bits));
    }
    if (value.isInfinite()) {

        sf.raiseFlags(FloatFlags::INVALID);
        return (i32)(value.sign() ? lo : hi);
    }

    auto saved = sf.flags();
    auto rounded = value.roundToInt();

    if (rounded.exponent() < 63) {

        auto i = rounded.toInt64Truncated();
        if (i >= lo && i <= hi) return (i32)i;
    }

    // An overflow is reported as an operand error without the inexact exception
    sf.clearFlags();
    sf.raiseFlags(saved);
    sf.raiseFlags(FloatFlags::INVALID);
    return (i32)(value.sign() ? lo : hi);
}


//
// Single and double precision
//

Float80
FpuFormats::fromSingle(u32 raw, SoftFloat &sf)
{
    // NaNs are converted bit by bit to keep their payload and their signaling state
    if ((raw & 0x7F800000) == 0x7F800000 && (raw & 0x007FFFFF)) {
        return Float80::fromBits(u16((raw >> 16) & 0x8000) | 0x7FFF,
                                 u64(raw & 0x007FFFFF) << 40).in(sf);
    }

    // Denormals keep their unnormalized significand and the smallest normal exponent
    if ((raw & 0x7F800000) == 0 && (raw & 0x007FFFFF)) {
        return Float80::fromBits(u16((raw >> 16) & 0x8000) | 0x3F81, u64(raw & 0x007FFFFF) << 40).in(sf);
    }
    return Float80::fromFloat(std::bit_cast<float>(raw), sf);
}

Float80
FpuFormats::fromDouble(u64 raw, SoftFloat &sf)
{
    if ((raw & 0x7FF0000000000000ULL) == 0x7FF0000000000000ULL && (raw & 0x000FFFFFFFFFFFFFULL)) {
        return Float80::fromBits(u16((raw >> 48) & 0x8000) | 0x7FFF,
                                 (raw & 0x000FFFFFFFFFFFFFULL) << 11).in(sf);
    }

    if ((raw & 0x7FF0000000000000ULL) == 0 && (raw & 0x000FFFFFFFFFFFFFULL)) {
        return Float80::fromBits(u16((raw >> 48) & 0x8000) | 0x3C01, (raw & 0x000FFFFFFFFFFFFFULL) << 11).in(sf);
    }
    return Float80::fromDouble(std::bit_cast<double>(raw), sf);
}

u32
FpuFormats::toSingle(const Float80 &value)
{
    auto &sf = value.env();
    if (value.isNaN()) {

        // Truncate the payload, but make sure the result remains a NaN
        u32 frac = u32((value.mantissa() >> 40) & 0x007FFFFF);
        if (value.isSignalingNaN()) { sf.raiseFlags(FloatFlags::SNAN); frac |= 0x00400000; }
        if (frac == 0) frac = 0x00400000;
        return u32(value.sign()) << 31 | 0x7F800000 | frac;
    }
    return std::bit_cast<u32>(value.toFloat());
}

u64
FpuFormats::toDouble(const Float80 &value)
{
    auto &sf = value.env();
    if (value.isNaN()) {

        u64 frac = (value.mantissa() >> 11) & 0x000FFFFFFFFFFFFFULL;
        if (value.isSignalingNaN()) { sf.raiseFlags(FloatFlags::SNAN); frac |= 0x0008000000000000ULL; }
        if (frac == 0) frac = 0x0008000000000000ULL;
        return u64(value.sign()) << 63 | 0x7FF0000000000000ULL | frac;
    }
    return std::bit_cast<u64>(value.toDouble());
}


//
// Extended precision
//

Float80
FpuFormats::fromExtended(const u32 (&w)[3], SoftFloat &sf)
{
    return Float80::fromBits(u16(w[0] >> 16), u64(w[1]) << 32 | w[2]).in(sf);
}

void
FpuFormats::toExtended(const Float80 &value, u32 (&w)[3])
{
    w[0] = u32(value.signExp()) << 16;
    w[1] = u32(value.mantissa() >> 32);
    w[2] = u32(value.mantissa());
}


namespace {

using u128 = unsigned __int128;

// A 128-bit extended-precision intermediate value (the significand includes
// the integer bit and is stored in the upper 64 bits for a plain extended value)
struct Dec { i32 exp; u128 sig; };

// A 192-bit value
struct U192 { u64 h; u128 l; };

u128 shiftJam(u128 a, int count)
{
    if (count <= 0) return a;
    if (count >= 128) return a != 0;
    return (a >> count) | ((a << (128 - count)) != 0);
}

void mul256(u128 a, u128 b, u128 &hi, u128 &lo)
{
    u64 a0 = u64(a >> 64), a1 = u64(a), b0 = u64(b >> 64), b1 = u64(b);
    u128 p11 = u128(a1) * b1, p10 = u128(a1) * b0, p01 = u128(a0) * b1, p00 = u128(a0) * b0;
    u128 mid = (p11 >> 64) + u64(p10) + u64(p01);
    lo = u128(u64(p11)) | (mid << 64);
    hi = p00 + (p10 >> 64) + (p01 >> 64) + (mid >> 64);
}

U192 mul128By64(u128 b, u64 z)
{
    u128 lo = u128(u64(b)) * z, hi = u128(u64(b >> 64)) * z;
    u128 mid = hi + (lo >> 64);
    return { u64(mid >> 64), (u128(u64(mid)) << 64) | u64(lo) };
}

U192 sub(U192 a, U192 b) { return { a.h - b.h - (a.l < b.l), a.l - b.l }; }
U192 add(U192 a, U192 b) { u128 l = a.l + b.l; return { a.h + b.h + (l < a.l), l }; }

u64 estimateDiv128To64(u64 a0, u64 a1, u64 b)
{
    if (b <= a0) return ~u64(0);
    u64 b0 = b >> 32;
    u64 z = (b0 << 32 <= a0) ? 0xFFFFFFFF00000000ULL : (a0 / b0) << 32;
    u128 rem = (u128(a0) << 64 | a1) - u128(b) * z;
    while (i64(rem >> 64) < 0) {
        z -= 1ULL << 32;
        rem += u128(b) << 32;
    }
    u64 r0 = u64(rem >> 32);
    z |= (b0 << 32 <= r0) ? 0xFFFFFFFFULL : r0 / b0;
    return z;
}

// Rounds a 128-bit significand to 64 bits
void roundTo64(bool sign, Dec &a, RoundingMode mode)
{
    u64 s0 = u64(a.sig >> 64), s1 = u64(a.sig);
    bool nearest = mode == RoundingMode::NEAREST;
    bool inc = i64(s1) < 0;

    if (!nearest) {
        if (mode == RoundingMode::ZERO) inc = false;
        else if (sign) inc = mode == RoundingMode::DOWN && s1;
        else inc = mode == RoundingMode::UP && s1;
    }
    if (inc) {
        if (++s0 == 0) { a.exp++; s0 = 1ULL << 63; }
        else if ((s1 << 1) == 0 && nearest) s0 &= ~1ULL;
    } else if (s0 == 0) {
        a.exp = 0;
    }
    a.sig = u128(s0) << 64;
}

Dec mul(Dec a, Dec b)
{
    u128 hi, lo;
    mul256(a.sig, b.sig, hi, lo);
    Dec z = { a.exp + b.exp - 0x3FFE, hi | (lo != 0) };
    if (0 < i64(z.sig >> 64)) { z.sig <<= 1; z.exp--; }
    return z;
}

Dec mulRounded(Dec a, Dec b, RoundingMode mode)
{
    roundTo64(false, b, mode);
    Dec z = mul(a, b);
    roundTo64(false, z, mode);
    return z;
}

Dec div(Dec a, Dec b)
{
    u64 b0 = u64(b.sig >> 64);
    i32 zExp = a.exp - b.exp + 0x3FFE;
    if (b.sig <= a.sig) { a.sig >>= 1; zExp++; }

    u64 z0 = estimateDiv128To64(u64(a.sig >> 64), u64(a.sig), b0);
    U192 rem = sub({ u64(a.sig >> 64), u128(u64(a.sig)) << 64 }, mul128By64(b.sig, z0));
    while (i64(rem.h) < 0) { z0--; rem = add(rem, { 0, b.sig }); }

    u64 z1 = estimateDiv128To64(u64(rem.l >> 64), u64(rem.l), b0);
    if ((z1 & 0x3FFF) <= 4) {
        U192 term = mul128By64(b.sig, z1);
        rem = sub({ u64(rem.l >> 64), u128(u64(rem.l)) << 64 }, term);
        while (i64(rem.h) < 0) { z1--; rem = add(rem, { 0, b.sig }); }
        z1 |= (rem.h | (rem.l != 0)) != 0;
    }
    return { zExp, u128(z0) << 64 | z1 };
}

// Computes 10^scale with 128-bit precision
// The rounding direction is chosen such that the sign of the final result is respected
Dec tenToInt(bool mSign, bool eSign, int scale, RoundingMode mode)
{
    switch (mode) {
        case RoundingMode::DOWN: if (mSign != eSign) mode = RoundingMode::UP; break;
        case RoundingMode::UP:   if (mSign != eSign) mode = RoundingMode::DOWN; break;
        case RoundingMode::ZERO: mode = eSign ? RoundingMode::UP : RoundingMode::DOWN; break;
        default: break;
    }
    Dec a = { 0x3FFF, u128(1) << 127 };
    Dec m = { 0x4002, u128(0xA000000000000000ULL) << 64 };

    for (; scale; scale >>= 1) {
        if (scale & 1) a = mulRounded(a, m, mode);
        m = mul(m, m);
    }
    return a;
}

u64 tenToIntDec(int scale)
{
    u64 x = 1, m = 10;
    for (; scale; scale >>= 1) { if (scale & 1) x *= m; m *= m; }
    return x;
}

// Converts the scaled value to an integer
u64 toInt64(bool sign, Dec z, RoundingMode mode, bool &inexact)
{
    u128 s = shiftJam(z.sig, 0x403E - z.exp);
    u64 s0 = u64(s >> 64), s1 = u64(s);
    bool nearest = mode == RoundingMode::NEAREST;
    bool inc = i64(s1) < 0;

    if (!nearest) {
        if (mode == RoundingMode::ZERO) inc = false;
        else if (sign) inc = mode == RoundingMode::DOWN && s1;
        else inc = mode == RoundingMode::UP && s1;
    }
    if (inc) {
        ++s0;
        if ((s1 << 1) == 0 && nearest) s0 &= ~1ULL;
    }
    inexact = s1 != 0;
    return s0;
}

// Estimates floor(log10(value)) from the exponent and the significand
i32 getDecimalExponent(i32 aExp, u64 aSig)
{
    if (aSig == 0 || aExp == 0x3FFF) return 0;
    if (aExp < 0) return -4932;

    aSig ^= 1ULL << 63;
    aExp -= 0x3FFF;
    bool zSign = aExp < 0;
    aExp = zSign ? -aExp : aExp;
    int shiftCount = 31 - std::countl_zero(u32(aExp));
    i32 zExp = 0x3FFF + shiftCount;
    u128 z;

    if (shiftCount < 0) {
        z = (u128(aSig) << 64) << -shiftCount;
    } else {
        z = (u128(aSig) << 64) >> shiftCount;
        u128 e = u128(u64(aExp) << (63 - shiftCount)) << 64;
        z = zSign ? e - z : e + z;
    }

    int lz = std::min(63, z >> 64 ? std::countl_zero(u64(z >> 64)) : 64);
    z <<= lz;
    zExp -= lz;

    // Multiply by log10(2)
    Dec d = mul({ zExp, z }, { 0x3FFD, (u128(0x9A209A84FBCFF798ULL) << 64) | 0x8F8959AC0B7C9178ULL });
    u128 s = shiftJam(d.sig, 0x403E - d.exp);
    u64 s0 = u64(s >> 64), s1 = u64(s);
    if (i64(s1) < 0) { ++s0; s0 &= ~(u64((s1 << 1) == 0)); }

    return (i32)(zSign ? (0 - s0) : s0);
}

}

//
// Packed decimal
//

Float80
FpuFormats::fromPacked(const u32 (&w)[3], SoftFloat &sf)
{
    // The conversion is carried out in extended precision. A reduced precision
    // is applied by the instruction that uses the value.
    SoftFloat::Scope scope(sf);
    sf.setPrecision(Precision::EXTENDED);

    bool sm = w[0] >> 31;

    // Infinities and NaNs
    if (((w[0] >> 16) & 0x7FFF) == 0x7FFF) {

        u64 man = u64(w[1]) << 32 | w[2];
        return Float80::fromBits(sm ? 0xFFFF : 0x7FFF, man).in(sf);
    }

    // Zero (the exponent is irrelevant if the mantissa is zero)
    if (!(w[0] & 0xF) && !w[1] && !w[2]) return Float80::zero(sm).in(sf);

    /* The conversion follows the algorithm of the reference implementation
     * (UAE's floatdecimal_to_floatx80). The mantissa is scaled by a power of
     * ten that is approximated with 128-bit precision.
     */

    // Extract the exponent (three digits)
    i32 exp = bcd(w[0], 24) * 100 + bcd(w[0], 20) * 10 + bcd(w[0], 16);
    bool se = (w[0] >> 30) & 1;
    if (se) exp = -exp;
    exp -= 16;
    if (exp < 0) { exp = -exp; se = true; }

    // Extract the mantissa
    u64 mant = w[0] & 0xF;
    for (int i = 28; i >= 0; i -= 4) mant = mant * 10 + ((w[1] >> i) & 0xF);
    for (int i = 28; i >= 0; i -= 4) mant = mant * 10 + ((w[2] >> i) & 0xF);

    // Scale the mantissa
    int shift = std::countl_zero(mant);
    Dec z = { 0x403E - shift, u128(mant << shift) << 64 };
    Dec x = tenToInt(sm, se, exp & 0x3FFF, sf.roundingMode());
    z = se ? div(z, x) : mul(z, x);

    if (u64(z.sig)) sf.raiseFlags(FloatFlags::INEXACT);
    roundTo64(sm, z, sf.roundingMode());

    return Float80::fromBits(u16(sm << 15 | z.exp), u64(z.sig >> 64)).in(sf);
}

void
FpuFormats::toPacked(const Float80 &value, int k, u32 (&w)[3])
{
    auto &sf = value.env();
    w[0] = w[1] = w[2] = 0;
    u32 sign = value.sign() ? 0x80000000 : 0;

    // Infinities and NaNs
    if (!value.isFinite()) {

        w[0] = sign | 0x7FFF0000;
        if (value.isNaN()) {

            // Signaling NaNs are converted to quiet NaNs
            u64 man = value.mantissa();
            if (value.isSignalingNaN()) { sf.raiseFlags(FloatFlags::SNAN); man |= 1ULL << 62; }
            w[1] = u32(man >> 32);
            w[2] = u32(man);
        }
        return;
    }

    // Zero
    if (value.isZero()) { w[0] = sign; return; }

    /* The conversion follows the algorithm of the reference implementation
     * (UAE's floatx80_to_floatdecimal). It scales the value by a power of ten
     * that is approximated with 128-bit precision. Hence, the digits may differ
     * from the correctly rounded result in rare cases, which is the behavior
     * of the real hardware.
     */
    u64 aSig = value.mantissa();
    i32 aExp = (i32)value.biasedExponent();
    bool aSign = value.sign();

    // Normalize denormals. As in the reference, the exponent of a denormal is regarded as 0 (not 1)
    if (aExp == 0) {
        int shift = std::countl_zero(aSig);
        aExp = -shift;
        aSig <<= shift;
    }

    i32 kfactor = k;
    i32 ilog = getDecimalExponent(aExp, aSig);
    RoundingMode mode = sf.roundingMode();
    bool retried = false;
    i32 len;
    u64 decSig;

    for (;;) {

        if (kfactor > 0) {
            if (kfactor > 17) { kfactor = 17; sf.raiseFlags(FloatFlags::INVALID); }
            len = kfactor;
        } else {
            len = std::clamp(ilog + 1 - kfactor, 1, 17);
            if (kfactor > ilog) ilog = kfactor;
        }

        bool lambda = false;
        i32 iscale = ilog + 1 - len;
        if (iscale < 0) { lambda = true; iscale = -iscale; }

        Dec x = tenToInt(lambda, false, iscale, mode);
        Dec z = { aExp, u128(aSig) << 64 };
        z = lambda ? mul(z, x) : div(z, x);

        bool inexact;
        decSig = toInt64(aSign, z, mode, inexact);
        if (inexact) sf.raiseFlags(FloatFlags::INEXACT);

        if (!retried) {

            u64 decX = tenToIntDec(len - 1);
            if (decSig < decX) { ilog -= 1; retried = true; continue; }
            if (decSig > decX * 10) { ilog += 1; retried = true; continue; }
        }
        break;
    }

    // Assemble the exponent
    u32 exponent = u32(ilog < 0 ? -ilog : ilog);
    if (exponent > 999) sf.raiseFlags(FloatFlags::INVALID);

    u32 packExp = 0, packExp4 = 0;
    for (int i = 3; i >= 0; i--) {
        u32 digit = exponent % 10;
        exponent /= 10;
        if (i == 0) packExp4 = digit; else packExp |= digit << (12 - i * 4);
    }

    // Assemble the mantissa
    u32 packInt = 0;
    u64 packFrac = 0;
    for (int i = len - 1; i >= 0; i--) {
        u64 digit = decSig % 10;
        decSig /= 10;
        if (i == 0) packInt = (u32)digit; else packFrac |= digit << (64 - i * 4);
    }

    w[0] = sign | (ilog < 0 ? 0x40000000 : 0) | packExp << 16 | packExp4 << 12 | packInt;
    w[1] = u32(packFrac >> 32);
    w[2] = u32(packFrac);
}

}
