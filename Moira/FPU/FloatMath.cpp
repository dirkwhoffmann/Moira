// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "FloatMath.h"
#include "BigNum.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace MOIRA_NAMESPACE {

namespace {

using u128 = unsigned __int128;


//
// Wide floating-point numbers
//

/* A floating-point number with a 128-bit significand and an exponent that is
 * not limited in practice. The value is (-1)^neg * m * 2^(e - 127). The
 * significand is normalized (bit 127 is set) unless the number is zero.
 *
 * The arithmetic truncates and is accurate to about 2^-126. All transcendental
 * functions are evaluated with this type and rounded to Float80 at the end.
 */
struct W {

    bool neg = false;
    i32 e = 0;
    u128 m = 0;

    bool zero() const { return m == 0; }
};

int clz128(u128 x)
{
    u64 hi = u64(x >> 64);
    return hi ? std::countl_zero(hi) : 64 + std::countl_zero(u64(x));
}

// Creates a number of value m * 2^(e - 127)
W norm(bool neg, i32 e, u128 m)
{
    W w;
    w.neg = neg;
    if (m) {
        int s = clz128(m);
        w.m = m << s;
        w.e = e - s;
    }
    return w;
}

W fromU64(u64 v) { return norm(false, 127, v); }
W fromI64(i64 v) { return norm(v < 0, 127, v < 0 ? u64(-v) : u64(v)); }
W fromFloat80(const Float80 &x) { return norm(x.sign(), i32(x.biasedExponent()) - Float80::bias, u128(x.mantissa()) << 64); }

W fromDouble(double d)
{
    int k;
    double f = std::frexp(std::fabs(d), &k);
    return norm(d < 0, k + 63, u64(std::ldexp(f, 64)));
}

double toDouble(const W &w)
{
    return (w.neg ? -1.0 : 1.0) * std::ldexp(double(u64(w.m >> 64)), w.e - 63);
}

W operator-(W a) { a.neg = !a.neg; return a; }

// Returns the high 128 bits of a 256-bit product
u128 mulhi(u128 a, u128 b)
{
    u64 a0 = u64(a), a1 = u64(a >> 64), b0 = u64(b), b1 = u64(b >> 64);
    u128 p00 = u128(a0) * b0, p01 = u128(a0) * b1, p10 = u128(a1) * b0, p11 = u128(a1) * b1;
    u128 mid = (p00 >> 64) + u64(p01) + u64(p10);
    return p11 + (p01 >> 64) + (p10 >> 64) + (mid >> 64);
}

W operator*(const W &a, const W &b)
{
    if (a.zero() || b.zero()) { W z; z.neg = a.neg != b.neg; return z; }
    return norm(a.neg != b.neg, a.e + b.e + 1, mulhi(a.m, b.m));
}

W operator+(W a, W b)
{
    if (a.zero()) return b;
    if (b.zero()) return a;
    if (a.e < b.e || (a.e == b.e && a.m < b.m)) std::swap(a, b);

    i32 d = a.e - b.e;
    u128 am = a.m >> 1;
    u128 bm = d >= 127 ? 0 : (b.m >> 1) >> d;
    u128 r = a.neg == b.neg ? am + bm : am - bm;
    return norm(a.neg, a.e + 1, r);
}

W operator-(const W &a, const W &b) { return a + (-b); }

W operator/(const W &a, const W &b)
{
    if (a.zero()) return a;

    bool ge = a.m >= b.m;
    u128 r = ge ? a.m - b.m : a.m;
    u128 q = ge ? 1 : 0;

    for (int i = 0; i < (ge ? 127 : 128); i++) {

        bool carry = r >> 127;
        r <<= 1;
        q <<= 1;
        if (carry || r >= b.m) { r -= b.m; q |= 1; }
    }
    W w;
    w.neg = a.neg != b.neg;
    w.m = q;
    w.e = a.e - b.e - (ge ? 0 : 1);
    return w;
}

// Compares the magnitudes of two numbers
int cmpAbs(const W &a, const W &b)
{
    if (a.zero() || b.zero()) return a.zero() == b.zero() ? 0 : (a.zero() ? -1 : 1);
    if (a.e != b.e) return a.e < b.e ? -1 : 1;
    return a.m == b.m ? 0 : (a.m < b.m ? -1 : 1);
}

// Multiplies by 2^n
W scale(W a, i32 n) { if (!a.zero()) a.e += n; return a; }

W abs(W a) { a.neg = false; return a; }

W sqrt(const W &a)
{
    if (a.zero()) return a;

    // Write a = f * 4^k with f in [1,4)
    i32 k = a.e >> 1;
    W f = a;
    f.e = a.e & 1;

    // Newton iteration
    W x = fromDouble(std::sqrt(toDouble(f)));
    W half = scale(fromU64(1), -1);
    for (int i = 0; i < 4; i++) x = (x + f / x) * half;

    return scale(x, k);
}

const W &one() { static const W w = fromU64(1); return w; }
const W &two() { static const W w = fromU64(2); return w; }


//
// Series
//

// Computes atanh(z) = z + z^3/3 + z^5/5 + ... for small z
W atanhSeries(const W &z)
{
    if (z.zero()) return z;

    W z2 = z * z, term = z, sum = z;
    for (u64 k = 1; k < 200; k++) {

        term = term * z2;
        if (term.zero() || term.e < sum.e - 135) break;
        sum = sum + term / fromU64(2 * k + 1);
    }
    return sum;
}

// Computes atan(z) = z - z^3/3 + z^5/5 - ... for small z
W atanSeries(const W &z)
{
    if (z.zero()) return z;

    W z2 = z * z, term = z, sum = z;
    for (u64 k = 1; k < 200; k++) {

        term = term * z2;
        if (term.zero() || term.e < sum.e - 135) break;
        W t = term / fromU64(2 * k + 1);
        sum = (k & 1) ? sum - t : sum + t;
    }
    return sum;
}

// Computes sin(r) and cos(r) for |r| <= pi/4
void sinCosSeries(const W &r, W &s, W &c)
{
    W r2 = r * r;

    // Sine
    W term = r, sum = r;
    for (u64 k = 1; k < 100 && !r.zero(); k++) {

        term = -(term * r2) / fromU64((2 * k) * (2 * k + 1));
        if (term.zero() || term.e < sum.e - 135) break;
        sum = sum + term;
    }
    s = sum;

    // Cosine
    term = one(); sum = one();
    for (u64 k = 1; k < 100; k++) {

        term = -(term * r2) / fromU64((2 * k - 1) * (2 * k));
        if (term.zero() || term.e < -135) break;
        sum = sum + term;
    }
    c = sum;
}


//
// Constants
//

const W &ln2()
{
    static const W w = scale(atanhSeries(one() / fromU64(3)), 1);
    return w;
}

const W &ln10()
{
    // ln(10) = 3 ln(2) + ln(5/4)
    static const W w = fromU64(3) * ln2() + scale(atanhSeries(one() / fromU64(9)), 1);
    return w;
}

const W &invLn2() { static const W w = one() / ln2(); return w; }

// Computes floor(pi * 2^bits) with a small error
Big bigPi(int bits)
{
    int fp = bits + 64;

    auto atanInv = [&](u32 k) {

        Big term(1);
        term.shl(fp);
        term.divSmall(k);
        Big sum = term;

        for (u32 i = 1; !term.isZero(); i++) {

            term.divSmall(k * k);
            Big t = term;
            t.divSmall(2 * i + 1);
            if (i & 1) sum.sub(t); else sum.add(t);
        }
        return sum;
    };

    Big a = atanInv(5), b = atanInv(239);
    a.mulSmall(16);
    b.mulSmall(4);
    a.sub(b);

    // Drop the guard bits
    Big result;
    int limbs = 64 / 32;
    if (a.v.size() > size_t(limbs)) result.v.assign(a.v.begin() + limbs, a.v.end());
    return result;
}

const W &pi()
{
    static const W w = [] {
        Big p = bigPi(192);
        u128 m = 0;
        int len = p.bitLength();
        for (int i = 0; i < 128; i++) m = m << 1 | p.bit(len - 1 - i);
        return norm(false, len - 193, m);
    }();
    return w;
}

const W &piOver2() { static const W w = scale(pi(), -1); return w; }

// Returns 2/pi in fixed point with at least 'bits' fractional bits
const Big &twoOverPi(int bits, int &fractionalBits)
{
    struct Table { int bits = 0; Big t; };
    static thread_local Table table;

    if (table.bits < bits) {

        int n = (bits + 1023) / 1024 * 1024;
        Big p = bigPi(n);

        Big num(1);
        num.shl(2 * n + 1);
        Big rem;
        table.t = num.divmod(p, rem);
        table.bits = n;
    }
    fractionalBits = table.bits;
    return table.t;
}


//
// Argument reduction
//

/* Computes the remainder r of |x| modulo pi/2 with |r| <= pi/4 and the
 * quadrant (the quotient modulo 4). The reduction is exact, i.e., it takes as
 * many bits of pi into account as necessary for the magnitude of x.
 */
void reduce(const W &x, int &quadrant, W &r)
{
    quadrant = 0;

    if (x.e < -1) { r = abs(x); return; }

    int fp;
    const Big &t = twoOverPi(std::max(0, x.e) + 400, fp);

    // P = m * T represents |x| * 2/pi * 2^(x.e - 127 + fp)
    Big m;
    m.v = { u32(x.m), u32(x.m >> 32), u32(x.m >> 64), u32(x.m >> 96) };
    m.trim();
    Big p = m.mul(t);

    int s = fp + 127 - x.e;
    int n = p.bit(s) | p.bit(s + 1) << 1;

    // The fractional part
    Big f;
    size_t whole = size_t(s / 32);
    f.v.assign(p.v.begin(), p.v.begin() + std::min(whole, p.v.size()));
    if (s % 32 && whole < p.v.size()) f.v.push_back(p.v[whole] & ((1u << (s % 32)) - 1));
    f.trim();

    bool negative = false;
    if (f.bit(s - 1)) {

        Big full(1);
        full.shl(s);
        full.sub(f);
        f = full;
        negative = true;
        n = (n + 1) & 3;
    }

    // Convert the fraction into a wide number and multiply by pi/2
    int len = f.bitLength();
    u128 mant = 0;
    for (int i = 0; i < 128; i++) mant = mant << 1 | f.bit(len - 1 - i);

    W frac = norm(negative, len - s - 1, mant);
    r = frac * piOver2();
    quadrant = n;
}


//
// Elementary functions
//

// Computes e^x. The argument is clamped to a range that overflows or underflows anyway.
W expW(W x)
{
    if (x.zero()) return one();
    if (x.e > 20) x = norm(x.neg, 20, u128(1) << 127);

    W t = x * invLn2();
    i64 k = std::llround(toDouble(t));
    W r = x - fromI64(k) * ln2();
    r = scale(r, -6);

    W sum = one(), term = one();
    for (u64 n = 1; n < 100; n++) {

        term = term * r / fromU64(n);
        if (term.zero() || term.e < -140) break;
        sum = sum + term;
    }
    for (int i = 0; i < 6; i++) sum = sum * sum;

    return scale(sum, i32(k));
}

// Computes e^x - 1
W expm1W(W x)
{
    if (x.zero()) return x;

    if (x.e < -1) {

        // Avoid cancellation
        W term = x, sum = x;
        for (u64 n = 2; n < 100; n++) {

            term = term * x / fromU64(n);
            if (term.zero() || term.e < sum.e - 135) break;
            sum = sum + term;
        }
        return sum;
    }
    return expW(x) - one();
}

// Computes ln(x) for x > 0
W lnW(const W &x)
{
    // Write x = f * 2^k with f in (0.7,1.4]
    i32 k = x.e;
    W f = x;
    f.e = 0;
    static const W sqrt2 = sqrt(two());
    if (cmpAbs(f, sqrt2) > 0) { f.e = -1; k++; }

    W z = (f - one()) / (f + one());
    W result = scale(atanhSeries(z), 1);
    if (k) result = result + fromI64(k) * ln2();
    return result;
}

// Computes ln(1 + x) for x > -1
W lognp1W(const W &x)
{
    if (x.zero()) return x;
    if (x.e < -1) return scale(atanhSeries(x / (two() + x)), 1);
    return lnW(one() + x);
}

// Computes atan(x)
W atanW(W x)
{
    if (x.zero()) return x;

    bool negative = x.neg;
    x.neg = false;

    bool invert = cmpAbs(x, one()) > 0;
    if (invert) x = one() / x;

    for (int i = 0; i < 3; i++) x = x / (one() + sqrt(one() + x * x));

    W r = scale(atanSeries(x), 3);
    if (invert) r = piOver2() - r;

    r.neg = negative;
    return r;
}

/* Rounds a wide number to a Float80. Like the reference, we report every
 * result of a transcendental function as inexact, even if it is exact (e.g.,
 * 2^n or ln(1)). Special operands that are resolved up front don't get here.
 */
Float80 finish(SoftFloat &sf, const W &w)
{
    sf.raiseFlags(FloatFlags::INEXACT);
    if (w.zero()) return Float80::zero(w.neg);
    return Float80::roundPack(w.neg, w.e + Float80::bias, u64(w.m >> 64), u64(w.m), sf);
}

Float80 invalid(SoftFloat &sf)
{
    sf.raiseFlags(FloatFlags::INVALID);
    return Float80::quietNaN();
}

Float80 divideByZero(SoftFloat &sf, bool negative)
{
    sf.raiseFlags(FloatFlags::DIV_BY_ZERO);
    return Float80::infinity(negative);
}

// Returns pi/2 or pi/4 etc. with a sign
Float80 signed_(SoftFloat &sf, const W &w, bool negative) { W r = w; r.neg = negative; return finish(sf, r); }

}


