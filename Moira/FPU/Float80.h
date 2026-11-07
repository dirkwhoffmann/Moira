// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "../MoiraTypes.h"
#include "SoftFloat.h"

namespace MOIRA_NAMESPACE {

/* Float80 implements the Motorola 68881 80-bit extended precision format:
 *
 *   Bit 79      : Sign bit
 *   Bits 64..78 : Biased exponent (bias 0x3FFF)
 *   Bits 0..63  : Mantissa, including an explicit integer bit (bit 63)
 *
 * All arithmetic is carried out in software (Berkeley SoftFloat), so results
 * are bit-exact and identical on all platforms.
 *
 * The class is a plain value type. It stores the raw bits of the number and
 * makes no attempt to normalize them. In particular, it can represent the
 * non-canonical encodings (unnormals, pseudo-denormals) that the hardware
 * accepts as operands.
 *
 * The result of an operation is controlled by a floating-point environment
 * (see SoftFloat): the rounding mode and the rounding precision are read from
 * it, and the IEEE exception flags raised by the operation are accumulated in
 * it. A number can be bound to a SoftFloat object, which is then used by all
 * operations on the number, and passed on to the results. Operations on numbers
 * that are not bound to any object use a thread-local standard environment
 * (SoftFloat::standard()). If the operands of a binary operation are bound to
 * different objects, the environment of the left operand takes precedence.
 */

class Float80 {

    // Sign and biased exponent
    u16 se = 0;

    // Mantissa (including the integer bit)
    u64 man = 0;

    // Floating-point environment (nullptr if the number is not bound)
    SoftFloat *envp = nullptr;

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
    static Float80 fromInt(i64 value, SoftFloat &sf = SoftFloat::standard());
    static Float80 fromFloat(float value, SoftFloat &sf = SoftFloat::standard());
    static Float80 fromDouble(double value, SoftFloat &sf = SoftFloat::standard());
    static Float80 fromLongDouble(long double value);

    /* Binds the number to a floating-point environment. The first variant
     * returns a bound copy. The second variant binds the number itself.
     */
    Float80 in(SoftFloat &sf) const;
    void bind(SoftFloat &sf) { envp = (&sf == &SoftFloat::standard()) ? nullptr : &sf; }
    bool isBound() const { return envp != nullptr; }

    /* The environment is a property of the storage location rather than of the
     * value. Assigning an unbound number to a bound one keeps the environment of
     * the destination. This way, a register that is bound to an FPU stays bound,
     * regardless of where the assigned value came from.
     */
    constexpr Float80(const Float80 &) = default;
    constexpr Float80 &operator=(const Float80 &rhs)
    {
        se = rhs.se; man = rhs.man;
        if (rhs.envp) envp = rhs.envp;
        return *this;
    }

    // Returns the environment used by operations on this number
    SoftFloat &env() const { return envp ? *envp : SoftFloat::standard(); }

    /* Composes a number from an unrounded significand. The value is
     * sig:extra * 2^(exp - bias - 63), where 'exp' is the biased exponent that
     * applies if 'sig' is normalized (bit 63 set). A 'sig' that is not
     * normalized is normalized first. The bits in 'extra' lie below 'sig' and
     * are rounded away according to the current floating-point environment
     * (rounding mode and precision). Raises the inexact, underflow and
     * overflow flags as needed.
     */
    static Float80 roundPack(bool sign, i32 exp, u64 sig, u64 extra = 0, SoftFloat &sf = SoftFloat::standard());

    // Rounds to the current precision and exponent range (FMOVE, FABS, FNEG)
    Float80 rounded() const;

    // Special values
    static constexpr Float80 zero(bool negative = false) { return { u16(negative ? 0x8000 : 0), 0 }; }
    static constexpr Float80 one(bool negative = false) { return { u16(negative ? 0xBFFF : 0x3FFF), 1ULL << 63 }; }
    static constexpr Float80 infinity(bool negative = false) { return { u16(negative ? 0xFFFF : 0x7FFF), 0 }; }
    static constexpr Float80 quietNaN(bool negative = false) { return { u16(negative ? 0xFFFF : 0x7FFF), 0xC000000000000000ULL }; }
    static constexpr Float80 signalingNaN(bool negative = false) { return { u16(negative ? 0xFFFF : 0x7FFF), 0xA000000000000000ULL }; }


    //
    // Accessing raw bits
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
