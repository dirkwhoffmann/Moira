// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "FPU.h"
#include <bit>

namespace moira {

void
FPU::reset()
{
    // After reset, all data registers contain a non-signaling NaN
    for (auto &reg : fpr) reg = Float80::quietNaN();

    fpiar = 0;
    fpsr = 0;
    fpcr = 0;

    resetState = true;
}

bool
FPU::isSupported(Instr I) const
{
    switch (I) {

        // Instructions that are only available on the 6888x. The 68040 traps them.
        case Instr::FACOS:     case Instr::FASIN:     case Instr::FATANH:    case Instr::FCOS:
        case Instr::FCOSH:     case Instr::FETOX:     case Instr::FETOXM1:   case Instr::FGETEXP:
        case Instr::FGETMAN:   case Instr::FLOG10:    case Instr::FLOG2:     case Instr::FLOGN:
        case Instr::FLOGNP1:   case Instr::FMOD:      case Instr::FREM:      case Instr::FSCAL:
        case Instr::FSIN:      case Instr::FSINCOS:   case Instr::FSINH:     case Instr::FTAN:
        case Instr::FTANH:     case Instr::FTENTOX:   case Instr::FTWOTOX:   case Instr::FATAN:
            return model != FPUModel::INTERNAL;

        // Instructions that are only available on the 68040
        case Instr::FSABS:     case Instr::FDABS:     case Instr::FSADD:     case Instr::FDADD:
        case Instr::FSDIV:     case Instr::FDDIV:     case Instr::FSMOVE:    case Instr::FDMOVE:
        case Instr::FSMUL:     case Instr::FDMUL:     case Instr::FSNEG:     case Instr::FDNEG:
        case Instr::FSSQRT:    case Instr::FDSQRT:    case Instr::FSSUB:     case Instr::FDSUB:
            return model == FPUModel::INTERNAL;

        default:
            return true;
    }
}

Precision
FPU::getPrecision() const
{
    switch (fpcr & FPCR::PREC_MASK) {

        case FPCR::PREC_EXTENDED:   return Precision::EXTENDED;
        case FPCR::PREC_SINGLE:     return Precision::SINGLE;
        default:                    return Precision::DOUBLE;
    }
}

RoundingMode
FPU::getRoundingMode() const
{
    switch (fpcr & FPCR::RND_MASK) {

        case FPCR::RND_NEAREST:     return RoundingMode::NEAREST;
        case FPCR::RND_ZERO:        return RoundingMode::ZERO;
        case FPCR::RND_DOWN:        return RoundingMode::DOWN;
        default:                    return RoundingMode::UP;
    }
}

void
FPU::setExcStatusBit(u32 mask)
{
    fpsr |= mask & FPSR::EXC_MASK;

    // Update the accrued exception byte (sticky bits)
    if (fpsr & (FPEXP::SNAN | FPEXP::OPERR))                    fpsr |= 1 << 7;
    if (fpsr & FPEXP::OVFL)                                     fpsr |= 1 << 6;
    if ((fpsr & FPEXP::UNFL) && (fpsr & FPEXP::INEX2))          fpsr |= 1 << 5;
    if (fpsr & FPEXP::DZ)                                       fpsr |= 1 << 4;
    if (fpsr & (FPEXP::INEX1 | FPEXP::INEX2 | FPEXP::OVFL))     fpsr |= 1 << 3;
}

void
FPU::setConditionCodes(const Float80 &value)
{
    fpsr &= ~FPSR::CC_MASK;

    if (value.isNegative()) fpsr |= FPSR::CC_N;
    if (value.isZero())     fpsr |= FPSR::CC_Z;
    if (value.isInfinite()) fpsr |= FPSR::CC_I;
    if (value.isNaN())      fpsr |= FPSR::CC_NAN;
}

void
FPU::setQuotientByte(u8 byte)
{
    fpsr = (fpsr & ~FPSR::QUOTIENT_MASK) | u32(byte) << 16;
}

void
FPU::setQuotientByte(u8 byte, bool sign)
{
    setQuotientByte((byte & 0x7F) | (sign ? 0x80 : 0x00));
}

FPU::Scope::Scope(FPU &fpu) : fpu(fpu)
{
    FloatEnv::setRoundingMode(fpu.getRoundingMode());
    FloatEnv::setPrecision(fpu.getPrecision());
    FloatEnv::clearFlags();
}

FPU::Scope::Scope(FPU &fpu, Precision precision) : Scope(fpu)
{
    FloatEnv::setPrecision(precision);
}

FPU::Scope::~Scope()
{
    fpu.collectFlags();
}

void
FPU::collectFlags()
{
    auto flags = FloatEnv::flags();
    u32 mask = 0;

    if (flags & FloatFlags::INEXACT)        mask |= FPEXP::INEX2;
    if (flags & FloatFlags::UNFL)           mask |= FPEXP::UNFL;
    if (flags & FloatFlags::OVFL)           mask |= FPEXP::OVFL;
    if (flags & FloatFlags::DIV_BY_ZERO)    mask |= FPEXP::DZ;
    if (flags & FloatFlags::INVALID)        mask |= FPEXP::OPERR;

    if (mask) setExcStatusBit(mask);
    FloatEnv::clearFlags();
}

//
// Arithmetic
//

namespace {

/* Runs an operation that must not produce NaN operands. If the operation
 * reports an invalid operation, the x87 default NaN produced by SoftFloat is
 * replaced by the default NaN of the 68k FPUs.
 */
template <typename F> Float80
invoke(F operation)
{
    auto saved = FloatEnv::flags();
    FloatEnv::clearFlags();

    auto result = operation();

    if ((FloatEnv::flags() & FloatFlags::INVALID) && result.isNaN()) {
        result = FPU::defaultNaN();
    }
    FloatEnv::raiseFlags(saved);
    return result;
}

// Runs an operation in extended precision with rounding to nearest
template <typename F> Float80
invokeExact(F operation)
{
    FloatEnv::Scope scope;
    FloatEnv::setPrecision(Precision::EXTENDED);
    FloatEnv::setRoundingMode(RoundingMode::NEAREST);
    return invoke(operation);
}

// Runs an operation and discards all exceptions it raises
template <typename F> Float80
invokeSilently(F operation)
{
    auto saved = FloatEnv::flags();
    auto result = operation();
    FloatEnv::clearFlags();
    FloatEnv::raiseFlags(saved);
    return result;
}

// Returns the exponent of the least significant mantissa bit
i32 lsbExponent(const Float80 &x)
{
    return std::max<i32>(1, x.biasedExponent()) - Float80::bias - 63;
}

/* Computes the seven least significant bits of floor(|a| / |b|) for finite
 * numbers a and b with b != 0.
 */
u8
truncatedQuotient(const Float80 &a, const Float80 &b)
{
    u64 ma = a.mantissa(), mb = b.mantissa();
    if (ma == 0) return 0;

    i32 d = lsbExponent(a) - lsbExponent(b);

    if (d < 0) {

        // The quotient is ma / (mb * 2^-d)
        if (-d > std::countl_zero(mb)) return 0;
        return u8((ma / (mb << -d)) & 0x7F);
    }

    // Binary long division of ma * 2^d by mb. Only the last bits are of interest.
    u64 rem = 0, q = 0;
    for (i32 i = 63; i >= -d; i--) {

        bool carry = rem >> 63;
        rem = rem << 1 | (i >= 0 ? (ma >> i) & 1 : 0);
        bool bit = carry || rem >= mb;
        if (bit) rem -= mb;
        q = q << 1 | bit;
    }
    return u8(q & 0x7F);
}

}

Float80
FPU::round(const Float80 &value)
{
    if (!value.isFinite() || value.isZero()) return value;

    // The exponent field of denormalized numbers counts as 1
    return Float80::roundPack(value.sign(), std::max<i32>(1, value.biasedExponent()), value.mantissa());
}

std::optional<Float80>
FPU::resolveNan(const Float80 &op1, const Float80 &op2)
{
    /* MC68881/MC68882 Floating-Point Coprocessor User's Manual:
     *
     * 4.5.4.1 NON-SIGNALING NANS
     *
     * "If either, but not both, operand of an operation is a NAN, and it is a
     *  non-signaling NAN, then that NAN is returned as the result. If both
     *  operands are non-signaling NANs, then the destination operand
     *  non-signaling NAN is returned as the result.
     *
     * 4.5.4.2 SIGNALING NANS
     *
     * If either operand to an operation is a signaling NAN (SNAN), then the
     * SNAN bit is set in the FPSR EXC byte. If the SNAN trap enable bit is
     * set in the FPCR ENABLE byte, then the trap is taken and the destination
     * is not modified. If the SNAN trap enable bit is not set, then the SNAN
     * is converted to a non-signaling NAN (by setting the SNAN bit in the
     * operand to a one), and the operation continues as described in the
     * preceding section for non-signaling NANs."
     */
    if (!op1.isNaN() && !op2.isNaN()) return std::nullopt;

    auto quiet = [&](const Float80 &op) {

        if (op.isSignalingNaN()) setExcStatusBit(FPEXP::SNAN);
        return Float80::fromBits(op.signExp(), op.mantissa() | (1ULL << 62));
    };

    // Both operands are processed, because both can raise the SNAN exception
    auto r1 = quiet(op1);
    auto r2 = quiet(op2);

    return op1.isNaN() ? r1 : r2;
}

std::optional<Float80>
FPU::resolveNan(const Float80 &op)
{
    return resolveNan(op, Float80::zero());
}

Float80
FPU::fabs(const Float80 &value)
{
    if (auto nan = resolveNan(value)) return nan->abs();
    return value.abs();
}

Float80
FPU::fneg(const Float80 &value)
{
    if (auto nan = resolveNan(value)) return -*nan;
    return -value;
}

Float80
FPU::fsqrt(const Float80 &value)
{
    if (auto nan = resolveNan(value)) return *nan;
    return invoke([&] { return value.sqrt(); });
}

Float80
FPU::fint(const Float80 &value)
{
    if (auto nan = resolveNan(value)) return *nan;
    return value.roundToInt();
}

Float80
FPU::fintrz(const Float80 &value)
{
    if (auto nan = resolveNan(value)) return *nan;

    FloatEnv::Scope scope;
    FloatEnv::setRoundingMode(RoundingMode::ZERO);
    return value.roundToInt();
}

Float80
FPU::fgetexp(const Float80 &value)
{
    if (auto nan = resolveNan(value)) return *nan;

    if (value.isInfinite()) { setExcStatusBit(FPEXP::OPERR); return defaultNaN(); }
    if (value.isZero()) return value;

    // The exponent of a denormalized number is determined after normalization
    i32 e = std::max<i32>(1, value.biasedExponent()) - Float80::bias - std::countl_zero(value.mantissa());
    return Float80::fromInt(e);
}

Float80
FPU::fgetman(const Float80 &value)
{
    if (auto nan = resolveNan(value)) return *nan;

    if (value.isInfinite()) { setExcStatusBit(FPEXP::OPERR); return defaultNaN(); }
    if (value.isZero()) return value;

    u64 man = value.mantissa() << std::countl_zero(value.mantissa());
    return Float80::fromBits(u16((value.sign() ? 0x8000 : 0) | Float80::bias), man);
}

Float80
FPU::ftst(const Float80 &value)
{
    auto result = value;
    if (auto nan = resolveNan(value)) result = *nan;

    setConditionCodes(result);
    return result;
}

Float80
FPU::fadd(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) return *nan;
    return invoke([&] { return dst + src; });
}