//
// Trigonometric functions
//

static void
sincos_(SoftFloat &sf, const Float80 &x, Float80 &sine, Float80 &cosine)
{
    if (x.isNaN()) { sine = cosine = x; return; }
    if (x.isInfinite()) { sine = cosine = invalid(sf); return; }
    if (x.isZero()) { sine = x; cosine = Float80::one(); return; }

    int n;
    W r, s, c;
    reduce(fromFloat80(x), n, r);
    sinCosSeries(r, s, c);

    W sv = (n & 1) ? c : s;
    W cv = (n & 1) ? -s : c;
    if (n & 2) { sv = -sv; cv = -cv; }
    if (x.sign()) sv = -sv;

    sine = finish(sf, sv);
    cosine = finish(sf, cv);
}

void
FloatMath::sincos(const Float80 &x, Float80 &sine, Float80 &cosine)
{
    auto &sf = x.env();
    sincos_(sf, x, sine, cosine);
    sine = sine.in(sf);
    cosine = cosine.in(sf);
}

namespace {

// Computes sin and cos as wide numbers (the argument must be finite and nonzero)
void sincosW(const Float80 &x, W &sv, W &cv)
{
    int n;
    W r, s, c;
    reduce(fromFloat80(x), n, r);
    sinCosSeries(r, s, c);

    sv = (n & 1) ? c : s;
    cv = (n & 1) ? -s : c;
    if (n & 2) { sv = -sv; cv = -cv; }
    if (x.sign()) sv = -sv;
}

}

