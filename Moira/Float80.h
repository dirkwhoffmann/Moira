// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "MoiraTypes.h"

namespace MOIRA_NAMESPACE {

/* Float80 is an 80-bit extended precision floating-point number, i.e., the
 * x87 / Motorola 68881 "extended" format:
 *
 *   bit 79      : sign
 *   bits 64..78 : biased exponent (bias 0x3FFF)
 *   bits 0..63  : mantissa, including an explicit integer bit (bit 63)
 *
 * All arithmetic is carried out in software (Berkeley SoftFloat), so results
 * are bit-exact and identical on all platforms, independent of the width of
 * the host's 'long double'.
 *
 * The class is a plain value type. It stores the raw bits of the number and
 * makes no attempt to normalize them. In particular, it can represent the
 * non-canonical encodings (unnormals, pseudo-denormals) that the hardware
 * accepts as operands.
 *
 * The result of an operation is controlled by the floating-point environment
 * (FloatEnv): the rounding mode and the rounding precision are read from it,
 * and the IEEE exception flags raised by the operation are accumulated in it.
 * The environment is thread-local, which means that independent computations
 * on different threads don't interfere.
 */

//
// Floating-point environment
//

enum class RoundingMode : int {

    NEAREST = 0,    ///< Round to nearest, ties to even
    ZERO    = 1,    ///< Round toward zero
    DOWN    = 2,    ///< Round toward negative infinity
    UP      = 3     ///< Round toward positive infinity
};

/* The rounding precision limits the number of mantissa bits of a result. The
 * exponent range is always the one of the extended format.
 */
enum class Precision : int {

    SINGLE   = 32,  ///< 24 mantissa bits
    DOUBLE   = 64,  ///< 53 mantissa bits
    EXTENDED = 80   ///< 64 mantissa bits
};

// Exception flags. They are sticky: Once raised, a flag stays set until cleared.
struct FloatFlags {

    static constexpr u8 INEXACT     = 0x01;
    static constexpr u8 UNFL        = 0x02;   ///< Underflow
    static constexpr u8 OVFL        = 0x04;   ///< Overflow
    static constexpr u8 DIV_BY_ZERO = 0x08;
    static constexpr u8 INVALID     = 0x10;
    static constexpr u8 SNAN        = 0x20;   ///< Signaling NaN operand (no SoftFloat counterpart)
    static constexpr u8 ALL         = 0x3F;
};

class FloatEnv {

public:

    // Rounding mode (default: NEAREST)
    static RoundingMode roundingMode();
    static void setRoundingMode(RoundingMode mode);

    // Rounding precision (default: EXTENDED)
    static Precision precision();
    static void setPrecision(Precision precision);

    /* The exponent range of results. If the precision is SINGLE or DOUBLE, the
     * Motorola FPUs limit the exponent range to the one of the corresponding
     * format (default: true). FSGLMUL and FSGLDIV only reduce the mantissa
     * and keep the extended exponent range.
     */
    static bool limitsExponent();
    static void setLimitsExponent(bool value);

    // Exception flags (see FloatFlags)
    static u8 flags();
    static void clearFlags(u8 mask = FloatFlags::ALL);
    static void raiseFlags(u8 mask);

    /* Saves the environment on construction and restores it on destruction.
     * The flags are not restored by default, so exceptions raised inside the
     * scope stay visible afterwards. Pass 'true' to discard them.
     */
    class Scope {

        RoundingMode savedMode;
        Precision savedPrecision;
        bool savedLimits;
        u8 savedFlags;
        bool restoreFlags;

    public:

        explicit Scope(bool restoreFlags = false);
        ~Scope();

        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;
    };
};


//
// Float80
//

class Float80 {

    // Sign and biased exponent
    u16 se = 0;

    // Mantissa (including the integer bit)
    u64 man = 0;

public:

    //
    // Creating numbers
    //

    // Creates +0
    constexpr Float80() = default;

    // Creates a number from its raw bits
    constexpr Float80(u16 signExp, u64 mantissa) : se(signExp), man(mantissa) { }
    static constexpr Float80 fromBits(u16 signExp, u64 mantissa) { return { signExp, mantissa }; }

    // Converts from other number formats
    static Float80 fromInt(i64 value);
    static Float80 fromFloat(float value);
    static Float80 fromDouble(double value);
    static Float80 fromLongDouble(long double value);

    /* Composes a number from an unrounded significand. The value is
     * sig:extra * 2^(exp - bias - 63), where 'exp' is the biased exponent that
     * applies if 'sig' is normalized (bit 63 set). A 'sig' that is not
     * normalized is normalized first. The bits in 'extra' lie below 'sig' and
     * are rounded away according to the current floating-point environment
     * (rounding mode and precision). Raises the inexact, underflow and
     * overflow flags as needed.
     */
    static Float80 roundPack(bool sign, i32 exp, u64 sig, u64 extra = 0);

    // Rounds to the current precision and exponent range (FMOVE, FABS, FNEG)
    Float80 rounded() const;

