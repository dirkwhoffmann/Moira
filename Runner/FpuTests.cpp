// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "MoiraConfig.h"
#include "FPU.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace moira;

// The floating-point environment of numbers that are not bound to an FPU
static SoftFloat &sf = SoftFloat::standard();

static int failures = 0;
static int checks = 0;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
    printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static bool same(const Float80 &a, const Float80 &b) { return a.identical(b); }

static void testIntegers()
{
    SoftFloat::Scope scope(sf);
    sf.clearFlags();

    CHECK(FpuFormats::toInteger(Float80::fromInt(42), 8) == 42);
    CHECK(sf.flags() == 0);
    CHECK(FpuFormats::toInteger(Float80::fromInt(-129), 8) == -128);
    CHECK(sf.flags() & FloatFlags::INVALID);
    sf.clearFlags();
    CHECK(FpuFormats::toInteger(Float80::fromInt(70000), 16) == 32767);
    CHECK(FpuFormats::toInteger(Float80::fromInt(-3000000000LL), 32) == INT32_MIN);
    sf.clearFlags();

    // Rounding modes
    auto x = Float80::fromDouble(2.5);
    sf.setRoundingMode(RoundingMode::NEAREST);
    CHECK(FpuFormats::toInteger(x, 32) == 2);
    sf.setRoundingMode(RoundingMode::UP);
    CHECK(FpuFormats::toInteger(x, 32) == 3);
    sf.setRoundingMode(RoundingMode::DOWN);
    CHECK(FpuFormats::toInteger(Float80::fromDouble(-2.5), 32) == -3);
    sf.setRoundingMode(RoundingMode::ZERO);
    CHECK(FpuFormats::toInteger(Float80::fromDouble(-2.5), 32) == -2);
    CHECK(sf.flags() & FloatFlags::INEXACT);
    sf.clearFlags();

    // Special values
    CHECK(FpuFormats::toInteger(Float80::infinity(true), 32) == INT32_MIN);
    CHECK(FpuFormats::toInteger(Float80::quietNaN(), 32) == i32(0xC0000000));      // High bits of the mantissa
    CHECK(sf.flags() & FloatFlags::INVALID);
}

static void testFloats()
{
    SoftFloat::Scope scope(sf);
    sf.clearFlags();

    CHECK(FpuFormats::toSingle(Float80::one()) == 0x3F800000);
    CHECK(FpuFormats::toDouble(Float80::one()) == 0x3FF0000000000000ULL);
    CHECK(same(FpuFormats::fromSingle(0x3F800000), Float80::one()));
    CHECK(same(FpuFormats::fromDouble(0xBFF0000000000000ULL), Float80::one(true)));
    CHECK(same(FpuFormats::fromSingle(0x7F800000), Float80::infinity()));
    CHECK(sf.flags() == 0);

    // Rounding of 1/3 to single and double
    auto third = Float80::one() / Float80::fromInt(3);
    CHECK(FpuFormats::toDouble(third) == 0x3FD5555555555555ULL);
    CHECK(FpuFormats::toSingle(third) == 0x3EAAAAAB);
    CHECK(sf.flags() & FloatFlags::INEXACT);
    sf.clearFlags();

    // Extended precision in memory
    u32 w[3];
    FpuFormats::toExtended(Float80::fromBits(0xC001, 0xC000000000000000ULL), w);
    CHECK(w[0] == 0xC0010000 && w[1] == 0xC0000000 && w[2] == 0);
    CHECK(same(FpuFormats::fromExtended(w), Float80::fromBits(0xC001, 0xC000000000000000ULL)));
}