static Float80
sin_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN() || x.isZero()) return x;
    if (x.isInfinite()) return invalid(sf);

    W sv, cv;
    sincosW(x, sv, cv);
    return finish(sf, sv);
}

Float80
FloatMath::sin(const Float80 &x)
{
    auto &sf = x.env();
    return sin_(sf, x).in(sf);
}

static Float80
cos_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN()) return x;
    if (x.isInfinite()) return invalid(sf);
    if (x.isZero()) return Float80::one();

    W sv, cv;
    sincosW(x, sv, cv);
    return finish(sf, cv);
}

Float80
FloatMath::cos(const Float80 &x)
{
    auto &sf = x.env();
    return cos_(sf, x).in(sf);
}

static Float80
tan_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN()) return x;
    if (x.isInfinite()) return invalid(sf);
    if (x.isZero()) return x;

    int n;
    W r, s, c;
    reduce(fromFloat80(x), n, r);
    sinCosSeries(r, s, c);

    W t = (n & 1) ? -(c / s) : s / c;
    if (x.sign()) t = -t;

    return finish(sf, t);
}

Float80
FloatMath::tan(const Float80 &x)
{
    auto &sf = x.env();
    return tan_(sf, x).in(sf);
}


//
// Inverse trigonometric functions
//

static Float80
atan_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN() || x.isZero()) return x;
    if (x.isInfinite()) return signed_(sf, piOver2(), x.sign());

    return finish(sf, atanW(fromFloat80(x)));
}