    // Special values
    static constexpr Float80 zero(bool negative = false) { return { u16(negative ? 0x8000 : 0), 0 }; }
    static constexpr Float80 one(bool negative = false) { return { u16(negative ? 0xBFFF : 0x3FFF), 1ULL << 63 }; }
    static constexpr Float80 infinity(bool negative = false) { return { u16(negative ? 0xFFFF : 0x7FFF), 0 }; }
    static constexpr Float80 quietNaN(bool negative = false) { return { u16(negative ? 0xFFFF : 0x7FFF), 0xC000000000000000ULL }; }
    static constexpr Float80 signalingNaN(bool negative = false) { return { u16(negative ? 0xFFFF : 0x7FFF), 0xA000000000000000ULL }; }


    //
    // Accessing the raw bits
    //

    constexpr u16 signExp() const { return se; }
    constexpr u64 mantissa() const { return man; }

    constexpr bool sign() const { return se & 0x8000; }
    constexpr u16 biasedExponent() const { return se & 0x7FFF; }
    constexpr i32 exponent() const { return i32(biasedExponent()) - bias; }
    constexpr bool integerBit() const { return man >> 63; }

    // Exponent bias
    static constexpr i32 bias = 0x3FFF;


    //
    // Classifying numbers
    //

    constexpr bool isNegative() const { return sign(); }
    constexpr bool isZero() const { return biasedExponent() == 0 && man == 0; }
    constexpr bool isInfinite() const { return biasedExponent() == 0x7FFF && (man << 1) == 0; }
    constexpr bool isNaN() const { return biasedExponent() == 0x7FFF && (man << 1) != 0; }
    constexpr bool isSignalingNaN() const { return isNaN() && !(man & (1ULL << 62)); }
    constexpr bool isQuietNaN() const { return isNaN() && (man & (1ULL << 62)); }
    constexpr bool isFinite() const { return biasedExponent() != 0x7FFF; }

    // A number with a minimal exponent and a cleared integer bit
    constexpr bool isDenormal() const { return biasedExponent() == 0 && man != 0 && !integerBit(); }

    // A number with a regular exponent and a set integer bit
    constexpr bool isNormal() const { return biasedExponent() != 0 && biasedExponent() != 0x7FFF && integerBit(); }


    //
    // Converting to other number formats
    //

    // Conversion to integers. The result is rounded according to the current
    // rounding mode (toInt) or truncated toward zero (toIntTruncated).
    // Out-of-range values raise the invalid flag and saturate.
    i32 toInt32() const;
    i32 toInt32Truncated() const;
    i64 toInt64() const;
    i64 toInt64Truncated() const;

    // Conversion to floating-point formats (rounded according to the current rounding mode)
    float toFloat() const;
    double toDouble() const;

    // Converts to 'long double'. The conversion is exact if the host's long
    // double is an 80-bit extended type and lossy otherwise.
    long double toLongDouble() const;

    // Returns a string of the form "0x3FFF:8000000000000000"
    std::string str() const;


    //
    // Arithmetic
    //

    Float80 operator-() const;
    Float80 operator+(const Float80 &rhs) const;
    Float80 operator-(const Float80 &rhs) const;
    Float80 operator*(const Float80 &rhs) const;
    Float80 operator/(const Float80 &rhs) const;

    Float80 &operator+=(const Float80 &rhs) { return *this = *this + rhs; }
    Float80 &operator-=(const Float80 &rhs) { return *this = *this - rhs; }
    Float80 &operator*=(const Float80 &rhs) { return *this = *this * rhs; }
    Float80 &operator/=(const Float80 &rhs) { return *this = *this / rhs; }

    // Absolute value (never raises exceptions)
    Float80 abs() const;

    // Square root
    Float80 sqrt() const;

    // IEEE remainder: this - n * rhs, where n is the integer nearest to this / rhs
    Float80 rem(const Float80 &rhs) const;

    // Rounds to an integral value according to the current rounding mode
    Float80 roundToInt() const;

    // Computes this * 2^n, where n is the integer part of rhs
    Float80 scale(const Float80 &rhs) const;


    //
    // Comparing
    //

    /* The comparison operators follow IEEE 754. A NaN is unordered, i.e., it
     * compares unequal to everything (including itself) and every ordering
     * comparison with it is false. Equality is quiet and raises the invalid
     * flag only for signaling NaNs. The ordering comparisons raise the invalid
     * flag for all NaNs. Note that +0 == -0.
     */
    bool operator==(const Float80 &rhs) const;
    bool operator!=(const Float80 &rhs) const { return !(*this == rhs); }
    bool operator<(const Float80 &rhs) const;
    bool operator<=(const Float80 &rhs) const;
    bool operator>(const Float80 &rhs) const { return rhs < *this; }
    bool operator>=(const Float80 &rhs) const { return rhs <= *this; }

    // Checks whether at least one operand is a NaN
    bool unordered(const Float80 &rhs) const { return isNaN() || rhs.isNaN(); }

    // Checks for identical bit patterns (in contrast to operator==, 0 and -0 differ)
    constexpr bool identical(const Float80 &rhs) const { return se == rhs.se && man == rhs.man; }
};

}
