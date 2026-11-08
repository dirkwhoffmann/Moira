// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "FpuEnv.h"

namespace MOIRA_NAMESPACE {

/* Transcendental functions on MFloat80
 *
 * The functions are evaluated with a 128-bit significand and rounded once to
 * the current precision and rounding mode of the floating-point environment
 * of the operand (see FpuEnv). Hence, results are accurate to far better than one unit in the
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
    static MFloat80 sin(FpuEnv &env, const MFloat80 &x);
    static MFloat80 cos(FpuEnv &env, const MFloat80 &x);
    static MFloat80 tan(FpuEnv &env, const MFloat80 &x);
    static void sincos(FpuEnv &env, const MFloat80 &x, MFloat80 &sine, MFloat80 &cosine);

    // Inverse trigonometric functions
    static MFloat80 asin(FpuEnv &env, const MFloat80 &x);
    static MFloat80 acos(FpuEnv &env, const MFloat80 &x);
    static MFloat80 atan(FpuEnv &env, const MFloat80 &x);

    // Hyperbolic functions
    static MFloat80 sinh(FpuEnv &env, const MFloat80 &x);
    static MFloat80 cosh(FpuEnv &env, const MFloat80 &x);
    static MFloat80 tanh(FpuEnv &env, const MFloat80 &x);
    static MFloat80 atanh(FpuEnv &env, const MFloat80 &x);

    // Exponential functions
    static MFloat80 etox(FpuEnv &env, const MFloat80 &x);         // e^x
    static MFloat80 etoxm1(FpuEnv &env, const MFloat80 &x);       // e^x - 1
    static MFloat80 twotox(FpuEnv &env, const MFloat80 &x);       // 2^x
    static MFloat80 tentox(FpuEnv &env, const MFloat80 &x);       // 10^x

    // Logarithms
    static MFloat80 logn(FpuEnv &env, const MFloat80 &x);         // ln(x)
    static MFloat80 lognp1(FpuEnv &env, const MFloat80 &x);       // ln(x + 1)
    static MFloat80 log2(FpuEnv &env, const MFloat80 &x);
    static MFloat80 log10(FpuEnv &env, const MFloat80 &x);
};

}