Float80
FloatMath::atan(const Float80 &x)
{
    auto &sf = x.env();
    return atan_(sf, x).in(sf);
}

static Float80
asin_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN() || x.isZero()) return x;
    if (x.isInfinite()) return invalid(sf);

    W v = fromFloat80(x);
    int c = cmpAbs(v, one());
    if (c > 0) return invalid(sf);
    if (c == 0) return signed_(sf, piOver2(), x.sign());

    // asin(x) = atan(x / sqrt((1 - x)(1 + x)))
    W root = sqrt((one() - abs(v)) * (one() + abs(v)));
    W r = atanW(abs(v) / root);
    r.neg = x.sign();
    return finish(sf, r);
}

Float80
FloatMath::asin(const Float80 &x)
{
    auto &sf = x.env();
    return asin_(sf, x).in(sf);
}

static Float80
acos_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN()) return x;
    if (x.isInfinite()) return invalid(sf);

    W v = fromFloat80(x);
    int c = cmpAbs(v, one());
    if (c > 0) return invalid(sf);
    if (c == 0) return x.sign() ? finish(sf, pi()) : Float80::zero();

    // acos(x) = 2 atan(sqrt((1 - x) / (1 + x)))
    W r = scale(atanW(sqrt((one() - v) / (one() + v))), 1);
    return finish(sf, r);
}

