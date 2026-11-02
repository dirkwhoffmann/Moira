// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "FPU.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

using namespace moira;

static int failures = 0;
static int checks = 0;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
    printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static bool same(const Float80 &a, const Float80 &b) { return a.identical(b); }

static void testIntegers()
{
    FloatEnv::Scope scope;
    FloatEnv::clearFlags();

    CHECK(FpuFormats::toInteger(Float80::fromInt(42), 8) == 42);
    CHECK(FloatEnv::flags() == 0);
    CHECK(FpuFormats::toInteger(Float80::fromInt(-129), 8) == -128);
    CHECK(FloatEnv::flags() & FloatFlags::INVALID);
    FloatEnv::clearFlags();
    CHECK(FpuFormats::toInteger(Float80::fromInt(70000), 16) == 32767);
    CHECK(FpuFormats::toInteger(Float80::fromInt(-3000000000LL), 32) == INT32_MIN);
    FloatEnv::clearFlags();

    // Rounding modes
    auto x = Float80::fromDouble(2.5);
    FloatEnv::setRoundingMode(RoundingMode::NEAREST);
    CHECK(FpuFormats::toInteger(x, 32) == 2);
    FloatEnv::setRoundingMode(RoundingMode::UP);
    CHECK(FpuFormats::toInteger(x, 32) == 3);
    FloatEnv::setRoundingMode(RoundingMode::DOWN);
    CHECK(FpuFormats::toInteger(Float80::fromDouble(-2.5), 32) == -3);
    FloatEnv::setRoundingMode(RoundingMode::ZERO);
    CHECK(FpuFormats::toInteger(Float80::fromDouble(-2.5), 32) == -2);
    CHECK(FloatEnv::flags() & FloatFlags::INEXACT);
    FloatEnv::clearFlags();

    // Special values
    CHECK(FpuFormats::toInteger(Float80::infinity(true), 32) == INT32_MIN);
    CHECK(FpuFormats::toInteger(Float80::quietNaN(), 32) == INT32_MAX);
    CHECK(FloatEnv::flags() & FloatFlags::INVALID);
}

static void testFloats()
{
    FloatEnv::Scope scope;
    FloatEnv::clearFlags();

    CHECK(FpuFormats::toSingle(Float80::one()) == 0x3F800000);
    CHECK(FpuFormats::toDouble(Float80::one()) == 0x3FF0000000000000ULL);
    CHECK(same(FpuFormats::fromSingle(0x3F800000), Float80::one()));
    CHECK(same(FpuFormats::fromDouble(0xBFF0000000000000ULL), Float80::one(true)));
    CHECK(same(FpuFormats::fromSingle(0x7F800000), Float80::infinity()));
    CHECK(FloatEnv::flags() == 0);

    // Rounding of 1/3 to single and double
    auto third = Float80::one() / Float80::fromInt(3);
    CHECK(FpuFormats::toDouble(third) == 0x3FD5555555555555ULL);
    CHECK(FpuFormats::toSingle(third) == 0x3EAAAAAB);
    CHECK(FloatEnv::flags() & FloatFlags::INEXACT);
    FloatEnv::clearFlags();

    // Extended precision in memory
    u32 w[3];
    FpuFormats::toExtended(Float80::fromBits(0xC001, 0xC000000000000000ULL), w);
    CHECK(w[0] == 0xC0010000 && w[1] == 0xC0000000 && w[2] == 0);
    CHECK(same(FpuFormats::fromExtended(w), Float80::fromBits(0xC001, 0xC000000000000000ULL)));
}

static void testPackedIn()
{
    FloatEnv::Scope scope;
    FloatEnv::clearFlags();

    // 1.0E0
    u32 one[3] = { 0x00000001, 0, 0 };
    CHECK(same(FpuFormats::fromPacked(one), Float80::one()));

    // -1.5E2
    u32 m150[3] = { 0x80020001, 0x50000000, 0 };
    CHECK(same(FpuFormats::fromPacked(m150), Float80::fromInt(-150)));
    CHECK(FloatEnv::flags() == 0);

    // 1.0E-5 is not exactly representable
    u32 tiny[3] = { 0x40050001, 0, 0 };
    auto r = FpuFormats::fromPacked(tiny);
    CHECK(r.toDouble() == 1e-5);
    CHECK(FloatEnv::flags() & FloatFlags::INEXACT);
    FloatEnv::clearFlags();

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
    CHECK(FloatEnv::flags() & FloatFlags::OVFL);
}