Float80
FPU::fsub(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) return *nan;
    return invoke([&] { return dst - src; });
}

Float80
FPU::fmul(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) return *nan;
    return invoke([&] { return dst * src; });
}

Float80
FPU::fdiv(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) return *nan;
    return invoke([&] { return dst / src; });
}

Float80
FPU::fscal(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) return *nan;

    // Scaling by an infinite value is invalid
    if (src.isInfinite()) { setExcStatusBit(FPEXP::OPERR); return defaultNaN(); }

    // Infinities and zeroes are unaffected by scaling
    if (dst.isInfinite() || dst.isZero()) return dst;

    return invoke([&] { return dst.scale(src); });
}

Float80
FPU::fsglmul(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) return *nan;

    // Round both operands to single precision, then multiply
    FloatEnv::Scope scope;
    FloatEnv::setPrecision(Precision::SINGLE);
    auto a = invokeSilently([&] { return round(dst); });
    auto b = invokeSilently([&] { return round(src); });
    return invoke([&] { return a * b; });
}

Float80
FPU::fsgldiv(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) return *nan;

    // Round both operands to single precision, then divide
    FloatEnv::Scope scope;
    FloatEnv::setPrecision(Precision::SINGLE);
    auto a = invokeSilently([&] { return round(dst); });
    auto b = invokeSilently([&] { return round(src); });
    return invoke([&] { return a / b; });
}