Float80
FloatMath::acos(const Float80 &x)
{
    auto &sf = x.env();
    return acos_(sf, x).in(sf);
}


//
// Hyperbolic functions
//

static Float80
sinh_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN() || x.isZero() || x.isInfinite()) return x;

    // sinh(x) = (E + E / (E + 1)) / 2, where E = e^|x| - 1
    W v = abs(fromFloat80(x));
    W r;

    if (cmpAbs(v, scale(one(), -32)) < 0) {

        // For tiny arguments, sinh(x) = x + x^3 / 6 is slightly larger than x. Don't compute it via
        // expm1, because rounding errors could push the result below x and signal a false underflow.
        r = v + scale(v, -65);

    } else {

        W e = expm1W(v);
        r = scale(e + e / (e + one()), -1);
    }
    r.neg = x.sign();
    return finish(sf, r);
}

Float80
FloatMath::sinh(const Float80 &x)
{
    auto &sf = x.env();
    return sinh_(sf, x).in(sf);
}

static Float80
cosh_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN()) return x;
    if (x.isInfinite()) return Float80::infinity();
    if (x.isZero()) return Float80::one();

    W e = expW(abs(fromFloat80(x)));
    return finish(sf, scale(e + one() / e, -1));
}

Float80
FloatMath::cosh(const Float80 &x)
{
    auto &sf = x.env();
    return cosh_(sf, x).in(sf);
}

