// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "FPU.h"

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