Float80
FPU::fmod(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) return *nan;

    // The remainder is always exact
    auto r = invokeExact([&] { return dst.rem(src); });

    // Invalid operations (dst infinite, src zero) have no quotient
    if (r.isNaN()) return r;

    bool negative = dst.sign() != src.sign();
    if (src.isInfinite() || dst.isZero()) { setQuotientByte(0, negative); return dst; }

    u8 q = truncatedQuotient(dst, src);
    setQuotientByte(q, negative);

    // rem rounds the quotient to nearest. Correct the result if it was rounded away from zero.
    if (!r.isZero() && r.sign() != dst.sign()) {

        auto adjust = dst.sign() ? -src.abs() : src.abs();
        r = invokeExact([&] { return r + adjust; });
    }
    return r.isZero() ? Float80::zero(dst.sign()) : r;
}

Float80
FPU::frem(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) return *nan;

    auto r = invokeExact([&] { return dst.rem(src); });
    if (r.isNaN()) return r;

    bool negative = dst.sign() != src.sign();
    if (src.isInfinite() || dst.isZero()) { setQuotientByte(0, negative); return dst; }

    // rem rounds the quotient to nearest, i.e., up if the remainder has the opposite sign
    u8 q = truncatedQuotient(dst, src);
    if (!r.isZero() && r.sign() != dst.sign()) q++;
    setQuotientByte(q, negative);

    return r.isZero() ? Float80::zero(dst.sign()) : r;
}