static Float80
tanh_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN() || x.isZero()) return x;
    if (x.isInfinite()) return Float80::one(x.sign());

    // tanh(x) = E / (E + 2), where E = e^(2|x|) - 1
    W e = expm1W(scale(abs(fromFloat80(x)), 1));
    W r = e / (e + two());
    r.neg = x.sign();

    // For large arguments, the quotient rounds to 1, but the exact result is slightly smaller.
    // Use the closest value below 1 to let the rounding mode take effect.
    if (r.e == one().e && r.m == one().m) { r.e -= 1; r.m = ~u128(0); }

    return finish(sf, r);
}

Float80
FloatMath::tanh(const Float80 &x)
{
    auto &sf = x.env();
    return tanh_(sf, x).in(sf);
}

static Float80
atanh_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN() || x.isZero()) return x;
    if (x.isInfinite()) return invalid(sf);

    W v = abs(fromFloat80(x));
    int c = cmpAbs(v, one());
    if (c > 0) return invalid(sf);
    if (c == 0) return divideByZero(sf, x.sign());

    // atanh(x) = ln(1 + 2x / (1 - x)) / 2
    W r = scale(lognp1W(scale(v, 1) / (one() - v)), -1);
    r.neg = x.sign();
    return finish(sf, r);
}

Float80
FloatMath::atanh(const Float80 &x)
{
    auto &sf = x.env();
    return atanh_(sf, x).in(sf);
}


//
// Exponential functions
//

static Float80
etox_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN()) return x;
    if (x.isInfinite()) return x.sign() ? Float80::zero() : x;
    if (x.isZero()) return Float80::one();

    return finish(sf, expW(fromFloat80(x)));
}

Float80
FloatMath::etox(const Float80 &x)
{
    auto &sf = x.env();
    return etox_(sf, x).in(sf);
}

static Float80
etoxm1_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN() || x.isZero()) return x;
    if (x.isInfinite()) {
        return x.sign() ? Float80::one(true) : x;
    }
    /* For tiny arguments, e^x - 1 equals x within the precision of the extended format. What
     * remains to be done is to raise the right flags and to round the result as requested.
     * The reference behavior is to add the smallest normal number with a negative sign (using
     * the precision and rounding mode of the instruction). This lets the result land just
     * below x in directed rounding modes and makes it tiny if x is a power of two at the lower
     * end of the destination range. The addend is only applied if it doesn't dominate x (if the
     * exponent field is at least 0x33). Otherwise, x is simply rounded to the destination format.
     * Note: The threshold and the addend can't be derived from the math. They reproduce the
     * observable behavior of the reference.
     */
    if ((x.signExp() & 0x7FFF) < Float80::bias - 65) {

        if ((x.signExp() & 0x7FFF) < 0x33) return finish(sf, fromFloat80(x));

        auto result = x + Float80::fromBits(0x8001, 1ULL << 63);
        sf.raiseFlags(FloatFlags::INEXACT);
        return result;
    }

    W v = fromFloat80(x);

    // Large negative arguments yield -1 + epsilon
    if (v.neg && v.e > 6) return finish(sf, norm(true, -1, ~u128(0)));

    return finish(sf, expm1W(v));
}

Float80
FloatMath::etoxm1(const Float80 &x)
{
    auto &sf = x.env();
    return etoxm1_(sf, x).in(sf);
}

