// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "FpuFormats.h"
#include <algorithm>
#include <bit>
#include <vector>

namespace moira {

namespace {

/* A minimal arbitrary-precision unsigned integer. It is only used for the
 * conversion from and to packed decimal numbers, which have to be exact. The
 * numbers involved are up to a few thousand bits wide, so the simple
 * algorithms used here are fast enough.
 */
struct Big {

    // Limbs in little-endian order without leading zeros
    std::vector<u32> v;

    Big() { }
    explicit Big(u64 x) { if (x) v.push_back((u32)x); if (x >> 32) v.push_back((u32)(x >> 32)); }

    bool isZero() const { return v.empty(); }

    int bitLength() const
    {
        return v.empty() ? 0 : int(v.size() - 1) * 32 + (32 - std::countl_zero(v.back()));
    }

    bool bit(int i) const
    {
        return i >= 0 && size_t(i / 32) < v.size() && ((v[i / 32] >> (i % 32)) & 1);
    }

    void trim() { while (!v.empty() && v.back() == 0) v.pop_back(); }

    void mulSmall(u32 m)
    {
        u64 carry = 0;
        for (auto &limb : v) { u64 t = u64(limb) * m + carry; limb = (u32)t; carry = t >> 32; }
        if (carry) v.push_back((u32)carry);
        trim();
    }

    void addSmall(u32 a)
    {
        u64 carry = a;
        for (size_t i = 0; carry && i < v.size(); i++) { u64 t = u64(v[i]) + carry; v[i] = (u32)t; carry = t >> 32; }
        if (carry) v.push_back((u32)carry);
    }

    void shl(int n)
    {
        if (isZero() || n == 0) return;
        int limbs = n / 32, bits = n % 32;
        if (bits) {
            u32 carry = 0;
            for (auto &limb : v) { u32 next = limb >> (32 - bits); limb = (limb << bits) | carry; carry = next; }
            if (carry) v.push_back(carry);
        }
        v.insert(v.begin(), limbs, 0);
    }

    // Shifts left by one bit and appends 'in' as the new least significant bit
    void shl1(bool in)
    {
        u32 carry = in;
        for (auto &limb : v) { u32 next = limb >> 31; limb = (limb << 1) | carry; carry = next; }
        if (carry) v.push_back(carry);
    }

    int cmp(const Big &o) const
    {
        if (v.size() != o.v.size()) return v.size() < o.v.size() ? -1 : 1;
        for (size_t i = v.size(); i-- > 0; ) if (v[i] != o.v[i]) return v[i] < o.v[i] ? -1 : 1;
        return 0;
    }

    // Subtracts o (which must not be greater than this)
    void sub(const Big &o)
    {
        i64 borrow = 0;
        for (size_t i = 0; i < v.size(); i++) {
            i64 t = i64(v[i]) - borrow - (i < o.v.size() ? i64(o.v[i]) : 0);
            borrow = t < 0; v[i] = (u32)(t + (borrow ? (1LL << 32) : 0));
        }
        trim();
    }

    Big mul(const Big &o) const
    {
        Big r;
        if (isZero() || o.isZero()) return r;
        r.v.assign(v.size() + o.v.size(), 0);
        for (size_t i = 0; i < v.size(); i++) {
            u64 carry = 0;
            for (size_t j = 0; j < o.v.size(); j++) {
                u64 t = u64(v[i]) * o.v[j] + r.v[i + j] + carry;
                r.v[i + j] = (u32)t; carry = t >> 32;
            }
            r.v[i + o.v.size()] += (u32)carry;
        }
        r.trim();
        return r;
    }

    // Checks whether any bit below position n is set
    bool anyBitBelow(int n) const
    {
        for (int i = 0; i < n / 32 && size_t(i) < v.size(); i++) if (v[i]) return true;
        if (size_t(n / 32) < v.size() && (n % 32) && (v[n / 32] & ((1u << (n % 32)) - 1))) return true;
        return false;
    }

