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

// Extracts the 128 most significant bits of n (with a sticky bit in bit 0 of 'extra')
void
top128(const Big &n, u64 &sig, u64 &extra, int &bitLength)
{
    bitLength = n.bitLength();
    sig = extra = 0;
    for (int i = 0; i < 64; i++) sig = sig << 1 | n.bit(bitLength - 1 - i);
    for (int i = 0; i < 64; i++) extra = extra << 1 | n.bit(bitLength - 65 - i);
    if (n.anyBitBelow(bitLength - 128)) extra |= 1;
}

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
    bool se = (w[0] >> 30) & 1;

    // Infinities and NaNs
    if (((w[0] >> 16) & 0xFFF) == 0xFFF) {

        u64 man = u64(w[1]) << 32 | w[2];
        return Float80::fromBits(sm ? 0xFFFF : 0x7FFF, man).in(sf);
    }

    // Extract the exponent
    int exp = bcd(w[0], 12) * 1000 + bcd(w[0], 24) * 100 + bcd(w[0], 20) * 10 + bcd(w[0], 16);
    if (se) exp = -exp;

    // Extract the mantissa as a 17 digit integer
    u64 digits = (u64)bcd(w[0], 0);
    for (int i = 28; i >= 0; i -= 4) digits = digits * 10 + bcd(w[1], i);
    for (int i = 28; i >= 0; i -= 4) digits = digits * 10 + bcd(w[2], i);

    if (digits == 0) return Float80::zero(sm).in(sf);

    // The value is digits * 10^e
    int e = exp - 16;
    u64 sig, extra;

    // Check for obvious overflows and underflows to keep the integers small
    if (e > 6000)  return Float80::roundPack(sm, 0x7FFF, 1ULL << 63, 0, sf);
    if (e < -6000) return Float80::roundPack(sm, 0, 1ULL << 63, 0, sf);

    if (e >= 0) {

        Big n(digits);
        for (; e >= 9; e -= 9) n.mulSmall(1000000000);
        for (; e > 0; e--) n.mulSmall(10);

        int len;
        top128(n, sig, extra, len);
        return Float80::roundPack(sm, 0x3FFF + len - 1, sig, extra, sf);

    } else {

        // Compute the quotient with at least 128 significant bits
        Big d = Big::pow10(-e);
        int shift = std::max(0, 128 + d.bitLength() - Big(digits).bitLength());
        Big n(digits);
        n.shl(shift);

        Big rem;
        Big q = n.divmod(d, rem);

        int len;
        top128(q, sig, extra, len);
        if (!rem.isZero()) extra |= 1;
        return Float80::roundPack(sm, 0x3FFF + len - 1 - shift, sig, extra, sf);
    }
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

    // Check the k-factor
    if (k > 17) { sf.raiseFlags(FloatFlags::INVALID); k = 17; }

    // Represent the value as man * 2^p
    Big man(value.mantissa());
    int p = (int)value.biasedExponent() - Float80::bias - 63;

    // Estimate the decimal exponent: floor(log10(value)) with a small error
    int top = p + man.bitLength() - 1;
    int e10 = (int)std::floor(top * 0.30102999566398120);

    // The exponent of the smallest normalized number is the lower limit. Smaller values (denormals)
    // are represented with leading zeros in the mantissa.
    constexpr int minExp10 = -4933;
    e10 = std::max(e10, minExp10);

    /* Determine the number of significant digits (nd) and the exponent of the
     * least significant digit (t). With a positive k-factor, k digits are
     * produced. Otherwise, the result has -k digits to the right of the
     * decimal point, limited to 17 significant digits. If the value is smaller
     * than the least significant digit, nd is less than 1.
     */
    auto digitCount = [&](int e) { return k > 0 ? k : std::min(e + 1 - k, 17); };
    auto lsbExponent = [&](int e, int nd) { return k > 0 ? e - (nd - 1) : (e + 1 - k > 17 ? e - 16 : k); };

    // Compute round(value / 10^t) and fix the estimate of the decimal exponent if necessary
    Big s; bool inexact = false; int nd = 0;

    for (int attempt = 0; attempt < 4; attempt++) {

        nd = digitCount(e10);
        int t = lsbExponent(e10, nd);

        Big num = man, den(1);
        if (p >= 0) num.shl(p); else den.shl(-p);
        if (t >= 0) den = den.mul(Big::pow10(t)); else num = num.mul(Big::pow10(-t));

        Big rem;
        s = num.divmod(den, rem);
        inexact = !rem.isZero();

        // Round the quotient
        bool up = false;
        if (inexact) {
            switch (sf.roundingMode()) {

                case RoundingMode::NEAREST: {
                    Big twice = rem; twice.shl1(false);
                    int c = twice.cmp(den);
                    up = c > 0 || (c == 0 && s.bit(0));
                    break;
                }
                case RoundingMode::ZERO:    up = false; break;
                case RoundingMode::DOWN:    up = value.sign(); break;
                case RoundingMode::UP:      up = !value.sign(); break;
            }
        }
        if (up) s.addSmall(1);

        // The value is smaller than the least significant digit
        if (nd < 1) {
            if (s.isZero()) {

                // The result is zero, but keeps the exponent of the least significant digit
                if (inexact) sf.raiseFlags(FloatFlags::INEXACT);
                int ae = std::abs(t);
                w[0] = sign | (t < 0 ? 0x40000000 : 0)
                     | u32(ae / 1000 % 10) << 12 | u32(ae / 100 % 10) << 24 | u32(ae / 10 % 10) << 20 | u32(ae % 10) << 16;
                return;
            }
            e10 = t;
            break;
        }

        // Check the result against the expected number of digits
        Big lowBound = Big::pow10(nd - 1), highBound = Big::pow10(nd);
        if (s.cmp(lowBound) < 0) { if (e10 == minExp10) break; e10--; continue; }
        if (s.cmp(highBound) >= 0) {
            if (s.cmp(highBound) == 0 && up) { s = lowBound; e10++; break; }
            e10++; continue;
        }
        break;
    }

    if (inexact) sf.raiseFlags(FloatFlags::INEXACT);

    // Assemble the exponent
    int ae = std::abs(e10);

    // The exponent doesn't fit into three digits
    if (ae >= 1000) sf.raiseFlags(FloatFlags::INVALID);
    u32 exp = u32(ae / 1000 % 10) << 12 | u32(ae / 100 % 10) << 24 | u32(ae / 10 % 10) << 20 | u32(ae % 10) << 16;
    w[0] = sign | (e10 < 0 ? 0x40000000 : 0) | exp;

    // Assemble the mantissa (17 digits, padded with zeros)
    auto ds = s.digits();
    if (int(ds.size()) < nd) ds.insert(ds.begin(), nd - ds.size(), 0);
    ds.resize(17, 0);
    w[0] |= (u32)ds[0];
    for (int i = 0; i < 8; i++) w[1] |= u32(ds[1 + i]) << (28 - 4 * i);
    for (int i = 0; i < 8; i++) w[2] |= u32(ds[9 + i]) << (28 - 4 * i);
}

}
