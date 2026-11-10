// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/* Compares Moira's FPU arithmetic and format conversions against the
 * floating-point back-end of UAE (see UAERef.h). Both sides get the
 * same randomly chosen operands, rounding modes and precisions. Result bits and
 * exception status bits are compared. Mismatches are collected and summarized,
 * so one run shows all kinds of deviations at once.
 *
 * The run can be tuned through environment variables: FPUREF_ITERATIONS
 * (default 2000), FPUREF_SEED (default 1), FPUREF_CLEAN (restricts the
 * operands to Extended precision without NaNs and denormals) and
 * FPUREF_MAXPRINT (number of mismatches printed per category).
 */

#include "doctest.h"
#include "FPU/FPU.h"
#include "FPU/FpuFormats.h"
#include "FPU/Wide.h"
#include "UAEGlue/UAERef.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <string>

using namespace moira;

namespace {

std::mt19937_64 rng;

// Restricts the tests to the well-understood part: Extended precision and
// no NaNs or denormals as operands
bool clean = false;

struct Stats {
    long checks = 0;
    long failures = 0;
    int printed = 0;
    std::map<std::string, long> categories;
    long known = 0;         // Mismatches that are accepted deviations from the reference
};
std::map<std::string, Stats> stats;
int maxPrinted = 3;

//
// Generating operands
//

MFloat80 genOperandRaw();

MFloat80 genOperand()
{
    for (;;) {
        MFloat80 v = genOperandRaw();
        if (!clean || !(v.isNaN() || (v.biasedExponent() == 0 && !v.isZero()))) return v;
    }
}

MFloat80 genOperandRaw()
{
    bool neg = rng() & 1;
    u16 sign = neg ? 0x8000 : 0;
    u64 rnd = rng();
    u64 payload = rng();

    switch (rng() % 16) {

        case 0:  return MFloat80::zero(neg);
        case 1:  return MFloat80::infinity(neg);
        case 2:  return MFloat80::fromBits(sign | 0x7FFF, (3ULL << 62) | (payload & ((1ULL << 62) - 1)));
        case 3:  return MFloat80::fromBits(sign | 0x7FFF, (1ULL << 63) | (payload & ((1ULL << 62) - 1)) | 1);
        case 4:  return MFloat80::fromBits(sign, rnd & ~(1ULL << 63) ? (rnd & ~(1ULL << 63)) : 1);  // Denormal
        case 5:  return MFloat80::fromBits(sign | (1 + rnd % 70), rnd | (1ULL << 63));              // Tiny
        case 6:  return MFloat80::fromBits(sign | (0x7FFE - rnd % 70), rnd | (1ULL << 63));         // Huge
        case 7:  return MFloat80::fromBits(sign | (0x3FFF - 3 + rnd % 7), rnd | (1ULL << 63));      // Around 1
        case 8:  return MFloat80::fromInt(i64(rnd % 41) - 20);                                     // Small integers
        case 9:  return MFloat80::fromBits(sign | (0x3FFF + 20 + rnd % 50), 0x8000000000000000ULL | (rnd & 0xFFFF000000000000ULL));
        case 10: return MFloat80::fromBits(sign | (0x3FFF - 120 + rnd % 240), rnd | (1ULL << 63));
        default: return MFloat80::fromBits(sign | (0x3FFF - 70 + rnd % 140), rnd | (1ULL << 63));
    }
}

u32 genFPCR()
{
    static const u32 precisions[] = { 0x00, 0x40, 0x80 };
    u32 prec = precisions[rng() % 3];
    return (clean ? 0 : prec) | ((rng() & 3) << 4);
}

//
// Reporting
//

// Classifies the operands of a mismatching test (helps to triage the results)
std::string classify(u32 fpcr, std::initializer_list<MFloat80> operands)
{
    std::string c;
    for (auto &v : operands) {
        if (v.isNaN()) { c = "NaN operand"; break; }
        if (v.biasedExponent() == 0 && !v.isZero()) c = "denormal operand";
    }
    if (c.empty() && (fpcr & 0xC0)) c = "reduced precision";
    if (c.empty()) for (auto &v : operands) if (v.isInfinite()) c = "infinity operand";
    return c.empty() ? "other" : c;
}

std::string str(const MFloat80 &v) { return v.str(); }
std::string str(u16 se, u64 man) { return MFloat80::fromBits(se, man).str(); }

struct Reporter {