static void testPackedOut()
{
    FloatEnv::Scope scope;
    FloatEnv::clearFlags();
    u32 w[3];

    FpuFormats::toPacked(Float80::one(), 17, w);
    CHECK(w[0] == 0x00000001 && w[1] == 0 && w[2] == 0);
    CHECK(FloatEnv::flags() == 0);

    FpuFormats::toPacked(Float80::fromInt(-150), 17, w);
    CHECK(w[0] == 0x80020001 && w[1] == 0x50000000 && w[2] == 0);

    // 0.5 = 5.0E-1
    FpuFormats::toPacked(Float80::fromDouble(0.5), 17, w);
    CHECK(w[0] == 0x40010005 && w[1] == 0 && w[2] == 0);

    // 1/3 with 17 digits: 3.3333333333333333E-1 (inexact)
    FpuFormats::toPacked(Float80::one() / Float80::fromInt(3), 17, w);
    CHECK(w[0] == 0x40010003 && w[1] == 0x33333333 && w[2] == 0x33333333);
    CHECK(FloatEnv::flags() & FloatFlags::INEXACT);
    FloatEnv::clearFlags();

    // k-factor: three significant digits of 2/3 -> 6.67E-1
    FpuFormats::toPacked(Float80::fromInt(2) / Float80::fromInt(3), 3, w);
    CHECK(w[0] == 0x40010006 && w[1] == 0x67000000 && w[2] == 0);
    FloatEnv::setRoundingMode(RoundingMode::ZERO);
    FpuFormats::toPacked(Float80::fromInt(2) / Float80::fromInt(3), 3, w);
    CHECK(w[1] == 0x66000000);
    FloatEnv::setRoundingMode(RoundingMode::NEAREST);
    FloatEnv::clearFlags();

    // Negative k-factor: two digits right of the decimal point (123.456 -> 123.46)
    FpuFormats::toPacked(Float80::fromDouble(123.456), -2, w);
    CHECK(w[0] == 0x00020001 && w[1] == 0x23460000);
    FloatEnv::clearFlags();

    // Rounding carries into a new digit: 9.99 with k = 2 -> 1.0E1
    FpuFormats::toPacked(Float80::fromDouble(9.99), 2, w);
    CHECK(w[0] == 0x00010001 && w[1] == 0);

    // k > 17 raises the invalid flag
    FloatEnv::clearFlags();
    FpuFormats::toPacked(Float80::one(), 18, w);
    CHECK(FloatEnv::flags() & FloatFlags::INVALID);
    FloatEnv::clearFlags();

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
    FloatEnv::Scope scope;
    double values[] = { 1.0, 0.1, 123456.789, 1e300, 1e-300, 4.9406564584124654e-324,
                        2.2250738585072014e-308, 1.7976931348623157e308, 3.141592653589793,
                        -2.718281828459045, 1e22, 1e23, 9007199254740993.0 };

    for (double d : values) {

        u32 w[3];
        FloatEnv::clearFlags();
        FpuFormats::toPacked(Float80::fromDouble(d), 17, w);
        auto back = FpuFormats::fromPacked(w);

        if (back.toDouble() != d) printf("round trip failed for %.17g (got %.17g)\n", d, back.toDouble());
        CHECK(back.toDouble() == d);
    }
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
        CHECK(FloatEnv::roundingMode() == RoundingMode::ZERO);
        CHECK(FloatEnv::precision() == Precision::SINGLE);
        auto r = Float80::one() / Float80::fromInt(3);
        CHECK(r.mantissa() == 0xAAAAAA0000000000ULL);
    }
    CHECK(FloatEnv::roundingMode() == RoundingMode::NEAREST);
    CHECK(FloatEnv::precision() == Precision::EXTENDED);
    CHECK(fpu.fpsr & FPEXP::INEX2);
    CHECK(fpu.fpsr & (1 << 3));
    CHECK(!(fpu.fpsr & FPEXP::DZ));

    fpu.setFPSR(0);
    {
        FPU::Scope scope(fpu);
        (void)(Float80::one() / Float80::zero());
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

int main()
{
    testIntegers();
    testFloats();
    testPackedIn();
    testPackedOut();
    testPackedRoundTrip();
    testFPU();

    printf("%d checks, %d failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
