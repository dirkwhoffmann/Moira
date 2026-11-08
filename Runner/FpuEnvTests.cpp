// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* Differential test of FpuEnv (software floating-point on unpacked numbers)
 * against Float80 (Berkeley SoftFloat plus adapter). Both are fed with random
 * operands in all rounding modes and precisions. Results and exception flags
 * have to be identical.
 */

#include "MoiraConfig.h"
#include "FPU/FPU.h"
#include "FPU/Float80.h"
#include "FPU/SoftFloat.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <string>

using namespace moira;

static std::mt19937_64 rng(1);

static MFloat80 genOperand(bool irregular)
{
    bool neg = rng() & 1;
    u16 sign = neg ? 0x8000 : 0;
    u64 rnd = rng();
    u64 payload = rng();

    switch (rng() % 18) {

        case 0:  return MFloat80::zero(neg);
        case 1:  return MFloat80::infinity(neg);
        case 2:  return MFloat80::fromBits(sign | 0x7FFF, (3ULL << 62) | (payload & ((1ULL << 62) - 1)));
        case 3:  return MFloat80::fromBits(sign | 0x7FFF, (1ULL << 63) | (payload & ((1ULL << 62) - 1)) | 1);
        case 4:  return MFloat80::fromBits(sign, rnd & ~(1ULL << 63) ? (rnd & ~(1ULL << 63)) : 1);      // Denormal
        case 5:  return MFloat80::fromBits(sign | (1 + rnd % 70), rnd | (1ULL << 63));                  // Tiny
        case 6:  return MFloat80::fromBits(sign | (0x7FFE - rnd % 70), rnd | (1ULL << 63));             // Huge
        case 7:  return MFloat80::fromBits(sign | (0x3FFF - 3 + rnd % 7), rnd | (1ULL << 63));          // Around 1
        case 8:  return MFloat80::fromInt(i64(rnd % 41) - 20);                                         // Small integers
        case 9:  return MFloat80::fromBits(sign | (0x3FFF + 20 + rnd % 50), 0x8000000000000000ULL | (rnd & 0xFFFF000000000000ULL));
        case 10: return MFloat80::fromBits(sign | (0x3FFF - 120 + rnd % 240), rnd | (1ULL << 63));
        case 11: return MFloat80::fromBits(sign | (0x3F80 - 3 + rnd % 8), rnd | (1ULL << 63));          // Around the single range
        case 12: return MFloat80::fromBits(sign | (0x3C00 - 3 + rnd % 8), rnd | (1ULL << 63));          // Around the double range
        case 13: return MFloat80::fromBits(sign | (0x407E - 3 + rnd % 8), rnd | (1ULL << 63));
        case 14: return MFloat80::fromBits(sign | (0x43FE - 3 + rnd % 8), rnd | (1ULL << 63));
        case 15:
            // Irregular encodings
            if (irregular) return MFloat80::fromBits(sign | (rng() % 0x7FFF), rnd & ~(1ULL << 63));
            return MFloat80::fromBits(sign | (0x3FFF - 2 + rnd % 5), rnd | (1ULL << 63));
        default: return MFloat80::fromBits(sign | (0x3FFF - 70 + rnd % 140), rnd | (1ULL << 63));
    }
}

// Unnormals are not normalized by SoftFloat, which yields meaningless results for some operations
static bool unnormalized(const MFloat80 &x) { return x.isFinite() && x.mantissa() && !x.integerBit() && x.biasedExponent(); }

static Float80 old(const MFloat80 &x) { return Float80::fromBits(x.signExp(), x.mantissa()); }
static MFloat80 neu(const Float80 &x) { return MFloat80::fromBits(x.signExp(), x.mantissa()); }

struct Stats { int runs = 0, fails = 0; };
static std::map<std::string, Stats> stats;
static int totalFails = 0;
static bool irregularOperands = false;