    std::string key;
    Stats &s;
    std::string category = "other";

    explicit Reporter(const std::string &k) : key(k), s(stats[k]) { s.checks++; }

    void known() { s.known++; }

    template <typename... Args>
    void fail(const char *fmt, Args... args)
    {
        s.failures++;
        s.categories[category]++;
        if (s.printed++ < maxPrinted) {
            printf("MISMATCH %s\n   ", key.c_str());
            printf(fmt, args...);
            printf("\n");
        }
    }
};

//
// Arithmetic
//

enum class Op { MOVE, INT, INTRZ, SQRT, ABS, NEG, GETEXP, GETMAN,
                DIV, MOD, ADD, MUL, SGLDIV, REM, SCALE, SGLMUL, SUB };

struct OpDesc {
    u8 cmd;
    const char *name;
    Op op;
    bool only6888x;     // Not available on the 68040
    bool only040;       // Not available on the 6888x
};

const OpDesc ops[] = {

    { 0x00, "fmove",   Op::MOVE,   false, false },
    { 0x01, "fint",    Op::INT,    false, false },
    { 0x03, "fintrz",  Op::INTRZ,  false, false },
    { 0x04, "fsqrt",   Op::SQRT,   false, false },
    { 0x18, "fabs",    Op::ABS,    false, false },
    { 0x1a, "fneg",    Op::NEG,    false, false },
    { 0x1e, "fgetexp", Op::GETEXP, true,  false },
    { 0x1f, "fgetman", Op::GETMAN, true,  false },
    { 0x20, "fdiv",    Op::DIV,    false, false },
    { 0x21, "fmod",    Op::MOD,    true,  false },
    { 0x22, "fadd",    Op::ADD,    false, false },
    { 0x23, "fmul",    Op::MUL,    false, false },
    { 0x24, "fsgldiv", Op::SGLDIV, true,  false },
    { 0x25, "frem",    Op::REM,    true,  false },
    { 0x26, "fscale",  Op::SCALE,  true,  false },
    { 0x27, "fsglmul", Op::SGLMUL, true,  false },
    { 0x28, "fsub",    Op::SUB,    false, false },

    { 0x40, "fsmove",  Op::MOVE,   false, true },
    { 0x44, "fdmove",  Op::MOVE,   false, true },
    { 0x41, "fssqrt",  Op::SQRT,   false, true },
    { 0x45, "fdsqrt",  Op::SQRT,   false, true },
    { 0x58, "fsabs",   Op::ABS,    false, true },
    { 0x5c, "fdabs",   Op::ABS,    false, true },
    { 0x5a, "fsneg",   Op::NEG,    false, true },
    { 0x5e, "fdneg",   Op::NEG,    false, true },
    { 0x60, "fsdiv",   Op::DIV,    false, true },
    { 0x64, "fddiv",   Op::DIV,    false, true },
    { 0x62, "fsadd",   Op::ADD,    false, true },
    { 0x66, "fdadd",   Op::ADD,    false, true },
    { 0x63, "fsmul",   Op::MUL,    false, true },
    { 0x67, "fdmul",   Op::MUL,    false, true },
    { 0x68, "fssub",   Op::SUB,    false, true },
    { 0x6c, "fdsub",   Op::SUB,    false, true },
};

MFloat80 execMoira(FPU &fpu, const OpDesc &d, const MFloat80 &dst, const MFloat80 &src, u32 &quot)
{
    MFloat80 result;
    quot = 0;

    // FS* and FD* instructions override the rounding precision
    bool override = d.cmd & 0x40;
    Precision prec = (d.cmd & 4) ? Precision::DOUBLE : Precision::SINGLE;

    {
        FPU::Scope scope = override ? FPU::Scope(fpu, prec) : FPU::Scope(fpu);

        switch (d.op) {

            case Op::MOVE:   result = fpu.round(src); break;
            case Op::INT:    result = fpu.fint(src); break;
            case Op::INTRZ:  result = fpu.fintrz(src); break;
            case Op::SQRT:   result = fpu.fsqrt(src); break;
            case Op::ABS:    result = fpu.fabs(src); break;
            case Op::NEG:    result = fpu.fneg(src); break;
            case Op::GETEXP: result = fpu.fgetexp(src); break;
            case Op::GETMAN: result = fpu.fgetman(src); break;
            case Op::DIV:    result = fpu.fdiv(dst, src); break;
            case Op::MOD:    result = fpu.fmod(dst, src); break;
            case Op::ADD:    result = fpu.fadd(dst, src); break;
            case Op::MUL:    result = fpu.fmul(dst, src); break;
            case Op::SGLDIV: result = fpu.fsgldiv(dst, src); break;
            case Op::REM:    result = fpu.frem(dst, src); break;
            case Op::SCALE:  result = fpu.fscal(dst, src); break;
            case Op::SGLMUL: result = fpu.fsglmul(dst, src); break;
            case Op::SUB:    result = fpu.fsub(dst, src); break;
        }
    }
    quot = (fpu.fpsr >> 16) & 0xFF;
    return result;
}

void testArithmetic(int model, long iterations)
{
    bool is040 = model == 68040;
    FPU fpu;
    fpu.setModel(is040 ? FPUModel::BUILTIN : FPUModel::M68882);
    uaeref::init(model);

    for (long i = 0; i < iterations; i++) {

        for (const OpDesc &d : ops) {

            if (is040 ? d.only6888x : d.only040) continue;

            MFloat80 dst = genOperand(), src = genOperand();
            u32 fpcr = genFPCR();

            // Moira
            fpu.reset();
            fpu.setFPCR(fpcr);
            fpu.setFPSR(0);
            u32 quot;
            MFloat80 res = execMoira(fpu, d, dst, src, quot);
            u32 exc = fpu.fpsr & 0xFF00;

            // Reference
            uaeref::Result ref;
            if (!uaeref::arith(d.cmd, dst.signExp(), dst.mantissa(), src.signExp(), src.mantissa(), fpcr, ref)) {
                printf("Reference doesn't implement command %02x\n", d.cmd);
                continue;
            }

            Reporter rep(std::string(d.name) + (is040 ? " (040)" : " (6888x)"));
            bool sameValue = res.signExp() == ref.se && res.mantissa() == ref.man;
            bool sameExc = exc == ref.exc;
            bool sameQuot = !(d.op == Op::MOD || d.op == Op::REM) || quot == ref.quot;

            if (!sameValue || !sameExc || !sameQuot) {

                // Accepted deviations from the reference
                bool infOperand = dst.isInfinite() || src.isInfinite();
                bool zeroByInf = (d.op == Op::MOD || d.op == Op::REM) && dst.isZero() && src.isInfinite();
                // The reference returns a zero significand with the minimum exponent of the format
                bool pseudoZero = ref.man == 0 && (ref.se & 0x7FFF) != 0 && (ref.se & 0x7FFF) != 0x7FFF;
                if (pseudoZero && res.isZero() && res.sign() == bool(ref.se & 0x8000) && (exc == ref.exc || (exc | uaeref::UNFL) == ref.exc)) { rep.known(); continue; }

                bool overflow = (res.signExp() & 0x7FFF) == 0x7FFF || (exc & FPEXP::OVFL);
                if ((overflow && sameValue && exc == (ref.exc | uaeref::INEX2)) ||      // Inexact exception on overflow
                    (d.op == Op::SUB && infOperand && !is040 && (exc == ref.exc)) ||    // 6888x quirk
                    (zeroByInf && res.isZero() && ref.man == 0 && res.sign() == bool(ref.se & 0x8000) && (exc | uaeref::UNFL) == ref.exc)) {
                    rep.known();
                    continue;
                }


                rep.category = classify(fpcr, { dst, src });
                if (sameValue && sameQuot) rep.category += " (flags only)";
                rep.fail("fpcr=%02x dst=%s src=%s\n   moira=%s exc=%04x q=%02x   ref=%s exc=%04x q=%02x",
                         fpcr, str(dst).c_str(), str(src).c_str(),
                         str(res).c_str(), exc, quot,
                         str(ref.se, ref.man).c_str(), ref.exc, ref.quot);
            }
        }
    }
}

//
// Transcendental functions
//

struct TransDesc {
    u8 cmd;
    const char *name;
    MFloat80 (FPU::*fn)(const MFloat80 &);
};

const TransDesc transOps[] = {

    { 0x02, "fsinh",    &FPU::fsinh },    { 0x06, "flognp1",  &FPU::flognp1 },
    { 0x08, "fetoxm1",  &FPU::fetoxm1 },  { 0x09, "ftanh",    &FPU::ftanh },
    { 0x0a, "fatan",    &FPU::fatan },    { 0x0c, "fasin",    &FPU::fasin },
    { 0x0d, "fatanh",   &FPU::fatanh },   { 0x0e, "fsin",     &FPU::fsin },
    { 0x0f, "ftan",     &FPU::ftan },     { 0x10, "fetox",    &FPU::fetox },
    { 0x11, "ftwotox",  &FPU::ftwotox },  { 0x12, "ftentox",  &FPU::ftentox },
    { 0x14, "flogn",    &FPU::flogn },    { 0x15, "flog10",   &FPU::flog10 },
    { 0x16, "flog2",    &FPU::flog2 },    { 0x19, "fcosh",    &FPU::fcosh },
    { 0x1c, "facos",    &FPU::facos },    { 0x1d, "fcos",     &FPU::fcos },
};

// The reference is accurate to only a few ulps
int ulpTolerance = 8;

void testTranscendentals(long iterations)
{
    FPU fpu;
    fpu.setModel(FPUModel::M68882);
    uaeref::init(68882);

    for (long i = 0; i < iterations; i++) {

        for (const TransDesc &d : transOps) {

            MFloat80 src = genOperand();
            u32 fpcr = genFPCR() & ~0xC0u;      // Extended precision only

            fpu.reset();
            fpu.setFPCR(fpcr);
            fpu.setFPSR(0);
            MFloat80 res;
            {
                FPU::Scope scope(fpu);
                res = fpu.round((fpu.*d.fn)(src));
            }
            u32 exc = fpu.fpsr & 0xFF00;

            uaeref::Result ref;
            if (!uaeref::arith(d.cmd, 0, 0, src.signExp(), src.mantissa(), fpcr, ref)) {
                printf("Reference doesn't implement command %02x\n", d.cmd);
                continue;
            }

            Reporter rep(std::string(d.name));
            bool bothFinite = res.isFinite() && (ref.se & 0x7FFF) != 0x7FFF;
            bool sameValue = res.signExp() == ref.se && res.mantissa() == ref.man;
            u128 diff = 0;
            if (bothFinite) diff = ulpDistance((res.signExp() & 0x8000) != 0, ulpPosition(res.signExp(), res.mantissa()),
                                               (ref.se & 0x8000) != 0, ulpPosition(ref.se, ref.man));
            bool closeValue = sameValue || (bothFinite && diff <= ulpTolerance);
            bool sameExc = exc == ref.exc;

            // The reference reduces the argument of sin, cos, and tan with a pi
            // of limited precision, so its results degrade for large arguments.
            // Moira reduces exactly (Payne-Hanek).
            bool trig = d.cmd == 0x0e || d.cmd == 0x0f || d.cmd == 0x1d;
            if (trig && bothFinite && !closeValue && src.isFinite() && src.exponent() >= 0) {
                double err = std::ldexp(toDouble(diff), res.exponent() - 63);
                double slope = d.cmd == 0x0f ? 1.0 + std::ldexp(1.0, 2 * res.exponent()) : 1.0;
                if (src.exponent() >= 20 || err <= std::ldexp(slope, src.exponent() - 58)) closeValue = true;
            }

            // The reference has an absolute error of a few units of 2^-64
            if (!closeValue && bothFinite && std::ldexp(toDouble(diff), res.exponent() - 63) <= std::ldexp(1.0, -60)) closeValue = true;

            // The reference returns garbage in the low bits for tiny arguments
            // where the function is the identity
            if (!closeValue && src.isFinite() && src.exponent() < -40 && res.identical(src)) closeValue = true;

            // The reference omits the inexact flag if an overflow or underflow occurs
            if (!sameExc && (exc & (FPEXP::OVFL | FPEXP::UNFL)) && exc == (ref.exc | uaeref::INEX2)) sameExc = true;
            if (closeValue && !sameValue) rep.known();

            if (!closeValue || !sameExc) {

                rep.category = classify(fpcr, { src });
                if (closeValue) rep.category += " (flags only)";
                else if (bothFinite) rep.category += " (" + std::to_string((diff > 1000 ? 1000LL : (long long)lo64(diff))) + " ulp)";
                rep.fail("fpcr=%02x src=%s\n   moira=%s exc=%04x   ref=%s exc=%04x", fpcr, str(src).c_str(),
                         str(res).c_str(), exc, str(ref.se, ref.man).c_str(), ref.exc);
            }
        }
    }
}

//
// FETOXM1 with tiny arguments (all rounding precisions and modes)
//

void testTinyEtoxm1(long iterations)
{
    FPU fpu;
    fpu.setModel(FPUModel::M68882);
    uaeref::init(68882);

    // Powers of two at the lower end of the single, double, and extended range
    static const u16 boundaries[] = { 0x3F81, 0x3C01, 0x0001 };

    for (long i = 0; i < iterations * 4; i++) {

        bool neg = rng() & 1;
        u16 sign = neg ? 0x8000 : 0;
        u64 rnd = rng();
        u64 man = rnd | (1ULL << 63);
        u16 exp;

        switch (rng() % 4) {

            case 0:  exp = boundaries[rng() % 3]; man = 1ULL << 63; break;                 // Exactly at the boundary
            case 1:  exp = boundaries[rng() % 3]; man += i64(rng() % 5) - 2; break;         // Next to the boundary
            case 2:  exp = u16(rng() % 0x60); if (exp == 0) man &= ~(1ULL << 63); break;    // (Almost) denormal
            default: exp = u16(rng() % 0x3FBE); break;                                      // Anywhere below 2^-65
        }
        MFloat80 src = MFloat80::fromBits(sign | exp, man);
        if (src.isZero()) continue;

        u32 fpcr = genFPCR();

        fpu.reset();
        fpu.setFPCR(fpcr);
        fpu.setFPSR(0);
        MFloat80 res;
        {
            FPU::Scope scope(fpu);
            res = fpu.round(fpu.fetoxm1(src));
        }
        u32 exc = fpu.fpsr & 0xFF00;

        uaeref::Result ref;
        if (!uaeref::arith(0x08, 0, 0, src.signExp(), src.mantissa(), fpcr, ref)) continue;

        Reporter rep("fetoxm1 (tiny)");
        bool sameValue = res.signExp() == ref.se && res.mantissa() == ref.man;

        // The reference returns a zero significand with the minimum exponent of the format
        bool pseudoZero = ref.man == 0 && (ref.se & 0x7FFF) != 0 && (ref.se & 0x7FFF) != 0x7FFF;
        if (pseudoZero && res.isZero() && res.sign() == bool(ref.se & 0x8000) && exc == ref.exc) { rep.known(); continue; }

        if (!sameValue || exc != ref.exc) {
            rep.category = classify(fpcr, { src });
            rep.fail("fpcr=%02x src=%s\n   moira=%s exc=%04x   ref=%s exc=%04x", fpcr, str(src).c_str(),
                     str(res).c_str(), exc, str(ref.se, ref.man).c_str(), ref.exc);
        }
    }
}

//
// Conversions
//

u32 genSingle()
{
    u32 raw = u32(rng());
    switch (rng() % 8) {
        case 0: return raw & 0x807FFFFF;                        // Denormal or zero
        case 1: return (raw & 0x80000000) | 0x7F800000 | (raw & 0x7FFFFF);   // Inf or NaN
        case 2: return raw & 0xBF7FFFFF;
        default: return raw;
    }
}

u64 genDouble()
{
    u64 raw = rng();
    switch (rng() % 8) {
        case 0: return raw & 0x800FFFFFFFFFFFFFULL;
        case 1: return (raw & 0x8000000000000000ULL) | 0x7FF0000000000000ULL | (raw & 0xFFFFFFFFFFFFFULL);
        default: return raw;
    }
}

void testConversions(int model, long iterations)
{
    std::string tag = model == 68040 ? " (040)" : " (6888x)";
    FPU fpu;
    fpu.setModel(model == 68040 ? FPUModel::BUILTIN : FPUModel::M68882);
    uaeref::init(model);

    for (long i = 0; i < iterations; i++) {

        u32 fpcr = genFPCR();
        fpu.reset();
        fpu.setFPCR(fpcr);

        // Single precision to extended
        {
            u32 raw = genSingle();
            MFloat80 v; { fpu.setFPSR(0); FPU::Scope s(fpu); v = FpuFormats::fromSingle(fpu.env, raw); }
            auto ref = uaeref::fromSingle(raw, fpcr);
            Reporter rep("single -> extended" + tag);
            if (v.signExp() != ref.se || v.mantissa() != ref.man || (fpu.fpsr & 0xFF00) != ref.exc)
                rep.fail("raw=%08x\n   moira=%s exc=%04x   ref=%s exc=%04x", raw, str(v).c_str(),
                         fpu.fpsr & 0xFF00, str(ref.se, ref.man).c_str(), ref.exc);
        }

        // Double precision to extended
        {
            u64 raw = genDouble();
            MFloat80 v; { fpu.setFPSR(0); FPU::Scope s(fpu); v = FpuFormats::fromDouble(fpu.env, raw); }
            auto ref = uaeref::fromDouble(raw, fpcr);
            Reporter rep("double -> extended" + tag);
            if (v.signExp() != ref.se || v.mantissa() != ref.man || (fpu.fpsr & 0xFF00) != ref.exc)
                rep.fail("raw=%016llx\n   moira=%s exc=%04x   ref=%s exc=%04x", (unsigned long long)raw,
                         str(v).c_str(), fpu.fpsr & 0xFF00, str(ref.se, ref.man).c_str(), ref.exc);
        }

        // Extended to single
        {
            MFloat80 x = genOperand();
            u32 raw; { fpu.setFPSR(0); FPU::Scope s(fpu); raw = FpuFormats::toSingle(fpu.env, x); }
            auto ref = uaeref::toSingle(x.signExp(), x.mantissa(), fpcr);
            Reporter rep("extended -> single" + tag);
            if (raw == ref.raw[0] && (fpu.fpsr & FPEXP::OVFL) && (fpu.fpsr & 0xFF00) == (ref.exc | uaeref::INEX2)) rep.known(); else
            if (raw != ref.raw[0] || (fpu.fpsr & 0xFF00) != ref.exc)
                rep.fail("fpcr=%02x x=%s\n   moira=%08x exc=%04x   ref=%08x exc=%04x", fpcr, str(x).c_str(),
                         raw, fpu.fpsr & 0xFF00, ref.raw[0], ref.exc);
        }

        // Extended to double
        {
            MFloat80 x = genOperand();
            u64 raw; { fpu.setFPSR(0); FPU::Scope s(fpu); raw = FpuFormats::toDouble(fpu.env, x); }
            auto ref = uaeref::toDouble(x.signExp(), x.mantissa(), fpcr);
            u64 refRaw = u64(ref.raw[0]) << 32 | ref.raw[1];
            Reporter rep("extended -> double" + tag);
            if (raw == refRaw && (fpu.fpsr & FPEXP::OVFL) && (fpu.fpsr & 0xFF00) == (ref.exc | uaeref::INEX2)) rep.known(); else
            if (raw != refRaw || (fpu.fpsr & 0xFF00) != ref.exc)
                rep.fail("fpcr=%02x x=%s\n   moira=%016llx exc=%04x   ref=%016llx exc=%04x", fpcr, str(x).c_str(),
                         (unsigned long long)raw, fpu.fpsr & 0xFF00, (unsigned long long)refRaw, ref.exc);
        }

        // Extended to integer
        for (int bits : { 8, 16, 32 }) {

            MFloat80 x = genOperand();
            i32 v; { fpu.setFPSR(0); FPU::Scope s(fpu); v = FpuFormats::toInteger(fpu.env, x, bits); }
            auto ref = uaeref::toInteger(x.signExp(), x.mantissa(), bits, fpcr);
            u32 mask = bits == 8 ? 0xFF : bits == 16 ? 0xFFFF : 0xFFFFFFFF;
            Reporter rep("extended -> int" + std::to_string(bits) + tag);
            u32 ourExc = fpu.fpsr & 0xFF00;
            if ((u32(v) & mask) == (u32(ref.ival) & mask) && x.isSignalingNaN() && (ourExc & ~uaeref::OPERR) == ref.exc) rep.known(); else   // Reference omits OPERR
            if ((u32(v) & mask) != (u32(ref.ival) & mask) || (fpu.fpsr & 0xFF00) != ref.exc)
                rep.fail("fpcr=%02x x=%s\n   moira=%08x exc=%04x   ref=%08x exc=%04x", fpcr, str(x).c_str(),
                         u32(v) & mask, fpu.fpsr & 0xFF00, u32(ref.ival) & mask, ref.exc);
        }

        // Extended to packed
        {
            MFloat80 x = genOperand();
            int k = int(rng() % 128) - 64;
            if (rng() % 2) k = 1 + int(rng() % 17);
            u32 w[3] = {}; { fpu.setFPSR(0); FPU::Scope s(fpu); FpuFormats::toPacked(fpu.env, x, k, w); }
            auto ref = uaeref::toPacked(x.signExp(), x.mantissa(), k, fpcr);
            Reporter rep("extended -> packed" + tag);
            bool sameWords = w[0] == ref.raw[0] && w[1] == ref.raw[1] && w[2] == ref.raw[2];
            auto digits = [](const u32 (&x)[3]) {
                u64 v = x[0] & 0xF;
                for (int i = 28; i >= 0; i -= 4) v = v * 10 + ((x[1] >> i) & 0xF);
                for (int i = 28; i >= 0; i -= 4) v = v * 10 + ((x[2] >> i) & 0xF);
                return v;
            };
            u32 refw[3] = { ref.raw[0], ref.raw[1], ref.raw[2] };
            u64 dm = digits(w), dr = digits(refw);
            u64 unit = 1;
            for (int i = k; k > 0 && i < 17; i++) unit *= 10;
            bool lastDigit = (w[0] & ~0xFu) == (refw[0] & ~0xFu) && (dm > dr ? dm - dr : dr - dm) <= 2 * unit && (fpu.fpsr & 0xFF00) == ref.exc;
            // The reference may be off by one in the last digit, which changes the exponent if a carry occurs
            auto value = [&](const u32 (&x)[3]) {
                int e = int((x[0] >> 24 & 0xF) * 100 + (x[0] >> 20 & 0xF) * 10 + (x[0] >> 16 & 0xF) + (x[0] >> 12 & 0xF) * 1000);
                return (long double)digits(x) * powl(10.0L, (x[0] & 0x40000000 ? -e : e) - 16);
            };
            long double vm = value(w), vr = value(refw);
            bool carry = k > 0 && vm != 0 && vr != 0 && (w[0] & 0xFFFFF000) != (refw[0] & 0xFFFFF000) && fabsl(vm - vr) <= 11 * powl(10.0L, floorl(log10l(fabsl(vm) + 1e-300L)) - (k - 1)) && (fpu.fpsr & 0xFF00) == ref.exc;
            lastDigit = lastDigit || carry;
            bool mismatch = !sameWords || (fpu.fpsr & 0xFF00) != ref.exc;
            if (mismatch && ((k == 1 || k == 2 || k == 3) || lastDigit ||      // The reference is off by one in directed rounding modes                                           // Reference rounds to k-1 digits
                             (sameWords && ref.exc == ((fpu.fpsr & 0xFF00) | uaeref::INEX2)))) rep.known(); else
            if (w[0] != ref.raw[0] || w[1] != ref.raw[1] || w[2] != ref.raw[2] || (fpu.fpsr & 0xFF00) != ref.exc)
                rep.fail("fpcr=%02x k=%d x=%s\n   moira=%08x %08x %08x exc=%04x   ref=%08x %08x %08x exc=%04x",
                         fpcr, k, str(x).c_str(), w[0], w[1], w[2], fpu.fpsr & 0xFF00,
                         ref.raw[0], ref.raw[1], ref.raw[2], ref.exc);
        }

        // Packed to extended
        {
            u32 w[3];
            w[0] = (u32(rng()) & 0xC0000000) | ((rng() % 1000) << 16) | (u32(rng() % 10) & 0xF);
            // Build BCD digits
            auto bcd = [] { u32 v = 0; for (int i = 0; i < 8; i++) v = v << 4 | (rng() % 10); return v; };
            w[1] = bcd(); w[2] = bcd();
            // Exponent digits must be BCD
            u32 e = rng() % 1000;
            w[0] = (w[0] & 0xC000000F) | ((e / 100) << 24) | (((e / 10) % 10) << 20) | ((e % 10) << 16);
            MFloat80 v; { fpu.setFPSR(0); FPU::Scope s(fpu); v = FpuFormats::fromPacked(fpu.env, w); }
            u32 exc = fpu.fpsr & 0xFF00;
            if (exc & FPEXP::INEX2) exc = (exc & ~FPEXP::INEX2) | FPEXP::INEX1;
            auto ref = uaeref::fromPacked(w, fpcr);
            Reporter rep("packed -> extended" + tag);
            u64 diff = v.mantissa() > ref.man ? v.mantissa() - ref.man : ref.man - v.mantissa();
            if (v.signExp() == ref.se && exc == ref.exc && diff <= 256 && diff != 0) rep.known(); else
            if (v.signExp() != ref.se || v.mantissa() != ref.man || exc != ref.exc)
                rep.fail("w=%08x %08x %08x\n   moira=%s exc=%04x   ref=%s exc=%04x", w[0], w[1], w[2],
                         str(v).c_str(), exc, str(ref.se, ref.man).c_str(), ref.exc);
        }
    }
}

}

