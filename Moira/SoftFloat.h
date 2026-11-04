// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "MoiraConfig.h"
#include "MoiraTypes.h"
#include "softfloat/softfloat_enums.h"
#include "softfloat/primitiveTypes.h"

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
    static constexpr u8 SNAN        = 0x20;   ///< Signaling NaN operand (no SoftFloat counterpart)
    static constexpr u8 ALL         = 0x3F;
};

/* SoftFloat is a C++ port of the extended-precision part of Berkeley SoftFloat
 * (Release 3e), which is used by Moira to carry out all floating-point
 * arithmetic in software. Results are bit-exact and independent of the host.
 *
 * The rounding mode, the rounding precision and the accumulated exception flags
 * form the floating-point environment. It is part of the object, which means
 * that every instance has its own environment. Different cores (or threads)
 * never interfere as long as they use different SoftFloat objects.
 */
class SoftFloat {

public:

    //
    // Environment
    //

    struct Env {

        uint_fast8_t roundingMode = softfloat_round_near_even;
        uint_fast8_t tininess = softfloat_tininess_afterRounding;
        uint_fast8_t flags = 0;         // Accumulated exception flags
        uint_fast8_t precision = 80;    // Rounding precision (32, 64, or 80 bits)

        /* The exponent range of results. If the precision is SINGLE or DOUBLE,
         * the Motorola FPUs limit the exponent range to the one of the
         * corresponding format. FSGLMUL and FSGLDIV only reduce the mantissa
         * and keep the extended exponent range.
         */
        bool limitExponent = true;
    };

    Env env;

    // The environment used by numbers that are not bound to a SoftFloat object
    static SoftFloat &standard();

    // Rounding mode (default: NEAREST)
    RoundingMode roundingMode() const;
    void setRoundingMode(RoundingMode mode);

    // Rounding precision (default: EXTENDED)
    Precision precision() const;
    void setPrecision(Precision precision);

    // Exponent range of results (default: limited by the precision)
    bool limitsExponent() const { return env.limitExponent; }
    void setLimitsExponent(bool value) { env.limitExponent = value; }

    // Exception flags (see FloatFlags)
    u8 flags() const { return (u8)env.flags; }
    void clearFlags(u8 mask = FloatFlags::ALL) { env.flags &= (uint_fast8_t)~mask; }
    void raiseFlags(u8 mask) { env.flags |= (uint_fast8_t)(mask & FloatFlags::ALL); }

    /* Saves the environment on construction and restores it on destruction.
     * The flags are not restored by default, so exceptions raised inside the
     * scope stay visible afterwards. Pass 'true' to discard them.
     */
    class Scope {

        SoftFloat &sf;
        RoundingMode savedMode;
        Precision savedPrecision;
        bool savedLimits;
        u8 savedFlags;
        bool restoreFlags;

    public:

        explicit Scope(SoftFloat &sf, bool restoreFlags = false);
        ~Scope();

        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;
    };


    //
    // Arithmetic
    //

    // a + b
    extFloat80_t add(extFloat80_t a, extFloat80_t b);

    // a - b
    extFloat80_t sub(extFloat80_t a, extFloat80_t b);

    // a * b
    extFloat80_t mul(extFloat80_t a, extFloat80_t b);

    // a / b
    extFloat80_t div(extFloat80_t a, extFloat80_t b);

    // IEEE remainder of a and b
    extFloat80_t rem(extFloat80_t a, extFloat80_t b);

    // Square root
    extFloat80_t sqrt(extFloat80_t a);

    // Rounds to an integral value (rounding mode given explicitly)
    extFloat80_t roundToInt(extFloat80_t a, uint_fast8_t roundingMode, bool exact);


    //
    // Comparing
    //

    // a == b (quiet)
    bool eq(extFloat80_t a, extFloat80_t b);

    // a <= b (signaling)
    bool le(extFloat80_t a, extFloat80_t b);

    // a < b (signaling)
    bool lt(extFloat80_t a, extFloat80_t b);

    // Checks for a signaling NaN
    bool isSignalingNaN(extFloat80_t a);


    //
    // Converting
    //

    // Converts from a 32-bit integer
    extFloat80_t fromI32(int32_t a);

    // Converts from a 64-bit integer
    extFloat80_t fromI64(int64_t a);

    // Converts from single precision
    extFloat80_t fromF32(float32_t a);

    // Converts from double precision
    extFloat80_t fromF64(float64_t a);

    // Converts to a 32-bit integer
    int_fast32_t toI32(extFloat80_t a, uint_fast8_t roundingMode, bool exact);

    // Converts to a 64-bit integer
    int_fast64_t toI64(extFloat80_t a, uint_fast8_t roundingMode, bool exact);

    // Converts to single precision
    float32_t toF32(extFloat80_t a);

    // Converts to double precision
    float64_t toF64(extFloat80_t a);


private:

    // Berkeley SoftFloat internals

    extFloat80_t softfloat_addMagsExtF80(uint_fast16_t uiA64, uint_fast64_t uiA0, uint_fast16_t uiB64, uint_fast64_t uiB0, bool signZ);
    void softfloat_extF80UIToCommonNaN(uint_fast16_t uiA64, uint_fast64_t uiA0, struct commonNaN *zPtr);
    void softfloat_f32UIToCommonNaN(uint_fast32_t uiA, struct commonNaN *zPtr);
    void softfloat_f64UIToCommonNaN(uint_fast64_t uiA, struct commonNaN *zPtr);
    extFloat80_t softfloat_normRoundPackToExtF80(bool sign, int_fast32_t exp, uint_fast64_t sig, uint_fast64_t sigExtra, uint_fast8_t roundingPrecision);
    struct uint128 softfloat_propagateNaNExtF80UI(uint_fast16_t uiA64, uint_fast64_t uiA0, uint_fast16_t uiB64, uint_fast64_t uiB0);
    extFloat80_t softfloat_roundPackToExtF80(bool sign, int_fast32_t exp, uint_fast64_t sig, uint_fast64_t sigExtra, uint_fast8_t roundingPrecision);
    float32_t softfloat_roundPackToF32(bool sign, int_fast16_t exp, uint_fast32_t sig);
    float64_t softfloat_roundPackToF64(bool sign, int_fast16_t exp, uint_fast64_t sig);
    int_fast32_t softfloat_roundToI32(bool sign, uint_fast64_t sig, uint_fast8_t roundingMode, bool exact);
    int_fast64_t softfloat_roundToI64(bool sign, uint_fast64_t sig, uint_fast64_t sigExtra, uint_fast8_t roundingMode, bool exact);
    extFloat80_t softfloat_subMagsExtF80(uint_fast16_t uiA64, uint_fast64_t uiA0, uint_fast16_t uiB64, uint_fast64_t uiB0, bool signZ);
    void softfloat_raiseFlags(uint_fast8_t flags);
};

}