static void report(const char *op, const char *what, RoundingMode m, Precision p, bool limit,
                   const MFloat80 &a, const MFloat80 &b, const std::string &expected, const std::string &got)
{
    auto &s = stats[std::string(op) + (irregularOperands ? " (irregular)" : "")];
    s.fails++;
    totalFails++;
    if (s.fails <= 3)
        printf("MISMATCH %s%s %s: mode=%d prec=%d limit=%d a=%s b=%s\n    old: %s\n    new: %s\n",
               op, irregularOperands ? " (irregular)" : "", what, (int)m, (int)p, limit,
               a.str().c_str(), b.str().c_str(), expected.c_str(), got.c_str());
}

static std::string flagStr(u8 f) { char b[16]; snprintf(b, sizeof(b), "flags=%02X", f); return b; }
static std::string res(const MFloat80 &r, u8 f) { return r.str() + " " + flagStr(f); }

// Compares the results of two operations including the exception flags
static void check(const char *op, RoundingMode m, Precision p, bool limit, const MFloat80 &a, const MFloat80 &b,
                  const MFloat80 &r1, u8 f1, const MFloat80 &r2, u8 f2)
{
    stats[std::string(op) + (irregularOperands ? " (irregular)" : "")].runs++;

    // All NaNs with the same payload are equivalent, but the bits must match
    if (!r1.identical(r2) || f1 != f2) report(op, "result", m, p, limit, a, b, res(r1, f1), res(r2, f2));
}