TEST_SUITE("FPU (reference)") {

TEST_CASE("Moira FPU vs. UAE")
{
    auto env = [](const char *name) { return getenv(name); };

    long iterations = env("FPUREF_ITERATIONS") ? atol(env("FPUREF_ITERATIONS")) : 2000;
    unsigned seed = env("FPUREF_SEED") ? unsigned(atol(env("FPUREF_SEED"))) : 1;
    clean = env("FPUREF_CLEAN") != nullptr;
    if (env("FPUREF_MAXPRINT")) maxPrinted = atoi(env("FPUREF_MAXPRINT"));
    rng.seed(seed);
    stats.clear();

    printf("Moira FPU vs. UAE: %ld iterations, seed %u%s\n\n", iterations, seed, clean ? " (clean)" : "");

    for (int model : { 68882, 68040 }) {
        testArithmetic(model, iterations);
        testConversions(model, iterations);
        if (model != 68040) { testTranscendentals(iterations); testTinyEtoxm1(iterations); }
    }

    long failures = 0;
    printf("\n%-28s %10s %10s %10s\n", "Test", "Checks", "Failures", "Accepted");
    for (auto &[name, s] : stats) {
        printf("%-28s %10ld %10ld %10ld%s\n", name.c_str(), s.checks, s.failures, s.known, s.failures ? "  <--" : "");
        for (auto &[cat, n] : s.categories) printf("      %-34s %6ld\n", cat.c_str(), n);
        failures += s.failures;
    }
    CHECK(failures == 0);
}

}