    static Big pow10(int n)
    {
        Big result(1);
        for (; n >= 9; n -= 9) result.mulSmall(1000000000);
        for (; n > 0; n--) result.mulSmall(10);
        return result;
    }

    // Computes quotient and remainder of this / d
    Big divmod(const Big &d, Big &rem) const
    {
        Big q; rem = Big();
        q.v.assign(v.size(), 0);
        for (int i = bitLength() - 1; i >= 0; i--) {
            rem.shl1(bit(i));
            if (rem.cmp(d) >= 0) { rem.sub(d); q.v[i / 32] |= 1u << (i % 32); }
        }
        q.trim();
        return q;
    }

    // Returns the decimal digits (most significant first, at least one)
    std::vector<int> digits() const
    {
        std::vector<int> r;
        Big t = *this;
        while (!t.isZero()) {
            u64 rem = 0;
            for (size_t i = t.v.size(); i-- > 0; ) { u64 cur = (rem << 32) | t.v[i]; t.v[i] = (u32)(cur / 10); rem = cur % 10; }
            t.trim(); r.push_back((int)rem);
        }
        if (r.empty()) r.push_back(0);
        std::reverse(r.begin(), r.end());
        return r;
    }
};

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
    i64 hi = (1LL << (bits - 1)) - 1;
    i64 lo = -hi - 1;

    if (value.isNaN()) {

        FloatEnv::raiseFlags(FloatFlags::INVALID);
        return (i32)hi;
    }
    if (value.isInfinite()) {

        FloatEnv::raiseFlags(FloatFlags::INVALID);
        return (i32)(value.sign() ? lo : hi);
    }

    auto rounded = value.roundToInt();

    if (rounded.exponent() < 63) {

        auto i = rounded.toInt64Truncated();
        if (i >= lo && i <= hi) return (i32)i;
    }