void
FPU::fcmp(const Float80 &dst, const Float80 &src)
{
    if (auto nan = resolveNan(dst, src)) { setConditionCodes(*nan); return; }

    // The condition codes reflect the difference dst - src
    Float80 diff;
    if (dst.isInfinite() && src.isInfinite() && dst.sign() == src.sign()) {
        diff = Float80::zero();
    } else {
        diff = invokeSilently([&] { return invokeExact([&] { return dst - src; }); });
    }
    setConditionCodes(diff);
}

Float80
FPU::readCR(unsigned nr)
{
    struct RomEntry { u16 hi; u64 lo; i64 r1; i64 r2; bool inex; };

    static constexpr RomEntry rom1[] = {

        { 0x4000, 0xc90fdaa22168c235, -1,  0,  1 }, // 0x00: Pi
        { 0x4001, 0xfe00068200000000,  0,  0,  0 }, // 0x01: Undocumented
        { 0x4001, 0xffc0050380000000,  0,  0,  0 }, // 0x02: Undocumented
        { 0x2000, 0x7FFFFFFF00000000,  0,  0,  0 }, // 0x03: Undocumented
        { 0x0000, 0xFFFFFFFFFFFFFFFF,  0,  0,  0 }, // 0x04: Undocumented
        { 0x3C00, 0xFFFFFFFFFFFFF800,  0,  0,  0 }, // 0x05: Undocumented
        { 0x3F80, 0xFFFFFF0000000000,  0,  0,  0 }, // 0x06: Undocumented
        { 0x0001, 0xF65D8D9C00000000,  0,  0,  0 }, // 0x07: Undocumented
        { 0x7FFF, 0x401E000000000000,  0,  0,  0 }, // 0x08: Undocumented
        { 0x43F3, 0xE000000000000000,  0,  0,  0 }, // 0x09: Undocumented
        { 0x4072, 0xC000000000000000,  0,  0,  0 }, // 0x0A: Undocumented
        { 0x3ffd, 0x9a209a84fbcff798,  0,  1,  1 }, // 0x0B: Log10(2)
        { 0x4000, 0xadf85458a2bb4a9a,  0,  1,  1 }, // 0x0C: E
        { 0x3fff, 0xb8aa3b295c17f0bc, -1,  0,  1 }, // 0x0D: Log2(e)
        { 0x3ffd, 0xde5bd8a937287195,  0,  0,  0 }, // 0x0E: Log10(e)
        { 0x0000, 0x0000000000000000,  0,  0,  0 }  // 0x0F: 0.0
    };

    static constexpr RomEntry rom2[] = {

        { 0x3ffe, 0xb17217f7d1cf79ac, -1,  0,  1  }, // 0x30: Ln(2)
        { 0x4000, 0x935d8dddaaa8ac17, -1,  0,  1  }, // 0x31: Ln(10)
        { 0x3FFF, 0x8000000000000000,  0,  0,  0  }, // 0x32: 10^0
        { 0x4002, 0xA000000000000000,  0,  0,  0  }, // 0x33: 10^1
        { 0x4005, 0xC800000000000000,  0,  0,  0  }, // 0x34: 10^2
        { 0x400C, 0x9C40000000000000,  0,  0,  0  }, // 0x35: 10^4
        { 0x4019, 0xBEBC200000000000,  0,  0,  0  }, // 0x36: 10^8
        { 0x4034, 0x8E1BC9BF04000000,  0,  0,  0  }, // 0x37: 10^16
        { 0x4069, 0x9DC5ADA82B70B59E, -1,  0,  1  }, // 0x38: 10^32
        { 0x40D3, 0xC2781F49FFCFA6D5,  0,  1,  1  }, // 0x39: 10^64
        { 0x41A8, 0x93BA47C980E98CE0, -1,  0,  1  }, // 0x3A: 10^128
        { 0x4351, 0xAA7EEBFB9DF9DE8E, -1,  0,  1  }, // 0x3B: 10^256
        { 0x46A3, 0xE319A0AEA60E91C7, -1,  0,  1  }, // 0x3C: 10^512
        { 0x4D48, 0xC976758681750C17,  0,  1,  1  }, // 0x3D: 10^1024
        { 0x5A92, 0x9E8B3B5DC53D5DE5, -1,  0,  1  }, // 0x3E: 10^2048
        { 0x7525, 0xC46052028A20979B, -1,  0,  1  }  // 0x3F: 10^4096
    };

    auto readRom = [&](const RomEntry &entry) {

        u64 lo = entry.lo;

        // Adjust the last digit if the rounding mode requires it
        switch (fpcr & FPCR::RND_MASK) {

            case FPCR::RND_ZERO:    lo += entry.r1; break;
            case FPCR::RND_DOWN:    lo += entry.r1; break;
            case FPCR::RND_UP:      lo += entry.r2; break;
        }

        // Mark the value as inexact if necessary
        if (entry.inex) setExcStatusBit(FPEXP::INEX2);

        return Float80::fromBits(entry.hi, lo);
    };

    // Values outside the two ranges seem to produce a Guru on the real machine
    if (nr < 0x10) return readRom(rom1[nr]);
    if (nr >= 0x30 && nr < 0x40) return readRom(rom2[nr - 0x30]);

    return Float80::zero();
}