static void testPackedIn()
{
    SoftFloat::Scope scope(sf);
    sf.clearFlags();

    // 1.0E0
    u32 one[3] = { 0x00000001, 0, 0 };
    CHECK(same(FpuFormats::fromPacked(one), Float80::one()));

    // -1.5E2
    u32 m150[3] = { 0x80020001, 0x50000000, 0 };
    CHECK(same(FpuFormats::fromPacked(m150), Float80::fromInt(-150)));
    CHECK(sf.flags() == 0);

    // 1.0E-5 is not exactly representable
    u32 tiny[3] = { 0x40050001, 0, 0 };
    auto r = FpuFormats::fromPacked(tiny);
    CHECK(r.toDouble() == 1e-5);
    CHECK(sf.flags() & FloatFlags::INEXACT);
    sf.clearFlags();

    // 1.0E4000 and 1.0E-4000 (the exponent has four digits)
    u32 big[3] = { 0x00004001 | (4u << 12), 0, 0 };
    big[0] = 0x00000001 | (4u << 12);
    CHECK(r.isFinite());
    CHECK(FpuFormats::fromPacked(big).biasedExponent() == 0x3FFF + 13287);

    // Zero, infinity, NaN
    u32 zero[3] = { 0x80000000, 0, 0 };
    CHECK(same(FpuFormats::fromPacked(zero), Float80::zero(true)));
    u32 inf[3] = { 0x7FFF0000, 0, 0 };
    CHECK(same(FpuFormats::fromPacked(inf), Float80::infinity()));
    u32 nan[3] = { 0x7FFF0000, 0xC0000000, 0 };
    CHECK(FpuFormats::fromPacked(nan).isNaN());

    // Overflow and underflow
    u32 huge[3] = { 0x00005001 | 0, 0, 0 };
    huge[0] = 0x00000001 | (9u << 12) | (9u << 24) | (9u << 20) | (9u << 16);
    CHECK(FpuFormats::fromPacked(huge).isInfinite());
    CHECK(sf.flags() & FloatFlags::OVFL);
}

static void testPackedOut()
{
    SoftFloat::Scope scope(sf);
    sf.clearFlags();
    u32 w[3];

    FpuFormats::toPacked(Float80::one(), 17, w);
    CHECK(w[0] == 0x00000001 && w[1] == 0 && w[2] == 0);
    CHECK(sf.flags() == 0);

    FpuFormats::toPacked(Float80::fromInt(-150), 17, w);
    CHECK(w[0] == 0x80020001 && w[1] == 0x50000000 && w[2] == 0);

    // 0.5 = 5.0E-1
    FpuFormats::toPacked(Float80::fromDouble(0.5), 17, w);
    CHECK(w[0] == 0x40010005 && w[1] == 0 && w[2] == 0);

    // 1/3 with 17 digits: 3.3333333333333333E-1 (inexact)
    FpuFormats::toPacked(Float80::one() / Float80::fromInt(3), 17, w);
    CHECK(w[0] == 0x40010003 && w[1] == 0x33333333 && w[2] == 0x33333333);
    CHECK(sf.flags() & FloatFlags::INEXACT);
    sf.clearFlags();

    // k-factor: three significant digits of 2/3 -> 6.67E-1
    FpuFormats::toPacked(Float80::fromInt(2) / Float80::fromInt(3), 3, w);
    CHECK(w[0] == 0x40010006 && w[1] == 0x67000000 && w[2] == 0);
    sf.setRoundingMode(RoundingMode::ZERO);
    FpuFormats::toPacked(Float80::fromInt(2) / Float80::fromInt(3), 3, w);
    CHECK(w[1] == 0x66000000);
    sf.setRoundingMode(RoundingMode::NEAREST);
    sf.clearFlags();

    // Negative k-factor: two digits right of the decimal point (123.456 -> 123.46)
    FpuFormats::toPacked(Float80::fromDouble(123.456), -2, w);
    CHECK(w[0] == 0x00020001 && w[1] == 0x23460000);
    sf.clearFlags();

    // Rounding carries into a new digit: 9.99 with k = 2 -> 1.0E1
    FpuFormats::toPacked(Float80::fromDouble(9.99), 2, w);
    CHECK(w[0] == 0x00010001 && w[1] == 0);

    // k > 17 raises the invalid flag
    sf.clearFlags();
    FpuFormats::toPacked(Float80::one(), 18, w);
    CHECK(sf.flags() & FloatFlags::INVALID);
    sf.clearFlags();

    // Special values
    FpuFormats::toPacked(Float80::zero(true), 17, w);
    CHECK(w[0] == 0x80000000 && w[1] == 0 && w[2] == 0);
    FpuFormats::toPacked(Float80::infinity(true), 17, w);
    CHECK(w[0] == 0xFFFF0000 && w[1] == 0 && w[2] == 0);

    // Large exponents have four digits
    FpuFormats::toPacked(Float80::fromBits(0x3FFF + 13300, 1ULL << 63), 17, w);
    CHECK((w[0] >> 12 & 0xF) == 4 && (w[0] >> 16 & 0xFFF) == 0x003);
}