    FloatEnv::raiseFlags(FloatFlags::INVALID);
    return (i32)(value.sign() ? lo : hi);
}


//
// Single and double precision
//

Float80
FpuFormats::fromSingle(u32 raw)
{
    return Float80::fromFloat(std::bit_cast<float>(raw));
}

Float80
FpuFormats::fromDouble(u64 raw)
{
    return Float80::fromDouble(std::bit_cast<double>(raw));
}

u32
FpuFormats::toSingle(const Float80 &value)
{
    return std::bit_cast<u32>(value.toFloat());
}

u64
FpuFormats::toDouble(const Float80 &value)
{
    return std::bit_cast<u64>(value.toDouble());
}


//
// Extended precision
//

Float80
FpuFormats::fromExtended(const u32 (&w)[3])
{
    return Float80::fromBits(u16(w[0] >> 16), u64(w[1]) << 32 | w[2]);
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
FpuFormats::fromPacked(const u32 (&w)[3])
{
    bool sm = w[0] >> 31;
    bool se = (w[0] >> 30) & 1;

    // Infinities and NaNs
    if (((w[0] >> 16) & 0xFFF) == 0xFFF) {

        u64 man = u64(w[1]) << 32 | w[2];
        return Float80::fromBits(sm ? 0xFFFF : 0x7FFF, man ? man : (1ULL << 63));
    }

    // Extract the exponent
    int exp = bcd(w[0], 12) * 1000 + bcd(w[0], 24) * 100 + bcd(w[0], 20) * 10 + bcd(w[0], 16);
    if (se) exp = -exp;

    // Extract the mantissa as a 17 digit integer
    u64 digits = (u64)bcd(w[0], 0);
    for (int i = 28; i >= 0; i -= 4) digits = digits * 10 + bcd(w[1], i);
    for (int i = 28; i >= 0; i -= 4) digits = digits * 10 + bcd(w[2], i);

    if (digits == 0) return Float80::zero(sm);

    // The value is digits * 10^e
    int e = exp - 16;
    u64 sig, extra;

    // Check for obvious overflows and underflows to keep the integers small
    if (e > 6000)  return Float80::roundPack(sm, 0x7FFF, 1ULL << 63);
    if (e < -6000) return Float80::roundPack(sm, 0, 1ULL << 63);

    if (e >= 0) {

        Big n(digits);
        for (; e >= 9; e -= 9) n.mulSmall(1000000000);
        for (; e > 0; e--) n.mulSmall(10);

        int len;
        top128(n, sig, extra, len);
        return Float80::roundPack(sm, 0x3FFF + len - 1, sig, extra);

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
        return Float80::roundPack(sm, 0x3FFF + len - 1 - shift, sig, extra);
    }
}

void
FpuFormats::toPacked(const Float80 &value, int k, u32 (&w)[3])
{
    w[0] = w[1] = w[2] = 0;
    u32 sign = value.sign() ? 0x80000000 : 0;

    // Infinities and NaNs
    if (!value.isFinite()) {

        w[0] = sign | 0x7FFF0000;
        if (value.isNaN()) { w[1] = u32(value.mantissa() >> 32); w[2] = u32(value.mantissa()); }
        return;
    }

    // Zero
    if (value.isZero()) { w[0] = sign; return; }

    // Check the k-factor
    if (k > 17) { FloatEnv::raiseFlags(FloatFlags::INVALID); k = 17; }
    if (k < -17) k = -17;

    // Represent the value as man * 2^p
    Big man(value.mantissa());
    int p = std::max(1, (int)value.biasedExponent()) - Float80::bias - 63;

    // Estimate the decimal exponent: floor(log10(value)) with a small error
    int top = p + man.bitLength() - 1;
    int e10 = (int)std::floor(top * 0.30102999566398120);

    // Determine the number of significant digits
    auto digitCount = [&](int e) { return k > 0 ? k : std::clamp(e + 1 - k, 1, 17); };

    // Compute round(value / 10^t) where t = e10 - (nd - 1), and fix the estimate if necessary
    Big s; bool inexact = false; int nd = 0;

    for (int attempt = 0; attempt < 4; attempt++) {

        nd = digitCount(e10);
        int t = e10 - (nd - 1);

        Big num = man, den(1);
        if (p >= 0) num.shl(p); else den.shl(-p);
        if (t >= 0) den = den.mul(Big::pow10(t)); else num = num.mul(Big::pow10(-t));

        Big rem;
        s = num.divmod(den, rem);
        inexact = !rem.isZero();

        // Round the quotient
        bool up = false;
        if (inexact) {
            switch (FloatEnv::roundingMode()) {

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

        // Check the result against the expected number of digits
        Big lowBound = Big::pow10(nd - 1), highBound = Big::pow10(nd);
        if (s.cmp(lowBound) < 0) { e10--; continue; }
        if (s.cmp(highBound) >= 0) {
            if (s.cmp(highBound) == 0 && up) { s = lowBound; e10++; break; }
            e10++; continue;
        }
        break;
    }

    if (inexact) FloatEnv::raiseFlags(FloatFlags::INEXACT);

    // Assemble the exponent
    int ae = std::abs(e10);
    u32 exp = u32(ae / 1000 % 10) << 12 | u32(ae / 100 % 10) << 24 | u32(ae / 10 % 10) << 20 | u32(ae % 10) << 16;
    w[0] = sign | (e10 < 0 ? 0x40000000 : 0) | exp;

    // Assemble the mantissa (17 digits, padded with zeros)
    auto ds = s.digits();
    ds.resize(17, 0);
    w[0] |= (u32)ds[0];
    for (int i = 0; i < 8; i++) w[1] |= u32(ds[1 + i]) << (28 - 4 * i);
    for (int i = 0; i < 8; i++) w[2] |= u32(ds[9 + i]) << (28 - 4 * i);
}

}
