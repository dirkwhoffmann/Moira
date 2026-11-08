// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "MFloat80.h"

namespace MOIRA_NAMESPACE {

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
    static constexpr u8 SNAN        = 0x20;   ///< Signaling NaN operand
    static constexpr u8 ALL         = 0x3F;
};

/* The floating-point environment of the Motorola FPUs
 *
 * The environment consists of the rounding mode, the rounding precision and the
 * accumulated exception flags. All operations on MFloat80 numbers are carried
 * out by an environment object. They are computed in software and produce
 * bit-exact results that are independent of the host.
 *
 * Operands are interpreted by value, which includes the non-canonical
 * encodings the hardware accepts (denormals, unnormals, pseudo-denormals). A
 * biased exponent of 0 is a regular exponent (the 68k FPUs don't treat it as 1
 * like the x87 does).
 */
class FpuEnv {

    RoundingMode mode = RoundingMode::NEAREST;
    Precision prec = Precision::EXTENDED;

    /* If the precision is SINGLE or DOUBLE, the Motorola FPUs limit the exponent
     * range to the one of the corresponding format. FSGLMUL and FSGLDIV only
     * reduce the mantissa and keep the extended exponent range.
     */
    bool limitExponent = true;

    u8 exceptions = 0;

public:

    //
    // Configuration
    //

    // Rounding mode (default: NEAREST)
    RoundingMode roundingMode() const { return mode; }
    void setRoundingMode(RoundingMode value) { mode = value; }

    // Rounding precision (default: EXTENDED)
    Precision precision() const { return prec; }
    void setPrecision(Precision value) { prec = value; }

    // Exponent range of results (default: limited by the precision)
    bool limitsExponent() const { return limitExponent; }
    void setLimitsExponent(bool value) { limitExponent = value; }

    // Exception flags (see FloatFlags)
    u8 flags() const { return exceptions; }
    void clearFlags(u8 mask = FloatFlags::ALL) { exceptions &= u8(~mask); }
    void raiseFlags(u8 mask) { exceptions |= u8(mask & FloatFlags::ALL); }

    /* Saves the environment on construction and restores it on destruction.
     * The flags are not restored by default, so exceptions raised inside the
     * scope stay visible afterwards. Pass 'true' to discard them.
     */
    class Scope {

        FpuEnv &env;
        RoundingMode savedMode;
        Precision savedPrecision;
        bool savedLimits;
        u8 savedFlags;
        bool restoreFlags;

    public:

        explicit Scope(FpuEnv &env, bool restoreFlags = false);
        ~Scope();

        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;
    };


    //
    // Arithmetic
    //

    // Rounds to the current precision and exponent range (FMOVE)
    MFloat80 round(const MFloat80 &a);

    MFloat80 add(const MFloat80 &a, const MFloat80 &b);
    MFloat80 sub(const MFloat80 &a, const MFloat80 &b);
    MFloat80 mul(const MFloat80 &a, const MFloat80 &b);
    MFloat80 div(const MFloat80 &a, const MFloat80 &b);
    MFloat80 sqrt(const MFloat80 &a);

    // Raises the invalid flag and returns the default NaN of the 68k FPUs
    MFloat80 invalidOperation();

    // Rounds to an integral value according to the current rounding mode
    MFloat80 roundToInt(const MFloat80 &a);

    // Computes a * 2^n, where n is the integer part of b
    MFloat80 scale(const MFloat80 &a, const MFloat80 &b);

    /* Composes a number from an unrounded significand. The value is
     * sig:extra * 2^(exp - bias - 63), where 'exp' is the biased exponent that
     * applies if 'sig' is normalized (bit 63 set). A 'sig' that is not
     * normalized is normalized first. The bits in 'extra' lie below 'sig' and
     * are rounded away according to the environment. Raises the inexact,
     * underflow and overflow flags as needed.
     */
    MFloat80 roundPack(bool sign, i32 exp, u64 sig, u64 extra = 0);


    //
    // Comparing
    //

    /* The comparisons follow IEEE 754. A NaN is unordered, i.e., it compares
     * unequal to everything (including itself) and every ordering comparison
     * with it is false. Equality is quiet and raises the invalid flag only for
     * signaling NaNs. The ordering comparisons raise the invalid flag for all
     * NaNs. Note that +0 == -0.
     */
    bool eq(const MFloat80 &a, const MFloat80 &b);
    bool lt(const MFloat80 &a, const MFloat80 &b);
    bool le(const MFloat80 &a, const MFloat80 &b);


    //
    // Converting
    //

    // Converts from single and double precision (always exact)
    MFloat80 fromFloat(float value);
    MFloat80 fromDouble(double value);

    /* Conversion to integers. The result is rounded according to the current
     * rounding mode (toInt) or truncated toward zero (toIntTruncated).
     * Out-of-range values and NaNs raise the invalid flag and yield the most
     * negative integer.
     */
    i32 toInt32(const MFloat80 &a);
    i32 toInt32Truncated(const MFloat80 &a);
    i64 toInt64(const MFloat80 &a);
    i64 toInt64Truncated(const MFloat80 &a);

    // Conversion to single and double precision (rounded according to the current rounding mode)
    float toFloat(const MFloat80 &a);
    double toDouble(const MFloat80 &a);

private:

    MFloat80 propagateNaN(const MFloat80 &a, const MFloat80 &b);
    MFloat80 addSub(const MFloat80 &a, const MFloat80 &b, bool negate);

    i64 toInteger(const MFloat80 &a, RoundingMode mode, int bits);
    u64 toIeee(const MFloat80 &a, int bits);
};

}
