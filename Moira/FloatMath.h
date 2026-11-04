// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "Float80.h"

namespace MOIRA_NAMESPACE {

/* Transcendental functions on Float80
 *
 * The functions are evaluated with a 128-bit significand and rounded once to
 * the current precision and rounding mode of the floating-point environment
 * of the operand (see SoftFloat). Hence, results are accurate to far better than one unit in the
 * last place (but, like on the real hardware, not necessarily correctly
 * rounded). Results that are mathematically exact (e.g., 2^n, log2(2^n),
 * sin(0)) are returned exactly.
 *
 * Special operands are handled according to the MC68881 user's manual:
 * Invalid operations (e.g., the logarithm of a negative number) raise the
 * invalid flag and return a quiet NaN, a division by zero (e.g., ln(0)) raises
 * the corresponding flag and returns an infinity. NaN operands have to be
 * resolved by the caller.
 */
struct FloatMath {

    // Trigonometric functions
    static Float80 sin(const Float80 &x);
    static Float80 cos(const Float80 &x);
    static Float80 tan(const Float80 &x);
    static void sincos(const Float80 &x, Float80 &sine, Float80 &cosine);

    // Inverse trigonometric functions
    static Float80 asin(const Float80 &x);
    static Float80 acos(const Float80 &x);
    static Float80 atan(const Float80 &x);

    // Hyperbolic functions
    static Float80 sinh(const Float80 &x);
    static Float80 cosh(const Float80 &x);
    static Float80 tanh(const Float80 &x);
    static Float80 atanh(const Float80 &x);

    // Exponential functions
    static Float80 etox(const Float80 &x);         // e^x
    static Float80 etoxm1(const Float80 &x);       // e^x - 1
    static Float80 twotox(const Float80 &x);       // 2^x
    static Float80 tentox(const Float80 &x);       // 10^x

    // Logarithms
    static Float80 logn(const Float80 &x);         // ln(x)
    static Float80 lognp1(const Float80 &x);       // ln(x + 1)
    static Float80 log2(const Float80 &x);
    static Float80 log10(const Float80 &x);
};

}
