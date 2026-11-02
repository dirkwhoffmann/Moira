// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "MoiraTypes.h"
#include "FpuFormats.h"
#include <optional>

namespace moira {

// Supported FPU models
enum class FPUModel
{
    NONE,           // No FPU installed (floating-point instructions cause a line F exception)
    INTERNAL,       // FPU built into the 68040
    M68881,         // External FPU
    M68882          // External FPU
};

// Types of FSAVE state frames
enum class FpuFrameType
{
    INVALID,
    NULL_FRAME,
    IDLE,
    UNIMP,
    BUSY
};

// Operand formats (source and destination specifier of FMOVE and arithmetic instructions)
enum class FpuFormat : int
{
    LONG        = 0,
    SINGLE      = 1,
    EXTENDED    = 2,
    PACKED      = 3,    // Packed decimal, k-factor is a static value
    WORD        = 4,
    DOUBLE      = 5,
    BYTE        = 6,
    PACKED_DYN  = 7     // Packed decimal, k-factor is taken from a data register
};

/* Floating-point control register (FPCR)
 *
 *   Bits 15..8: Exception enable byte (BSUN, SNAN, OPERR, OVFL, UNFL, DZ, INEX2, INEX1)
 *   Bits 7..6:  Rounding precision (00 = extended, 01 = single, 10 = double)
 *   Bits 5..4:  Rounding mode (00 = nearest, 01 = zero, 10 = down, 11 = up)
 */
namespace FPCR {

static constexpr u32 MASK           = 0x0000FFF0;
static constexpr u32 PREC_MASK      = 0x000000C0;
static constexpr u32 PREC_EXTENDED  = 0x00000000;
static constexpr u32 PREC_SINGLE    = 0x00000040;
static constexpr u32 PREC_DOUBLE    = 0x00000080;
static constexpr u32 RND_MASK       = 0x00000030;
static constexpr u32 RND_NEAREST    = 0x00000000;
static constexpr u32 RND_ZERO       = 0x00000010;
static constexpr u32 RND_DOWN       = 0x00000020;
static constexpr u32 RND_UP         = 0x00000030;
}

/* Floating-point status register (FPSR)
 *
 *   Bits 27..24: Condition code byte (N, Z, I, NAN)
 *   Bits 23..16: Quotient byte (sign and seven bits)
 *   Bits 15..8:  Exception status byte (same layout as the enable byte in FPCR)
 *   Bits 7..3:   Accrued exception byte (IOP, OVFL, UNFL, DZ, INEX)
 */
namespace FPSR {

static constexpr u32 MASK           = 0x0FFFFFF8;
static constexpr u32 CC_N           = 1 << 27;
static constexpr u32 CC_Z           = 1 << 26;
static constexpr u32 CC_I           = 1 << 25;
static constexpr u32 CC_NAN         = 1 << 24;
static constexpr u32 CC_MASK        = 0x0F000000;
static constexpr u32 QUOTIENT_MASK  = 0x00FF0000;
static constexpr u32 EXC_MASK       = 0x0000FF00;
static constexpr u32 AEXC_MASK      = 0x000000F8;
}

// Exception status bits (FPSR bits 15..8, FPCR enable bits 15..8)
namespace FPEXP {

static constexpr u32 BSUN           = 1 << 15;
static constexpr u32 SNAN           = 1 << 14;
static constexpr u32 OPERR          = 1 << 13;
static constexpr u32 OVFL           = 1 << 12;
static constexpr u32 UNFL           = 1 << 11;
static constexpr u32 DZ             = 1 << 10;
static constexpr u32 INEX2          = 1 << 9;
static constexpr u32 INEX1          = 1 << 8;
}

class FPU {

public:

    // Emulated FPU model
    FPUModel model = FPUModel::INTERNAL;

    // Floating-point data registers FP0 to FP7
    Float80 fpr[8];

    // Control register
    u32 fpcr = 0;

    // Status register
    u32 fpsr = 0;

    // Instruction address register
    u32 fpiar = 0;

    // Indicates whether the FPU is in reset state (affects FSAVE)
    bool resetState = true;


    //
    // Constructing
    //

public:

    FPU() { reset(); }

    // Initializes all registers with their reset values
    void reset();

    // Indicates whether the FPU is in reset state (used by FSAVE)
    bool inResetState() const { return resetState; }


    //
    // Configuring
    //

public:

    FPUModel getModel() const { return model; }
    bool isPresent() const { return model != FPUModel::NONE; }
    void setModel(FPUModel newModel) { model = newModel; }

    // Checks whether an instruction is implemented in hardware by the emulated model
    bool isSupported(Instr I) const;

    // Returns the precision and rounding mode, as specified in the FPCR
    Precision getPrecision() const;
    RoundingMode getRoundingMode() const;


    //
    // Accessing registers
    //

public:

    // Accesses the control register
    u32 getFPCR() const { return fpcr & FPCR::MASK; }
    void setFPCR(u32 value) { fpcr = value & FPCR::MASK; }

    // Accesses the status register
    u32 getFPSR() const { return fpsr & FPSR::MASK; }
    void setFPSR(u32 value) { fpsr = value & FPSR::MASK; }

    // Accesses the instruction address register
    u32 getFPIAR() const { return fpiar; }
    void setFPIAR(u32 value) { fpiar = value; }

