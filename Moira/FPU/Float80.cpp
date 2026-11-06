// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "Float80.h"
#include <algorithm>
#include <bit>
#include <cfloat>
#include <cstring>

namespace MOIRA_NAMESPACE {

namespace {

/* Conversion between Float80 and the SoftFloat representation
 *
 * Float80 uses the encoding of the Motorola FPUs, whereas SoftFloat follows
 * the x87. The two differ in the representation of infinity: The 68k FPUs
 * store an infinity with an all-zero mantissa, the x87 sets the integer bit.
 * The conversion functions translate between the two. The remaining
 * differences (denormals, exponent range) are handled elsewhere.
 */
inline extFloat80_t
toSF(const Float80 &x)
{
    extFloat80_t result;
    result.signExp = x.signExp();
    result.signif = x.isInfinite() ? (1ULL << 63) : x.mantissa();
    return result;
}

inline Float80
fromSF(const extFloat80_t &x)
{
    bool infinity = (x.signExp & 0x7FFF) == 0x7FFF && (x.signif << 1) == 0;
    return Float80::fromBits(x.signExp, infinity ? 0 : x.signif);
}

using u128 = unsigned __int128;

/* Returns the offset of the exponent range in effect. With a reduced precision,
 * the exponent range of the respective format applies (single: 0x3F80, double:
 * 0x3C00), unless the limit is switched off.
 */
inline i32
exponentOffset(SoftFloat &sf)
{
    if (!sf.limitsExponent()) return 0;

    switch (sf.precision()) {

        case Precision::SINGLE:     return 0x3F80;
        case Precision::DOUBLE:     return 0x3C00;
        default:                    return 0;
    }
}

/* Rounds a value to the current precision and exponent range
 *
 * The value is given as sign, biased exponent and a normalized significand
 * (integer bit set) plus up to 64 additional bits below the significand. This
 * is the rounding step of the Motorola FPUs:
 *
 *  - A result beyond the largest number of the format raises OVFL and INEX2
 *    and becomes infinity, or the largest number if the rounding mode points
 *    towards zero.
 *  - A result below the smallest normalized number raises UNFL (tininess is
 *    detected before rounding, even if the result is exact) and is
 *    denormalized. Denormals are stored with an unnormalized significand and
 *    the exponent of the smallest normalized number. The extended format is
 *    special, because a denormal has the biased exponent 0 whereas the smallest
 *    normalized number has the biased exponent 1. A number with the exponent 0
 *    and the integer bit set is thus a regular result that is not tiny.
 */
Float80
roundToFormat(SoftFloat &sf, bool sign, i32 exp, u64 sig, u64 extra = 0)
{
    auto precision = sf.precision();
    int bits = precision == Precision::SINGLE ? 24 : precision == Precision::DOUBLE ? 53 : 64;
    i32 offset = exponentOffset(sf);
    i32 lowest = offset ? offset + 1 : 0;
    i32 maxExp = 0x7FFE - offset;
    auto mode = sf.roundingMode();

    u128 t = u128(sig) << 64 | extra;
    bool sticky = false;

    // Denormalize
    bool tiny = exp < lowest;
    if (tiny) {
        i32 shift = lowest - exp;
        if (shift >= 128) {
            sticky = t != 0;
            t = 0;
        } else {
            sticky = (t & ((u128(1) << shift) - 1)) != 0;
            t >>= shift;
        }
    }

    // Round
    int drop = 128 - bits;
    u128 unit = u128(1) << drop;
    u128 half = unit >> 1;
    u128 frac = t & (unit - 1);
    u128 kept = t >> drop;
    bool inexact = frac != 0 || sticky;
    bool roundBit = (frac & half) != 0;
    bool rest = (frac & (half - 1)) != 0 || sticky;

    bool increment = false;
    switch (mode) {

        case RoundingMode::NEAREST: increment = roundBit && (rest || (kept & 1)); break;
        case RoundingMode::ZERO:    increment = false; break;
        case RoundingMode::DOWN:    increment = sign && inexact; break;
        case RoundingMode::UP:      increment = !sign && inexact; break;
    }
    kept += increment;
    if (!tiny && (kept >> bits)) { kept >>= 1; exp++; }

    // Overflow
    if (exp > maxExp) {

        sf.raiseFlags(FloatFlags::OVFL | FloatFlags::INEXACT);
        bool towardsZero = mode == RoundingMode::ZERO ||
                           (sign && mode == RoundingMode::UP) || (!sign && mode == RoundingMode::DOWN);
        u64 largest = bits == 64 ? ~0ULL : (~0ULL << (64 - bits));
        return towardsZero ? Float80::fromBits(u16(maxExp | (sign ? 0x8000 : 0)), largest)
                           : Float80::infinity(sign);
    }

    if (inexact) sf.raiseFlags(FloatFlags::INEXACT);
    if (tiny) sf.raiseFlags(FloatFlags::UNFL);

    // A result that rounds to zero is a regular zero
    if (kept == 0) return Float80::zero(sign);

    u64 result = u64(kept) << (64 - bits);
    return Float80::fromBits(u16((tiny ? lowest : exp) | (sign ? 0x8000 : 0)), result);
}

/* Normalized representation of a finite number. The exponent is the biased
 * exponent that applies to the normalized significand. It is not limited to the
 * range of the format. In contrast to the x87, a biased exponent of 0 is a
 * regular exponent (the 68k FPUs don't treat it as 1).
 */
struct Num {

