// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "Float80.h"
#include "softfloat/platform.h"
#include "softfloat/moira_env.h"
#include <bit>
#include <cfloat>
#include <cstring>

extern "C" {
#include "softfloat/softfloat.h"
#include "softfloat/internals.h"
}

namespace moira {

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

inline uint_fast8_t
roundingModeOf()
{
    return moira_sf_getRoundingMode();
}

thread_local bool limitExponent = true;

/* Rounds a value to single or double precision including the exponent range
 * of the respective format.
 *
 * The value is given as sign, biased exponent and a normalized significand
 * (integer bit set) plus a sticky flag that indicates nonzero bits below the
 * significand. This is the rounding step of the Motorola FPUs:
 *
 *  - A result beyond the largest number of the format raises OVFL and INEX2
 *    and becomes infinity, or the largest number if the rounding mode points
 *    towards zero.
 *  - A result below the smallest normalized number of the format raises UNFL
 *    (tininess is detected before rounding) and is denormalized. The result is
 *    stored with the exponent of the smallest normalized number and an
 *    unnormalized significand.
 */
Float80
roundToFormat(bool sign, i32 exp, u64 sig, bool sticky)
{
    bool single = FloatEnv::precision() == Precision::SINGLE;
    i32 offset = single ? 0x3F80 : 0x3C00;
    u64 mask = single ? 0x000000FFFFFFFFFFULL : 0x00000000000007FFULL;
    u64 inc = single ? 0x0000008000000000ULL : 0x0000000000000400ULL;
    auto mode = FloatEnv::roundingMode();
    bool nearest = mode == RoundingMode::NEAREST;

    // Determine the rounding increment
    if (!nearest) {
        if (mode == RoundingMode::ZERO) inc = 0;
        else inc = ((mode == RoundingMode::UP) == !sign) ? mask : 0;
    }
    bool towardsZero = mode == RoundingMode::ZERO ||
                       (sign && mode == RoundingMode::UP) || (!sign && mode == RoundingMode::DOWN);

    sig |= sticky;
    u64 roundBits = sig & mask;

    // Overflow
    i32 maxExp = 0x7FFE - offset;
    if (exp > maxExp || (exp == maxExp && sig + inc < sig)) {

        FloatEnv::raiseFlags(FloatFlags::OVFL | FloatFlags::INEXACT);
        return towardsZero ? Float80::fromBits(u16(maxExp | (sign ? 0x8000 : 0)), ~mask)
                           : Float80::infinity(sign);
    }

    // Underflow
    if (exp < offset + 1) {

        FloatEnv::raiseFlags(FloatFlags::UNFL);

        i32 shift = offset + 1 - exp;
        sig = shift >= 64 ? (sig != 0) : (sig >> shift) | ((sig << (64 - shift)) != 0);
        roundBits = sig & mask;
        if (roundBits) FloatEnv::raiseFlags(FloatFlags::INEXACT);

        sig += inc;
        if (nearest && roundBits == inc) mask |= inc << 1;
        sig &= ~mask;

        // A result that rounds to zero is a regular zero
        if (sig == 0) return Float80::zero(sign);
        return Float80::fromBits(u16((offset + 1) | (sign ? 0x8000 : 0)), sig);
    }

    // Regular case
    if (roundBits) FloatEnv::raiseFlags(FloatFlags::INEXACT);

    sig += inc;
    if (sig < inc) { exp++; sig = 1ULL << 63; }

    u64 half = mask + 1;
    if (nearest && (roundBits << 1) == half) mask |= half;
    sig &= ~mask;

    return Float80::fromBits(u16(exp | (sign ? 0x8000 : 0)), sig);
}

// Checks whether the exponent range of the current precision has to be applied
inline bool
limitsExponentRange()
{
    return limitExponent && FloatEnv::precision() != Precision::EXTENDED;
}

/* Executes an arithmetic operation, taking the exponent range of the current
 * precision into account. The operation is first executed by SoftFloat, which
 * rounds the significand correctly, but allows the full extended exponent
 * range. If the result lies close to or outside the limits of the target
 * format, the operation is repeated with truncation to obtain the exact
 * result bits, which are then rounded by roundToFormat.
 */
template <typename F> Float80
ranged(F operation)
{
    if (!limitsExponentRange()) return operation();

    auto saved = FloatEnv::flags();
    FloatEnv::clearFlags();
    auto result = operation();
    auto flags = FloatEnv::flags();

    // Check if the result is safely inside the range of the format
    i32 offset = FloatEnv::precision() == Precision::SINGLE ? 0x3F80 : 0x3C00;
    bool special = !result.isFinite() || result.isZero();
    bool overflown = result.isInfinite() && (flags & FloatFlags::OVFL);
    i32 exp = result.biasedExponent();
    bool inside = special ? !overflown : (exp >= offset + 2 && exp <= 0x7FFE - offset - 1);

    if (inside) {
        FloatEnv::raiseFlags(saved);
        return result;
    }

    // Repeat the operation with truncation to get the exact bits and a sticky flag
    auto mode = FloatEnv::roundingMode();
    auto precision = FloatEnv::precision();
    FloatEnv::clearFlags();
    FloatEnv::setRoundingMode(RoundingMode::ZERO);
    FloatEnv::setPrecision(Precision::EXTENDED);
    auto exact = operation();
    bool sticky = FloatEnv::flags() & FloatFlags::INEXACT;
    FloatEnv::setRoundingMode(mode);
    FloatEnv::setPrecision(precision);
    FloatEnv::clearFlags();
    FloatEnv::raiseFlags(saved);

    // Bring the significand into normalized form
    i32 e = exact.biasedExponent() ? exact.biasedExponent() : 1;
    u64 sig = exact.mantissa();
    if (sig == 0) { e = -100000; sig = 1ULL << 63; sticky = true; }
    int shift = std::countl_zero(sig);
    sig <<= shift;
    e -= shift;

    return roundToFormat(exact.sign(), e, sig, sticky);
}

}