static void testPackedRoundTrip()
{
    SoftFloat::Scope scope(sf);
    double values[] = { 1.0, 0.1, 123456.789, 1e300, 1e-300, 4.9406564584124654e-324,
                        2.2250738585072014e-308, 1.7976931348623157e308, 3.141592653589793,
                        -2.718281828459045, 1e22, 1e23, 9007199254740993.0 };

    for (double d : values) {

        u32 w[3];
        sf.clearFlags();
        FpuFormats::toPacked(Float80::fromDouble(d), 17, w);
        auto back = FpuFormats::fromPacked(w);

        if (back.toDouble() != d) printf("round trip failed for %.17g (got %.17g)\n", d, back.toDouble());
        CHECK(back.toDouble() == d);
    }
}

static Float80 F(double d) { return Float80::fromDouble(d); }

static void testArithmetic()
{
    FPU fpu;
    FPU::Scope scope(fpu);

    // Basic operations
    CHECK(same(fpu.fadd(F(1.5), F(2.25)), F(3.75)));
    CHECK(same(fpu.fsub(F(1.5), F(2.25)), F(-0.75)));
    CHECK(same(fpu.fmul(F(1.5), F(-4)), F(-6)));
    CHECK(same(fpu.fdiv(F(9), F(4)), F(2.25)));
    CHECK(same(fpu.fsqrt(F(2.25)), F(1.5)));
    CHECK(same(fpu.fabs(F(-3)), F(3)));
    CHECK(same(fpu.fneg(F(3)), F(-3)));
    CHECK(same(fpu.fscal(F(3), F(4.9)), F(48)));
    CHECK(same(fpu.fscal(F(48), F(-4)), F(3)));
    CHECK(fpu.fpsr == 0);

    // Exceptions
    fpu.fdiv(F(1), F(0));
    fpu.collectFlags();
    CHECK(fpu.fpsr & FPEXP::DZ);
    fpu.setFPSR(0);
    fpu.fdiv(F(1), F(3));
    fpu.collectFlags();
    CHECK(fpu.fpsr & FPEXP::INEX2);
    fpu.setFPSR(0);

    // Invalid operations produce the positive default NaN
    auto defNaN = FPU::defaultNaN();
    CHECK(same(fpu.fmul(F(0), Float80::infinity()), defNaN));
    fpu.collectFlags();
    CHECK(fpu.fpsr & FPEXP::OPERR);
    fpu.setFPSR(0);
    CHECK(same(fpu.fsub(Float80::infinity(), Float80::infinity()), defNaN));
    CHECK(same(fpu.fdiv(F(0), F(0)), defNaN));
    CHECK(same(fpu.fsqrt(F(-1)), defNaN));
    CHECK(same(fpu.fgetexp(Float80::infinity()), defNaN));
    CHECK(same(fpu.fscal(F(1), Float80::infinity()), defNaN));
    CHECK(same(fpu.fmod(Float80::infinity(), F(3)), defNaN));
    CHECK(same(fpu.frem(F(3), F(0)), defNaN));
    fpu.setFPSR(0);

    // NaN operands
    auto qnan = Float80::fromBits(0x7FFF, 0xC000000000000123ULL);
    auto qnan2 = Float80::fromBits(0xFFFF, 0xC000000000000999ULL);
    auto snan = Float80::fromBits(0x7FFF, 0x8000000000000123ULL);
    CHECK(same(fpu.fadd(qnan, F(1)), qnan));
    CHECK(same(fpu.fadd(F(1), qnan), qnan));
    CHECK(same(fpu.fadd(qnan, qnan2), qnan));
    CHECK(same(fpu.fadd(qnan2, qnan), qnan2));
    CHECK(fpu.fpsr == 0);
    CHECK(same(fpu.fadd(F(1), snan), qnan));
    CHECK(fpu.fpsr & FPEXP::SNAN);
    CHECK(!(fpu.fpsr & FPEXP::OPERR));
    fpu.setFPSR(0);
    CHECK(same(fpu.fadd(qnan2, snan), qnan2));
    CHECK(fpu.fpsr & FPEXP::SNAN);
    fpu.setFPSR(0);
    CHECK(same(fpu.fsqrt(snan), qnan));
    CHECK(same(fpu.fint(snan), qnan));
    CHECK(fpu.fpsr & FPEXP::SNAN);
    fpu.setFPSR(0);

    // FINT and FINTRZ
    CHECK(same(fpu.fint(F(2.5)), F(2)));
    CHECK(same(fpu.fint(F(3.5)), F(4)));
    CHECK(same(fpu.fint(F(-0.2)), Float80::zero(true)));
    CHECK(same(fpu.fintrz(F(-2.7)), F(-2)));
    CHECK(same(fpu.fintrz(F(2.7)), F(2)));
    CHECK(same(fpu.fint(Float80::infinity(true)), Float80::infinity(true)));
    CHECK(sf.roundingMode() == RoundingMode::NEAREST);
    fpu.setFPSR(0);

    // FGETEXP and FGETMAN
    CHECK(same(fpu.fgetexp(F(8)), F(3)));
    CHECK(same(fpu.fgetexp(F(0.1)), F(-4)));
    CHECK(same(fpu.fgetexp(F(-1)), F(0)));
    CHECK(same(fpu.fgetexp(Float80::fromBits(0x0000, 1)), Float80::fromInt(-16383 - 63)));
    CHECK(same(fpu.fgetexp(Float80::zero(true)), Float80::zero(true)));

    // Denormals: The biased exponent 0 is a regular exponent, i.e., 0:8000... is 2^-16383
    CHECK(same(fpu.fgetexp(Float80::fromBits(0x0000, 1ULL << 63)), Float80::fromInt(-16383)));
    {
        FPU::Scope scope(fpu);
        fpu.setFPSR(0);
        auto half = Float80::fromBits(0x3FFE, 1ULL << 63);
        auto minNormal = Float80::fromBits(0x0001, 1ULL << 63);

        // 2^-16382 * 2^-1 is a regular result (no underflow), 2^-16382 * 2^-2 is not
        CHECK(same(minNormal * half, Float80::fromBits(0x0000, 1ULL << 63)));
        CHECK((fpu.fpsr & FPEXP::UNFL) == 0);
        CHECK(same(minNormal * Float80::fromBits(0x3FFD, 1ULL << 63), Float80::fromBits(0x0000, 1ULL << 62)));
        CHECK(sf.flags() & FloatFlags::UNFL);
        sf.clearFlags();

        // Denormal operands are scaled by the 68k convention
        CHECK(same(Float80::fromBits(0x0000, 5) + Float80::fromBits(0x0000, 6), Float80::fromBits(0x0000, 11)));
        CHECK(same(Float80::fromBits(0x0000, 1ULL << 63) + Float80::fromBits(0x0000, 1ULL << 63), minNormal));
        CHECK(same(Float80::fromBits(0x0000, 5) / Float80::fromBits(0x3FFE, 1ULL << 63), Float80::fromBits(0x0000, 10)));
        sf.clearFlags();
    }
    CHECK(same(fpu.fgetman(F(10)), F(1.25)));
    CHECK(same(fpu.fgetman(F(-0.1)), Float80::fromBits(0xBFFF, Float80::fromDouble(0.1).mantissa())));
    CHECK(same(fpu.fgetman(Float80::fromBits(0x0000, 1)), Float80::one()));
    CHECK(fpu.fpsr == 0);

    // FTST
    fpu.ftst(F(-1));
    CHECK((fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_N);
    fpu.ftst(snan);
    CHECK(fpu.fpsr & FPSR::CC_NAN);
    CHECK(fpu.fpsr & FPEXP::SNAN);
    fpu.setFPSR(0);

    // FCMP
    fpu.fcmp(F(1), F(2));
    CHECK((fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_N);
    fpu.fcmp(F(2), F(2));
    CHECK((fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_Z);
    fpu.fcmp(F(3), F(2));
    CHECK((fpu.fpsr & FPSR::CC_MASK) == 0);
    fpu.fcmp(Float80::infinity(), Float80::infinity());
    CHECK((fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_Z);
    // The I flag is never set (the magnitudes are compared instead of subtracting)
    fpu.fcmp(Float80::infinity(), F(1));
    CHECK((fpu.fpsr & FPSR::CC_MASK) == 0);
    fpu.fcmp(F(1), Float80::infinity());
    CHECK((fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_N);
    fpu.fcmp(qnan, F(1));
    CHECK((fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_NAN);
    CHECK(!(fpu.fpsr & (FPEXP::INEX2 | FPEXP::OVFL | FPEXP::OPERR)));
    fpu.setFPSR(0);

    // FMOD and FREM
    auto q = [&] { return (fpu.fpsr >> 16) & 0xFF; };
    CHECK(same(fpu.fmod(F(10), F(3)), F(1)));
    CHECK(q() == 3);
    CHECK(same(fpu.frem(F(10), F(3)), F(1)));
    CHECK(q() == 3);
    CHECK(same(fpu.fmod(F(11), F(3)), F(2)));
    CHECK(q() == 3);
    CHECK(same(fpu.frem(F(11), F(3)), F(-1)));
    CHECK(q() == 4);
    CHECK(same(fpu.fmod(F(-11), F(3)), F(-2)));
    CHECK(q() == 0x83);
    CHECK(same(fpu.frem(F(-11), F(3)), F(1)));
    CHECK(q() == 0x84);
    CHECK(same(fpu.fmod(F(11), F(-3)), F(2)));
    CHECK(q() == 0x83);
    CHECK(same(fpu.fmod(F(2), F(5)), F(2)));
    CHECK(q() == 0);
    CHECK(same(fpu.fmod(F(6), F(3)), F(0)));
    CHECK(same(fpu.fmod(F(-6), F(3)), Float80::zero(true)));
    CHECK(same(fpu.fmod(F(5), Float80::infinity()), F(5)));
    CHECK(same(fpu.fmod(F(0.1), F(0.1)), Float80::zero()));
    fpu.setFPSR(0);

    // Large quotients: 10^20 mod 7 and the low bits of the quotient
    {
        unsigned __int128 a = (unsigned __int128)10000000000ULL * 10000000000ULL;
        auto r = fpu.fmod(Float80::fromInt(0) + F(1e20), F(7));
        // 1e20 as double is exactly 100000000000000000000
        CHECK(same(r, Float80::fromInt((i64)(a % 7))));
        CHECK(q() == (u8)((a / 7) & 0x7F));
    }

    // A quotient with more than 64 bits
    {
        auto big = Float80::fromBits(0x3FFF + 200, 0xDEADBEEFCAFEBABEULL);
        auto small = Float80::fromBits(0x3FFF + 3, 0x9000000000000000ULL);    // 9 * 2^0 ... = 1.125 * 8
        auto r = fpu.fmod(big, small);
        // Reference: (m1 * 2^137) mod 9 and ((m1 * 2^137) / 9) mod 128, where big = m1 * 2^137, small = 9
        u64 m1 = 0xDEADBEEFCAFEBABEULL;
        unsigned rem = 0, qlow = 0;
        // Binary long division of m1 << 137 by 9
        for (int i = 63 + 137; i >= 0; i--) {
            unsigned bit = (i >= 137) ? (m1 >> (i - 137)) & 1 : 0;
            rem = rem * 2 + bit;
            unsigned b = rem >= 9;
            if (b) rem -= 9;
            qlow = (qlow << 1 | b) & 0x7F;
        }
        CHECK(same(r, Float80::fromInt(rem)));
        CHECK(q() == qlow);
    }
    fpu.setFPSR(0);

    // FSGLMUL and FSGLDIV round to single precision
    auto x = Float80::fromBits(0x3FFF, 0x8000008000000000ULL);    // 1 + 2^-24
    auto y = Float80::fromBits(0x3FFF, 0x8000010000000000ULL);    // 1 + 2^-23
    auto p = fpu.fsglmul(x, y);
    CHECK((p.mantissa() & 0x000000FFFFFFFFFFULL) == 0);
    CHECK(same(fpu.fsglmul(F(3), F(5)), F(15)));
    CHECK(same(fpu.fsgldiv(F(15), F(5)), F(3)));
    CHECK(((fpu.fsgldiv(F(1), F(3))).mantissa() & 0x000000FFFFFFFFFFULL) == 0);
    CHECK(sf.precision() == Precision::EXTENDED);
    fpu.setFPSR(0);
}

static void testRoundingPrecision()
{
    FPU fpu;
    auto third = Float80::one() / Float80::fromInt(3);

    // FSADD / FDADD style operations
    {
        FPU::Scope scope(fpu, Precision::SINGLE);
        CHECK(fpu.sf.precision() == Precision::SINGLE);
        auto r = fpu.fdiv(Float80::one(), Float80::fromInt(3));
        CHECK(r.mantissa() == 0xAAAAAB0000000000ULL);
    }
    {
        FPU::Scope scope(fpu, Precision::DOUBLE);
        auto r = fpu.fdiv(Float80::one(), Float80::fromInt(3));
        CHECK(r.mantissa() == 0xAAAAAAAAAAAAA800ULL);
    }
    CHECK(sf.precision() == Precision::EXTENDED);

    // Rounding of a value (FMOVE FPm,FPn)
    {
        fpu.setFPCR(FPCR::PREC_SINGLE | FPCR::RND_ZERO);
        FPU::Scope scope(fpu);
        CHECK(fpu.round(third).mantissa() == 0xAAAAAA0000000000ULL);
        fpu.setFPCR(0);
    }

    // Unnormals and denormals are normalized
    {
        FPU::Scope scope(fpu);
        auto unnormal = Float80::fromBits(0x4000, 0x4000000000000000ULL);
        CHECK(same(fpu.round(unnormal), Float80::one()));
        auto denormal = Float80::fromBits(0x0000, 0x4000000000000000ULL);
        auto r = fpu.round(denormal);
        CHECK(r.isDenormal() && r.mantissa() == 0x4000000000000000ULL);
    }
}

static void testNaNConversions()
{
    SoftFloat::Scope scope(sf);
    sf.clearFlags();

    // Signaling NaNs stay signaling, quiet NaNs stay quiet, and no flags are raised
    auto snan = FpuFormats::fromSingle(0x7F800001);
    CHECK(snan.isSignalingNaN());
    CHECK(same(snan, Float80::fromBits(0x7FFF, 1ULL << 40)));      // Without integer bit, as the 68k FPUs do it
    CHECK(FpuFormats::fromSingle(0xFFC00000).isQuietNaN() && FpuFormats::fromSingle(0xFFC00000).sign());
    CHECK(FpuFormats::fromDouble(0x7FF0000000000001ULL).isSignalingNaN());
    CHECK(FpuFormats::fromDouble(0x7FF8000000000001ULL).isQuietNaN());
    CHECK(sf.flags() == 0);

    // The conversion back truncates the payload
    CHECK(FpuFormats::toSingle(FpuFormats::fromSingle(0x7FC12345)) == 0x7FC12345);
    CHECK(FpuFormats::toDouble(FpuFormats::fromDouble(0xFFF8000000000042ULL)) == 0xFFF8000000000042ULL);

    // A NaN never turns into an infinity
    CHECK(FpuFormats::toSingle(Float80::fromBits(0x7FFF, 0x8000000000000001ULL)) == 0x7FC00000);
    CHECK(FpuFormats::toDouble(Float80::fromBits(0x7FFF, 0x8000000000000001ULL)) == 0x7FF8000000000000ULL);
}

static void testFPU()
{
    FPU fpu;

    // Reset state
    for (auto &reg : fpu.fpr) CHECK(same(reg, Float80::quietNaN()));
    CHECK(fpu.inResetState());
    CHECK(fpu.getFPCR() == 0 && fpu.getFPSR() == 0);

    // FPCR decoding
    fpu.setFPCR(0x00FF);
    CHECK(fpu.getFPCR() == 0x00F0);
    CHECK(fpu.getPrecision() == Precision::DOUBLE && fpu.getRoundingMode() == RoundingMode::UP);
    fpu.setFPCR(0x0040 | 0x0010);
    CHECK(fpu.getPrecision() == Precision::SINGLE && fpu.getRoundingMode() == RoundingMode::ZERO);
    fpu.setFPCR(0);

    // The scope loads rounding mode and precision and collects the flags
    {
        fpu.setFPCR(FPCR::PREC_SINGLE | FPCR::RND_ZERO);
        FPU::Scope scope(fpu);
        CHECK(fpu.sf.roundingMode() == RoundingMode::ZERO);
        CHECK(fpu.sf.precision() == Precision::SINGLE);
        auto r = Float80::one().in(fpu.sf) / Float80::fromInt(3);
        CHECK(r.mantissa() == 0xAAAAAA0000000000ULL);
    }
    CHECK(fpu.sf.roundingMode() == RoundingMode::NEAREST);
    CHECK(fpu.sf.precision() == Precision::EXTENDED);
    CHECK(fpu.fpsr & FPEXP::INEX2);
    CHECK(fpu.fpsr & (1 << 3));
    CHECK(!(fpu.fpsr & FPEXP::DZ));

    fpu.setFPSR(0);
    {
        FPU::Scope scope(fpu);
        (void)(Float80::one().in(fpu.sf) / Float80::zero());
    }
    CHECK(fpu.fpsr & FPEXP::DZ);
    CHECK(fpu.fpsr & (1 << 4));

    // Condition codes
    fpu.setConditionCodes(Float80::fromInt(-5));
    CHECK((fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_N);
    fpu.setConditionCodes(Float80::zero(true));
    CHECK((fpu.fpsr & FPSR::CC_MASK) == (FPSR::CC_N | FPSR::CC_Z));
    fpu.setConditionCodes(Float80::infinity());
    CHECK((fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_I);
    fpu.setConditionCodes(Float80::quietNaN());
    CHECK((fpu.fpsr & FPSR::CC_MASK) == (FPSR::CC_N | FPSR::CC_NAN) || (fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_NAN);

    // Quotient byte
    fpu.setQuotientByte(0x15, true);
    CHECK(((fpu.fpsr >> 16) & 0xFF) == 0x95);

    // Constant ROM
    fpu.setFPCR(0);
    fpu.setFPSR(0);
    CHECK(same(fpu.readCR(0x00), Float80::fromBits(0x4000, 0xc90fdaa22168c235)));
    CHECK(fpu.fpsr & FPEXP::INEX2);
    CHECK(same(fpu.readCR(0x33), Float80::fromInt(10)));
    CHECK(same(fpu.readCR(0x0F), Float80::zero()));

    // State frames
    fpu.setModel(FPUModel::M68882);
    CHECK(fpu.stateFrameSize(FpuFrameType::IDLE) == 56);
    CHECK(fpu.typeOfFrame(fpu.computeFormatWord(FpuFrameType::BUSY)) == FpuFrameType::BUSY);
    fpu.setModel(FPUModel::M68881);
    CHECK(fpu.stateFrameSize(FpuFrameType::BUSY) == 180);
}

static void testTranscendental()
{
    FPU fpu;
    fpu.setModel(FPUModel::M68882);
    FPU::Scope scope(fpu);

    auto near = [](const Float80 &a, long double expected) {
        long double v = a.toLongDouble();
        return std::fabs(double(v - expected)) <= std::fabs(double(expected)) * 1e-12;
    };

    // Exact special cases
    CHECK(same(fpu.fetox(Float80::zero()), Float80::one()));
    CHECK(same(fpu.fcos(Float80::zero()), Float80::one()));
    CHECK(same(fpu.fsin(Float80::zero(true)), Float80::zero(true)));
    CHECK(same(fpu.ftwotox(Float80::fromInt(10)), Float80::fromInt(1024)));
    CHECK(same(fpu.ftwotox(Float80::fromInt(-3)), Float80::fromDouble(0.125)));
    CHECK(same(fpu.ftentox(Float80::fromInt(9)), Float80::fromInt(1000000000)));
    CHECK(same(fpu.flog2(Float80::fromInt(1024)), Float80::fromInt(10)));
    CHECK(same(fpu.flog10(Float80::fromInt(1000)), Float80::fromInt(3)));
    CHECK(same(fpu.flogn(Float80::one()), Float80::zero()));
    CHECK(same(fpu.ftanh(Float80::infinity(true)), Float80::one(true)));
    CHECK(same(fpu.fetox(Float80::infinity(true)), Float80::zero()));
    CHECK(near(fpu.fatan(Float80::infinity()), 1.5707963267948966192L));

    // Values
    CHECK(near(fpu.fsin(Float80::fromInt(1)), std::sin(1.0L)));
    CHECK(near(fpu.fcos(Float80::fromInt(2)), std::cos(2.0L)));
    CHECK(near(fpu.ftan(Float80::fromInt(1)), std::tan(1.0L)));
    CHECK(near(fpu.fatan(Float80::fromInt(1)), std::atan(1.0L)));
    CHECK(near(fpu.fasin(Float80::fromDouble(0.5)), std::asin(0.5L)));
    CHECK(near(fpu.facos(Float80::fromDouble(0.5)), std::acos(0.5L)));
    CHECK(near(fpu.fsinh(Float80::fromInt(1)), std::sinh(1.0L)));
    CHECK(near(fpu.fcosh(Float80::fromInt(1)), std::cosh(1.0L)));
    CHECK(near(fpu.ftanh(Float80::fromInt(1)), std::tanh(1.0L)));
    CHECK(near(fpu.fatanh(Float80::fromDouble(0.5)), std::atanh(0.5L)));
    CHECK(near(fpu.fetox(Float80::fromInt(1)), std::exp(1.0L)));
    CHECK(near(fpu.fetoxm1(Float80::fromDouble(1e-5)), std::expm1(1e-5L)));
    CHECK(near(fpu.flogn(Float80::fromInt(10)), std::log(10.0L)));
    CHECK(near(fpu.flognp1(Float80::fromDouble(1e-5)), std::log1p(1e-5L)));

    // sin(pi) is tiny, but not zero. Compare to the part of pi not covered by the 64-bit significand.
    Float80 pi = Float80::fromBits(0x4000, 0xC90FDAA22168C235ULL);
    Float80 s = fpu.fsin(pi);
    CHECK(s.exponent() < -60 && s.exponent() > -70);

    // Large arguments are reduced exactly (reference value computed offline)
    Float80 big = Float80::fromBits(0x3FFF + 100, 1ULL << 63);
    Float80 sb = fpu.fsin(big);
    CHECK(near(sb, -0.87218360541826730978L));

    // Exceptions
    fpu.sf.clearFlags();
    Float80 r = fpu.fsin(Float80::infinity());
    CHECK(r.isNaN() && (fpu.sf.flags() & FloatFlags::INVALID));
    CHECK(fpu.fasin(Float80::fromInt(2)).isNaN());
    CHECK(fpu.flogn(Float80::fromInt(-1)).isNaN());
    CHECK(same(fpu.flogn(Float80::zero()), Float80::infinity(true)));
    CHECK(same(fpu.fatanh(Float80::one()), Float80::infinity()));

    // sincos agrees with sin and cos
    Float80 sn, cs;
    fpu.fsincos(Float80::fromDouble(0.7), sn, cs);
    CHECK(same(sn, fpu.fsin(Float80::fromDouble(0.7))));
    CHECK(same(cs, fpu.fcos(Float80::fromDouble(0.7))));
}

int main()
{
    testIntegers();
    testFloats();
    testPackedIn();
    testPackedOut();
    testPackedRoundTrip();
    testArithmetic();
    testRoundingPrecision();
    testNaNConversions();
    testFPU();
    testTranscendental();

    printf("%d checks, %d failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