int
FPU::stateFrameSize(FpuFrameType type) const
{
    switch (type) {

        case FpuFrameType::IDLE:

            switch (model) {
                case FPUModel::INTERNAL:    return 0;
                case FPUModel::M68881:      return 24;
                case FPUModel::M68882:      return 56;
            }
            break;

        case FpuFrameType::UNIMP:

            return model == FPUModel::INTERNAL ? 44 : 0;

        case FpuFrameType::BUSY:

            switch (model) {
                case FPUModel::INTERNAL:    return 92;
                case FPUModel::M68881:      return 180;
                case FPUModel::M68882:      return 212;
            }
            break;

        default:
            break;
    }

    return 0;
}

int
FPU::stateFrameSize(u32 fmtWord) const
{
    return (fmtWord >> 16) & 0xFF;
}

FpuFrameType
FPU::typeOfFrame(u32 fmtWord) const
{
    int size = stateFrameSize(fmtWord);

    if (size == stateFrameSize(FpuFrameType::NULL_FRAME)) return FpuFrameType::NULL_FRAME;
    if (size == stateFrameSize(FpuFrameType::IDLE)) return FpuFrameType::IDLE;
    if (size == stateFrameSize(FpuFrameType::UNIMP)) return FpuFrameType::UNIMP;
    if (size == stateFrameSize(FpuFrameType::BUSY)) return FpuFrameType::BUSY;

    return FpuFrameType::INVALID;
}

u32
FPU::computeFormatWord(FpuFrameType type) const
{
    switch (type) {

        case FpuFrameType::IDLE:
        case FpuFrameType::UNIMP:
        case FpuFrameType::BUSY:

            return 0x1F000000 | u32(stateFrameSize(type)) << 16;

        default:

            return 0;
    }
}

}