//
// FloatEnv
//

RoundingMode
FloatEnv::roundingMode()
{
    switch (moira_sf_getRoundingMode()) {

        case softfloat_round_minMag:    return RoundingMode::ZERO;
        case softfloat_round_min:       return RoundingMode::DOWN;
        case softfloat_round_max:       return RoundingMode::UP;
        default:                        return RoundingMode::NEAREST;
    }
}

void
FloatEnv::setRoundingMode(RoundingMode mode)
{
    switch (mode) {

        case RoundingMode::NEAREST: moira_sf_setRoundingMode(softfloat_round_near_even); break;
        case RoundingMode::ZERO:    moira_sf_setRoundingMode(softfloat_round_minMag); break;
        case RoundingMode::DOWN:    moira_sf_setRoundingMode(softfloat_round_min); break;
        case RoundingMode::UP:      moira_sf_setRoundingMode(softfloat_round_max); break;
    }
}

Precision
FloatEnv::precision()
{
    switch (moira_sf_getPrecision()) {

        case 32:    return Precision::SINGLE;
        case 64:    return Precision::DOUBLE;
        default:    return Precision::EXTENDED;
    }
}

void
FloatEnv::setPrecision(Precision precision)
{
    moira_sf_setPrecision((uint_fast8_t)precision);
}

bool
FloatEnv::limitsExponent()
{
    return limitExponent;
}

void
FloatEnv::setLimitsExponent(bool value)
{
    limitExponent = value;
}

u8
FloatEnv::flags()
{
    return (u8)moira_sf_getFlags();
}

void
FloatEnv::clearFlags(u8 mask)
{
    moira_sf_setFlags((uint_fast8_t)(moira_sf_getFlags() & ~mask));
}

void
FloatEnv::raiseFlags(u8 mask)
{
    moira_sf_setFlags((uint_fast8_t)(moira_sf_getFlags() | (mask & FloatFlags::ALL)));
}

FloatEnv::Scope::Scope(bool restoreFlags) : restoreFlags(restoreFlags)
{
    savedMode = FloatEnv::roundingMode();
    savedPrecision = FloatEnv::precision();
    savedLimits = FloatEnv::limitsExponent();
    savedFlags = FloatEnv::flags();
}

FloatEnv::Scope::~Scope()
{
    FloatEnv::setRoundingMode(savedMode);
    FloatEnv::setPrecision(savedPrecision);
    FloatEnv::setLimitsExponent(savedLimits);

    if (restoreFlags) {
        FloatEnv::clearFlags();
        FloatEnv::raiseFlags(savedFlags);
    }
}


//
// Float80: Creating numbers
//

Float80
Float80::fromInt(i64 value)
{
    // The conversion is always exact
    return fromSF(moira_i64_to_extF80(value));
}

Float80
Float80::fromFloat(float value)
{
    // The conversion is always exact. Signaling NaNs raise the invalid flag.
    return fromSF(moira_f32_to_extF80(float32_t{ std::bit_cast<u32>(value) }));
}

Float80
Float80::fromDouble(double value)
{
    // The conversion is always exact. Signaling NaNs raise the invalid flag.
    return fromSF(moira_f64_to_extF80(float64_t{ std::bit_cast<u64>(value) }));
}