    // Manipulates the exception status byte. Setting a bit updates the accrued exception byte.
    void clearExcStatusByte() { fpsr &= ~FPSR::EXC_MASK; }
    void clearExcStatusBit(u32 mask) { fpsr &= ~(mask & FPSR::EXC_MASK); }
    void setExcStatusBit(u32 mask);

    // Sets the condition code byte according to a value
    void setConditionCodes(const Float80 &value);
    void setConditionCodes(int reg) { setConditionCodes(fpr[reg & 7]); }

    /* Evaluates a floating-point condition (the 5 least significant bits of
     * the condition field are used). Conditions 16 to 31 are the signaling
     * variants of conditions 0 to 15. They set the BSUN bit if the NAN condition
     * code bit is set.
     */
    bool test(unsigned condition);

    // Sets the quotient byte (used by FMOD and FREM)
    void setQuotientByte(u8 byte);
    void setQuotientByte(u8 byte, bool sign);

    // Checks if an exception is enabled in FPCR
    bool isEnabled(u32 exceptionBit) const { return fpcr & exceptionBit; }


    //
    // Connecting to the floating-point environment
    //

public:

    /* All arithmetic is carried out by Float80. Float80 takes the rounding
     * mode and the rounding precision from the (thread-local) FloatEnv and
     * reports exceptions by raising flags in it. A Scope object connects the
     * FPU to this environment: When created, it loads the rounding mode and
     * precision from FPCR and clears the exception flags. When destroyed, it
     * transfers the raised exceptions into the exception status byte of FPSR
     * and restores the previous environment.
     *
     *     {
     *         FPU::Scope scope(fpu);
     *         fpu.fpr[0] = fpu.fpr[1] + fpu.fpr[2];
     *     }
     */
    class Scope {

        FPU &fpu;
        FloatEnv::Scope saved;

    public:

        explicit Scope(FPU &fpu);

        // Same as above, but overrides the rounding precision (FSADD, FDADD, ...)
        Scope(FPU &fpu, Precision precision);
        ~Scope();
    };

    // Transfers the exception flags of FloatEnv into FPSR and clears them
    void collectFlags();


    //
    // Executing arithmetic instructions
    //

    /* The functions in this section implement the arithmetic of the Motorola
     * FPUs on top of Float80. They have to be called inside a Scope, because
     * they use the rounding mode and precision of the floating-point
     * environment and report exceptions by raising flags in it.
     *
     * The functions differ from plain Float80 arithmetic in the way special
     * values are handled:
     *
     *  - NaN operands are resolved before the operation is carried out. A
     *    signaling NaN sets the SNAN bit in FPSR and is converted into a
     *    quiet NaN. If only one operand is a NaN, this NaN is the result. If
     *    both operands are NaNs, the destination operand is the result.
     *  - The result of an invalid operation is the default NaN, which is
     *    positive and has an all-ones mantissa (not the negative default NaN
     *    of the x87).
     *
     * Dyadic operations are computed as 'dst <op> src'. Neither the condition
     * codes nor any register is modified, unless noted otherwise.
     */

public:

    // The default NaN returned by invalid operations
    static Float80 defaultNaN() { return Float80::fromBits(0x7FFF, 0xFFFFFFFFFFFFFFFFULL); }

    // Rounds a value to the precision and rounding mode of the environment
    Float80 round(const Float80 &value);

    // Resolves NaN operands (returns nothing if there is no NaN)
    std::optional<Float80> resolveNan(const Float80 &op1, const Float80 &op2);
    std::optional<Float80> resolveNan(const Float80 &op);

    // Monadic operations
    Float80 fabs(const Float80 &value);
    Float80 fneg(const Float80 &value);
    Float80 fsqrt(const Float80 &value);
    Float80 fint(const Float80 &value);
    Float80 fintrz(const Float80 &value);
    Float80 fgetexp(const Float80 &value);
    Float80 fgetman(const Float80 &value);

    // Sets the condition codes according to a value (FTST)
    Float80 ftst(const Float80 &value);

    // Dyadic operations
    Float80 fadd(const Float80 &dst, const Float80 &src);
    Float80 fsub(const Float80 &dst, const Float80 &src);
    Float80 fmul(const Float80 &dst, const Float80 &src);
    Float80 fdiv(const Float80 &dst, const Float80 &src);
    Float80 fscal(const Float80 &dst, const Float80 &src);

    // Single precision multiplication and division (FSGLMUL, FSGLDIV)
    Float80 fsglmul(const Float80 &dst, const Float80 &src);
    Float80 fsgldiv(const Float80 &dst, const Float80 &src);

    // Remainders. These functions update the quotient byte in FPSR.
    Float80 fmod(const Float80 &dst, const Float80 &src);
    Float80 frem(const Float80 &dst, const Float80 &src);

    // Compares two values and sets the condition codes (FCMP)
    void fcmp(const Float80 &dst, const Float80 &src);


    //
    // Accessing the constant ROM
    //

public:

    // Reads a value from the constant ROM (FMOVECR). Sets INEX2 if the value is inexact.
    Float80 readCR(unsigned nr);


    //
    // Working with state frames (FSAVE, FRESTORE)
    //

public:

    // Determines the size of a state frame (varies between FPU models)
    int stateFrameSize(FpuFrameType type) const;
    int stateFrameSize(u32 fmtWord) const;

    // Determines the type a state frame
    FpuFrameType typeOfFrame(u32 fmtWord) const;

    // Computes a format word for a given frame type
    u32 computeFormatWord(FpuFrameType type) const;
};

}