    bool sign;
    bool zero;
    i32 exp;
    u64 sig;

    explicit Num(const Float80 &x) : sign(x.sign()), zero(x.mantissa() == 0), exp(x.biasedExponent()), sig(x.mantissa())
    {
        if (!zero) {
            int shift = std::countl_zero(sig);
            sig <<= shift;
            exp -= shift;
        }
    }

    // Creates an x87 compatible number with the given biased exponent
    Float80 rebased(i32 field) const
    {
        return zero ? Float80::zero(sign) : Float80::fromBits(u16(field | (sign ? 0x8000 : 0)), sig);
    }
};

// Checks if a number is in a form that SoftFloat processes like the 68k FPUs
inline bool
isRegular(const Float80 &x)
{
    if (!x.isFinite() || x.mantissa() == 0) return true;
    return x.biasedExponent() != 0 && x.integerBit();
}

// Selects the environment of a binary operation (the left operand takes precedence)
inline SoftFloat &
envOf(const Float80 &a, const Float80 &b)
{
    return a.isBound() ? a.env() : b.env();
}

enum class Op { ADD, SUB, MUL, DIV, SQRT };

Float80
plain(SoftFloat &sf, Op op, const Float80 &a, const Float80 &b)
{
    switch (op) {

        case Op::ADD:   return fromSF(sf.add(toSF(a), toSF(b)));
        case Op::SUB:   return fromSF(sf.sub(toSF(a), toSF(b)));
        case Op::MUL:   return fromSF(sf.mul(toSF(a), toSF(b)));
        case Op::DIV:   return fromSF(sf.div(toSF(a), toSF(b)));
        case Op::SQRT:  return fromSF(sf.sqrt(toSF(a)));
    }
    return Float80();
}

/* Executes an operation on arbitrary finite operands, including denormals,
 * unnormals and results that lie outside the range of the target format.
 *
 * The operands are rebased to the middle of the exponent range of the x87, where
 * SoftFloat computes the exact result bits. The exponent of the result is
 * corrected afterwards. If the result is close to or outside the limits of the
 * target format, the operation is repeated with truncation to obtain the exact
 * result bits including a sticky flag, which are then rounded by roundToFormat.
 */
Float80
exactOp(SoftFloat &sf, Op op, const Float80 &a, const Float80 &b)
{
    constexpr i32 mid = Float80::bias;

    Num x(a), y(b);
    Float80 ra, rb;
    i32 delta = 0;

    switch (op) {

        case Op::ADD:
        case Op::SUB: {

            // Keep the distance between the operands, but limit it to avoid denormals
            i32 top = x.zero ? y.exp : y.zero ? x.exp : std::max(x.exp, y.exp);
            ra = x.rebased(mid - std::min(top - x.exp, 300));
            rb = y.rebased(mid - std::min(top - y.exp, 300));
            delta = top - mid;
            break;
        }
        case Op::MUL:
            ra = x.rebased(mid);
            rb = y.rebased(mid);
            delta = (x.exp - mid) + (y.exp - mid);
            break;

        case Op::DIV:
            ra = x.rebased(mid);
            rb = y.rebased(mid);
            delta = (x.exp - mid) - (y.exp - mid);
            break;

        case Op::SQRT: {

            // The parity of the exponent must be preserved
            i32 field = mid + ((x.exp - mid) & 1);
            ra = x.rebased(field);
            delta = (x.exp - field) / 2;
            break;
        }
    }

    auto saved = sf.flags();
    auto mode = sf.roundingMode();
    auto precision = sf.precision();
    i32 offset = exponentOffset(sf);
    i32 lowest = offset ? offset + 1 : 0;
    i32 maxExp = 0x7FFE - offset;

    sf.clearFlags();
    sf.setPrecision(Precision::EXTENDED);

    // Run 1: Execute the operation with the selected rounding mode
    auto r1 = plain(sf, op, ra, rb);
    auto flags1 = sf.flags();

    // Special results are final (zeros, NaNs, division by zero, ...)
    if (!r1.isFinite() || r1.isZero()) {

        sf.setPrecision(precision);
        sf.clearFlags();
        sf.raiseFlags(saved | flags1);
        return r1;
    }

    // An extended precision result that is not tiny is already rounded correctly
    i32 exp1 = r1.biasedExponent() + delta;
    auto final1 = [&] {

        sf.clearFlags();
        sf.raiseFlags(saved | flags1);
        if (exp1 > maxExp) return roundToFormat(sf, r1.sign(), exp1, r1.mantissa());
        return Float80::fromBits(u16(exp1 | (r1.sign() ? 0x8000 : 0)), r1.mantissa());
    };
    if (precision == Precision::EXTENDED && exp1 >= lowest + 1) return final1();

    // Run 2: Truncate to get the exact result bits and a sticky flag
    sf.clearFlags();
    sf.setRoundingMode(RoundingMode::ZERO);
    auto r2 = plain(sf, op, ra, rb);
    bool sticky = sf.flags() & FloatFlags::INEXACT;

    sf.setRoundingMode(mode);
    sf.setPrecision(precision);
    sf.clearFlags();
    sf.raiseFlags(saved);

    // Tininess is detected before rounding
    i32 exp2 = r2.biasedExponent() + delta;
    if (precision == Precision::EXTENDED && exp2 >= lowest) return final1();

    return roundToFormat(sf, r2.sign(), r2.biasedExponent() + delta, r2.mantissa(), sticky ? 1 : 0);
}

/* Executes an arithmetic operation. Regular operands are first processed by
 * SoftFloat directly. If the result is close to or outside the limits of the
 * target format, or if an operand is a denormal or unnormal, the operation is
 * executed by exactOp.
 */
Float80
arith(SoftFloat &sf, Op op, const Float80 &a, const Float80 &b = Float80())
{
    bool unary = op == Op::SQRT;

    if (isRegular(a) && (unary || isRegular(b))) {

        auto saved = sf.flags();
        sf.clearFlags();
        auto result = plain(sf, op, a, b);
        auto flags = sf.flags();

        // Check if the result is safely inside the range of the format
        i32 offset = exponentOffset(sf);
        bool special = !result.isFinite() || result.isZero();
        bool overflown = result.isInfinite() && (flags & FloatFlags::OVFL);
        bool lost = result.isZero() && (flags & (FloatFlags::INEXACT | FloatFlags::UNFL));
        i32 exp = result.biasedExponent();
        bool inside = special ? !overflown && !lost : (exp >= offset + 2 && exp <= 0x7FFE - offset - 1);

        if (inside) {
            sf.raiseFlags(saved);
            return result;
        }
        sf.clearFlags();
        sf.raiseFlags(saved);
    }

    // NaNs and infinities are processed as they are
    if (!a.isFinite() || (!unary && !b.isFinite())) return plain(sf, op, a, b);

    return exactOp(sf, op, a, b);
}

}


//
// Float80: Creating numbers
//

Float80
Float80::in(SoftFloat &sf) const
{
    // Numbers in the standard environment stay unbound (they may be used by other threads)
    Float80 result = *this;
    result.bind(sf);
    return result;
}

Float80
Float80::fromInt(i64 value, SoftFloat &sf)
{
    // The conversion is always exact
    return fromSF(sf.fromI64(value)).in(sf);
}

Float80
Float80::fromFloat(float value, SoftFloat &sf)
{
    // The conversion is always exact. Signaling NaNs raise the invalid flag.
    return fromSF(sf.fromF32(float32_t{ std::bit_cast<u32>(value) })).in(sf);
}

Float80
Float80::fromDouble(double value, SoftFloat &sf)
{
    // The conversion is always exact. Signaling NaNs raise the invalid flag.
    return fromSF(sf.fromF64(float64_t{ std::bit_cast<u64>(value) })).in(sf);
}

Float80
Float80::roundPack(bool sign, i32 exp, u64 sig, u64 extra, SoftFloat &sf)
{
    if (sig == 0 && extra == 0) return zero(sign).in(sf);
    if (sig == 0) { sig = extra; extra = 0; exp -= 64; }

    int shift = std::countl_zero(sig);
    sig = shift ? sig << shift | extra >> (64 - shift) : sig;
    extra <<= shift;
    exp -= shift;

    return roundToFormat(sf, sign, exp, sig, extra).in(sf);
}

Float80
Float80::rounded() const
{
    if (!isFinite() || mantissa() == 0) return *this;

    // Regular extended precision values are not changed
    auto &sf = env();
    if (isRegular(*this) && exponentOffset(sf) == 0 && sf.precision() == Precision::EXTENDED) return *this;

    Num n(*this);
    return roundToFormat(sf, n.sign, n.exp, n.sig).in(sf);
}

Float80
Float80::fromLongDouble(long double value)
{
    if constexpr (LDBL_MANT_DIG == 64 && std::endian::native == std::endian::little) {

        // x87 extended format: 8 bytes mantissa followed by 2 bytes sign and exponent
        u64 m;
        u16 e;
        std::memcpy(&m, &value, sizeof(m));
        std::memcpy(&e, reinterpret_cast<const char *>(&value) + sizeof(m), sizeof(e));
        return fromBits(e, m);

    } else {

        return fromDouble((double)value);
    }
}


//
// Float80: Converting to other number formats
//

i32
Float80::toInt32() const
{
    return env().toI32(toSF(*this), env().env.roundingMode, true);
}

i32
Float80::toInt32Truncated() const
{
    return env().toI32(toSF(*this), softfloat_round_minMag, true);
}

i64
Float80::toInt64() const
{
    return env().toI64(toSF(*this), env().env.roundingMode, true);
}

i64
Float80::toInt64Truncated() const
{
    return env().toI64(toSF(*this), softfloat_round_minMag, true);
}

float
Float80::toFloat() const
{
    return std::bit_cast<float>(env().toF32(toSF(*this)).v);
}

double
Float80::toDouble() const
{
    return std::bit_cast<double>(env().toF64(toSF(*this)).v);
}

long double
Float80::toLongDouble() const
{
    if constexpr (LDBL_MANT_DIG == 64 && std::endian::native == std::endian::little) {

        long double result = 0;
        std::memcpy(&result, &man, sizeof(man));
        std::memcpy(reinterpret_cast<char *>(&result) + sizeof(man), &se, sizeof(se));
        return result;

    } else {

        return (long double)toDouble();
    }
}

std::string
Float80::str() const
{
    char buf[32];
    std::snprintf(buf, sizeof(buf), "0x%04X:%016llX", (unsigned)se, (unsigned long long)man);
    return buf;
}


//
// Float80: Arithmetic
//

Float80
Float80::operator-() const
{
    // Flipping the sign is exact and raises no exception
    Float80 result = *this;
    result.se ^= 0x8000;
    return result;
}

Float80
Float80::abs() const
{
    Float80 result = *this;
    result.se &= 0x7FFF;
    return result;
}

Float80
Float80::operator+(const Float80 &rhs) const
{
    auto &sf = envOf(*this, rhs);
    return arith(sf, Op::ADD, *this, rhs).in(sf);
}

Float80
Float80::operator-(const Float80 &rhs) const
{
    auto &sf = envOf(*this, rhs);
    return arith(sf, Op::SUB, *this, rhs).in(sf);
}

Float80
Float80::operator*(const Float80 &rhs) const
{
    auto &sf = envOf(*this, rhs);
    return arith(sf, Op::MUL, *this, rhs).in(sf);
}

Float80
Float80::operator/(const Float80 &rhs) const
{
    auto &sf = envOf(*this, rhs);
    return arith(sf, Op::DIV, *this, rhs).in(sf);
}

Float80
Float80::sqrt() const
{
    auto &sf = env();
    return arith(sf, Op::SQRT, *this).in(sf);
}

Float80
Float80::rem(const Float80 &rhs) const
{
    auto &sf = envOf(*this, rhs);
    return fromSF(sf.rem(toSF(*this), toSF(rhs))).in(sf);
}

Float80
Float80::roundToInt() const
{
    auto &sf = env();
    return fromSF(sf.roundToInt(toSF(*this), sf.env.roundingMode, true)).in(sf);
}

Float80
Float80::scale(const Float80 &rhs) const
{
    auto &sf = envOf(*this, rhs);

    // NaNs propagate (and signaling NaNs raise the invalid flag)
    if (isNaN() || rhs.isNaN()) return *this + rhs;

    // Scaling by an infinite factor is undefined
    if (rhs.isInfinite()) {
        sf.raiseFlags(FloatFlags::INVALID);
        return quietNaN().in(sf);
    }

    // Zeros and infinities are not changed
    if (isZero() || isInfinite()) return *this;

    // Determine the scale factor. Factors beyond the exponent range of the
    // format are clamped, because they overflow or underflow anyway.
    i32 n;
    if (rhs.exponent() >= 20) {
        n = rhs.sign() ? -0x100000 : 0x100000;
    } else {
        // Truncating discards the fraction without being inexact
        auto saved = sf.flags();
        n = rhs.toInt32Truncated();
        sf.clearFlags();
        sf.raiseFlags(saved);
    }

    // Normalize the mantissa
    Num x(*this);

    // The rounding step takes care of overflows and underflows
    return roundToFormat(sf, x.sign, x.exp + std::clamp(n, -0x100000, 0x100000), x.sig).in(sf);
}


//
// Float80: Comparing
//

bool
Float80::operator==(const Float80 &rhs) const
{
    return envOf(*this, rhs).eq(toSF(*this), toSF(rhs));
}

bool
Float80::operator<(const Float80 &rhs) const
{
    return envOf(*this, rhs).lt(toSF(*this), toSF(rhs));
}

bool
Float80::operator<=(const Float80 &rhs) const
{
    return envOf(*this, rhs).le(toSF(*this), toSF(rhs));
}

}