static void test(int iterations, bool irregular)
{
    irregularOperands = irregular;
    SoftFloat sf;
    FpuEnv env;

    static const RoundingMode modes[] = { RoundingMode::NEAREST, RoundingMode::ZERO, RoundingMode::DOWN, RoundingMode::UP };
    static const Precision precs[] = { Precision::EXTENDED, Precision::DOUBLE, Precision::SINGLE };

    for (int i = 0; i < iterations; i++) {

        auto m = modes[rng() % 4];
        auto p = precs[rng() % 3];
        bool limit = rng() % 4 != 0;

        MFloat80 a = genOperand(irregular), b = genOperand(irregular);
        Float80 oa = old(a).in(sf), ob = old(b).in(sf);

        auto prepare = [&] {
            sf.setRoundingMode(m); sf.setPrecision(p); sf.setLimitsExponent(limit); sf.clearFlags();
            env.setRoundingMode(m); env.setPrecision(p); env.setLimitsExponent(limit); env.clearFlags();
        };

        #define BINARY(NAME, EXPR_OLD, EXPR_NEW) { \
            prepare(); \
            MFloat80 r1 = neu(EXPR_OLD); u8 f1 = sf.flags(); \
            MFloat80 r2 = EXPR_NEW; u8 f2 = env.flags(); \
            check(NAME, m, p, limit, a, b, r1, f1, r2, f2); }

        BINARY("add", oa + ob, env.add(a, b))
        BINARY("sub", oa - ob, env.sub(a, b))
        BINARY("mul", oa * ob, env.mul(a, b))
        BINARY("div", oa / ob, env.div(a, b))
        BINARY("sqrt", oa.sqrt(), env.sqrt(a))
        BINARY("roundToInt", oa.roundToInt(), env.roundToInt(a))
        BINARY("scale", oa.scale(ob), env.scale(a, b))
        BINARY("round", oa.rounded(), env.round(a))

        // Comparisons
        prepare();
        bool e1 = oa == ob, l1 = oa < ob, g1 = oa <= ob; u8 cf1 = sf.flags();
        bool e2 = env.eq(a, b), l2 = env.lt(a, b), g2 = env.le(a, b); u8 cf2 = env.flags();
        stats[std::string("compare") + (irregular ? " (irregular)" : "")].runs++;
        if (!unnormalized(a) && !unnormalized(b) && (e1 != e2 || l1 != l2 || g1 != g2 || cf1 != cf2))
            report("compare", "result", m, p, limit, a, b,
                   std::to_string(e1) + std::to_string(l1) + std::to_string(g1) + " " + flagStr(cf1),
                   std::to_string(e2) + std::to_string(l2) + std::to_string(g2) + " " + flagStr(cf2));

        // Integer conversions
        prepare();
        i64 i1[4] = { oa.toInt32(), oa.toInt32Truncated(), oa.toInt64(), oa.toInt64Truncated() };
        u8 if1 = sf.flags();
        env.clearFlags();
        i64 i2[4] = { env.toInt32(a), env.toInt32Truncated(a), env.toInt64(a), env.toInt64Truncated(a) };
        stats[std::string("toInt") + (irregular ? " (irregular)" : "")].runs++;
        if (!unnormalized(a) && (memcmp(i1, i2, sizeof(i1)) || if1 != env.flags()))
            report("toInt", "result", m, p, limit, a, b,
                   std::to_string(i1[0]) + "," + std::to_string(i1[1]) + "," + std::to_string(i1[2]) + "," + std::to_string(i1[3]) + " " + flagStr(if1),
                   std::to_string(i2[0]) + "," + std::to_string(i2[1]) + "," + std::to_string(i2[2]) + "," + std::to_string(i2[3]) + " " + flagStr(env.flags()));

        // Conversions to single and double precision
        prepare();
        float s1 = oa.toFloat(); double d1 = oa.toDouble(); u8 ff1 = sf.flags();
        env.clearFlags();
        float s2 = env.toFloat(a); double d2 = env.toDouble(a); u8 ff2 = env.flags();
        stats[std::string("toFloat") + (irregular ? " (irregular)" : "")].runs++;
        if (!unnormalized(a) && (memcmp(&s1, &s2, sizeof(s1)) || memcmp(&d1, &d2, sizeof(d1)) || ff1 != ff2)) {
            char b1[64], b2[64];
            snprintf(b1, sizeof(b1), "%08X %016llX %s", std::bit_cast<u32>(s1), (unsigned long long)std::bit_cast<u64>(d1), flagStr(ff1).c_str());
            snprintf(b2, sizeof(b2), "%08X %016llX %s", std::bit_cast<u32>(s2), (unsigned long long)std::bit_cast<u64>(d2), flagStr(ff2).c_str());
            report("toFloat", "result", m, p, limit, a, b, b1, b2);
        }

        // Conversions from single and double precision
        {
            u64 bits = rng();
            if (rng() & 1) bits = (bits & 0x807FFFFF00000000ULL) | (u64(rng() % 4 ? (rng() % 255) : (rng() & 1 ? 0 : 255)) << 55 & 0);
            float f = std::bit_cast<float>(u32(bits));
            double d = std::bit_cast<double>(bits);
            prepare();
            MFloat80 x1 = neu(Float80::fromFloat(f, sf)); MFloat80 y1 = neu(Float80::fromDouble(d, sf)); u8 g1f = sf.flags();
            env.clearFlags();
            MFloat80 x2 = env.fromFloat(f); MFloat80 y2 = env.fromDouble(d); u8 g2f = env.flags();
            stats[std::string("fromFloat") + (irregular ? " (irregular)" : "")].runs++;
            if (!x1.identical(x2) || !y1.identical(y2) || g1f != g2f)
                report("fromFloat", "result", m, p, limit, x1, y1, x1.str() + " " + y1.str() + " " + flagStr(g1f), x2.str() + " " + y2.str() + " " + flagStr(g2f));
        }

        // Integers
        {
            i64 v = (i64)rng() >> (rng() % 64);
            if (!neu(Float80::fromInt(v)).identical(MFloat80::fromInt(v))) {
                stats["fromInt"].fails++; totalFails++; printf("MISMATCH fromInt %lld\n", (long long)v);
            }
            stats["fromInt"].runs++;
        }
    }
}

int main(int argc, char **argv)
{
    int iterations = argc > 1 ? atoi(argv[1]) : 200000;
    if (argc > 2) rng.seed(atoi(argv[2]));

    test(iterations, false);
    test(iterations, true);

    for (auto &[name, s] : stats) printf("%-24s %9d runs %8d failures\n", name.c_str(), s.runs, s.fails);
    printf("%d total failures\n", totalFails);
    return totalFails != 0;
}