Float80
Float80::roundPack(bool sign, i32 exp, u64 sig, u64 extra)
{
    if (sig == 0 && extra == 0) return zero(sign);
    if (sig == 0) { sig = extra; extra = 0; exp -= 64; }

    return fromSF(moira_softfloat_normRoundPackToExtF80(sign, exp, sig, extra, moira_sf_getPrecision()));
}

Float80
Float80::rounded() const
{
    if (!isFinite() || isZero()) return *this;

    // Without an exponent limit, only the significand is rounded
    if (!limitsExponentRange()) return roundPack(sign(), std::max<i32>(1, biasedExponent()), mantissa());

    // Normalize the significand
    i32 e = biasedExponent() ? biasedExponent() : 1;
    u64 m = mantissa();
    int shift = std::countl_zero(m);
    m <<= shift;
    e -= shift;

    return roundToFormat(sign(), e, m, false);
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
    return moira_extF80_to_i32(toSF(*this), roundingModeOf(), true);
}

i32
Float80::toInt32Truncated() const
{
    return moira_extF80_to_i32(toSF(*this), softfloat_round_minMag, true);
}

i64
Float80::toInt64() const
{
    return moira_extF80_to_i64(toSF(*this), roundingModeOf(), true);
}

i64
Float80::toInt64Truncated() const
{
    return moira_extF80_to_i64(toSF(*this), softfloat_round_minMag, true);
}

float
Float80::toFloat() const
{
    return std::bit_cast<float>(moira_extF80_to_f32(toSF(*this)).v);
}

double
Float80::toDouble() const
{
    return std::bit_cast<double>(moira_extF80_to_f64(toSF(*this)).v);
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
    return fromBits(u16(se ^ 0x8000), man);
}

Float80
Float80::abs() const
{
    return fromBits(u16(se & 0x7FFF), man);
}

Float80
Float80::operator+(const Float80 &rhs) const
{
    return ranged([&] { return fromSF(moira_extF80_add(toSF(*this), toSF(rhs))); });
}

Float80
Float80::operator-(const Float80 &rhs) const
{
    return ranged([&] { return fromSF(moira_extF80_sub(toSF(*this), toSF(rhs))); });
}

Float80
Float80::operator*(const Float80 &rhs) const
{
    return ranged([&] { return fromSF(moira_extF80_mul(toSF(*this), toSF(rhs))); });
}

Float80
Float80::operator/(const Float80 &rhs) const
{
    return ranged([&] { return fromSF(moira_extF80_div(toSF(*this), toSF(rhs))); });
}

Float80
Float80::sqrt() const
{
    return ranged([&] { return fromSF(moira_extF80_sqrt(toSF(*this))); });
}

Float80
Float80::rem(const Float80 &rhs) const
{
    return fromSF(moira_extF80_rem(toSF(*this), toSF(rhs)));
}

Float80
Float80::roundToInt() const
{
    return fromSF(moira_extF80_roundToInt(toSF(*this), roundingModeOf(), true));
}

Float80
Float80::scale(const Float80 &rhs) const
{
    // NaNs propagate (and signaling NaNs raise the invalid flag)
    if (isNaN() || rhs.isNaN()) return *this + rhs;

    // Scaling by an infinite factor is undefined
    if (rhs.isInfinite()) {
        FloatEnv::raiseFlags(FloatFlags::INVALID);
        return quietNaN();
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
        auto saved = FloatEnv::flags();
        n = rhs.toInt32Truncated();
        FloatEnv::clearFlags();
        FloatEnv::raiseFlags(saved);
    }

    // Normalize the mantissa. Denormals have the exponent of the smallest
    // normal number.
    i32 e = biasedExponent() ? biasedExponent() : 1;
    u64 m = man;
    int shift = std::countl_zero(m);
    m <<= shift;
    e -= shift;

    // With a reduced precision, the exponent range of the format applies
    if (limitsExponentRange()) {

        if (e + n < -100000) return roundToFormat(sign(), -100000, m, false);
        return roundToFormat(sign(), e + n, m, false);
    }

    // Let SoftFloat do the rounding and the exception handling
    return fromSF(moira_softfloat_roundPackToExtF80(sign(), e + n, m, 0, moira_sf_getPrecision()));
}


//
// Float80: Comparing
//

bool
Float80::operator==(const Float80 &rhs) const
{
    return moira_extF80_eq(toSF(*this), toSF(rhs));
}

bool
Float80::operator<(const Float80 &rhs) const
{
    return moira_extF80_lt(toSF(*this), toSF(rhs));
}

bool
Float80::operator<=(const Float80 &rhs) const
{
    return moira_extF80_le(toSF(*this), toSF(rhs));
}

}