static Float80
twotox_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN()) return x;
    if (x.isInfinite()) return x.sign() ? Float80::zero() : x;
    if (x.isZero()) return Float80::one();

    W v = fromFloat80(x);

    // Integer exponents yield an exact result
    if (x.exponent() >= 0 && x.exponent() < 20 && (x.mantissa() << (x.exponent() + 1)) == 0) {

        i64 n = i64(x.mantissa() >> (63 - x.exponent()));
        return finish(sf, norm(false, x.sign() ? -i32(n) : i32(n), u128(1) << 127));
    }
    if (v.e > 20) v = norm(v.neg, 20, u128(1) << 127);
    return finish(sf, expW(v * ln2()));
}

Float80
FloatMath::twotox(const Float80 &x)
{
    auto &sf = x.env();
    return twotox_(sf, x).in(sf);
}

static Float80
tentox_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN()) return x;
    if (x.isInfinite()) return x.sign() ? Float80::zero() : x;
    if (x.isZero()) return Float80::one();

    W v = fromFloat80(x);

    // 10^n is exact for 0 <= n <= 27
    if (!x.sign() && x.exponent() >= 0 && x.exponent() < 5 && (x.mantissa() << (x.exponent() + 1)) == 0) {

        i64 n = i64(x.mantissa() >> (63 - x.exponent()));
        if (n <= 27) {
            u128 p = 1;
            for (i64 i = 0; i < n; i++) p *= 10;
            return finish(sf, norm(false, 127, p));
        }
    }
    if (v.e > 20) v = norm(v.neg, 20, u128(1) << 127);
    return finish(sf, expW(v * ln10()));
}

Float80
FloatMath::tentox(const Float80 &x)
{
    auto &sf = x.env();
    return tentox_(sf, x).in(sf);
}


//
// Logarithms
//

namespace {

enum class Base { E, TWO, TEN };

Float80 logarithm(SoftFloat &sf, const Float80 &x, Base base)
{
    if (x.isNaN()) return x;
    if (x.isZero()) return divideByZero(sf, true);
    if (x.sign()) return invalid(sf);
    if (x.isInfinite()) return x;

    W v = fromFloat80(x);

    // Exact results
    if (base == Base::TWO && (x.mantissa() << 1) == 0) {
        sf.raiseFlags(FloatFlags::INEXACT);
        return Float80::fromInt(x.exponent(), sf);
    }
    if (base == Base::TEN) {
        u128 p = 1;
        for (int n = 0; n <= 27; n++, p *= 10) {
            W t = norm(false, 127, p);
            if (v.e == t.e && v.m == t.m) { sf.raiseFlags(FloatFlags::INEXACT); return Float80::fromInt(n, sf); }
        }
    }
    W r = lnW(v);
    if (base == Base::TWO) r = r * invLn2();
    if (base == Base::TEN) r = r / ln10();
    return finish(sf, r);
}

}

Float80 FloatMath::logn(const Float80 &x) { auto &sf = x.env(); return logarithm(sf, x, Base::E).in(sf); }
Float80 FloatMath::log2(const Float80 &x) { auto &sf = x.env(); return logarithm(sf, x, Base::TWO).in(sf); }
Float80 FloatMath::log10(const Float80 &x) { auto &sf = x.env(); return logarithm(sf, x, Base::TEN).in(sf); }

static Float80
lognp1_(SoftFloat &sf, const Float80 &x)
{
    if (x.isNaN() || x.isZero()) return x;
    if (x.isInfinite()) return x.sign() ? invalid(sf) : x;

    W v = fromFloat80(x);
    if (v.neg) {
        int c = cmpAbs(v, one());
        if (c > 0) return invalid(sf);
        if (c == 0) return divideByZero(sf, true);
    }
    return finish(sf, lognp1W(v));
}

Float80
FloatMath::lognp1(const Float80 &x)
{
    auto &sf = x.env();
    return lognp1_(sf, x).in(sf);
}

}
