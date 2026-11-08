// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "../MoiraTypes.h"
#include "FpuFormats.h"
#include <optional>

namespace MOIRA_NAMESPACE {

// Supported FPU models
enum class FPUModel
{
    NONE,           // The FPU is not emulated (floating-point instructions cause a line F exception)
    BUILTIN,        // Use the FPU of the CPU (the 68040 has one, other CPUs have none)
    M68881,         // External FPU (requires a coprocessor interface, i.e., a 68020 or 68030)
    M68882          // External FPU (requires a coprocessor interface, i.e., a 68020 or 68030)
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

    // The FPU that is effectively present (resolved by Moira from the CPU model
    // and the selected FPU model, hence never BUILTIN on a CPU without an FPU)
    FPUModel model = FPUModel::NONE;

    // The floating-point environment (rounding mode, precision, exception flags)
    FpuEnv env;

    // Floating-point data registers FP0 to FP7
    MFloat80 fpr[8];

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

    // Returns the rounding precision of an instruction. FS* and FD* instructions
    // round to single or double precision. All other instructions use the FPCR.
    Precision getPrecision(Instr I) const;
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
    void setConditionCodes(const MFloat80 &value);
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

    /* All arithmetic is carried out by the floating-point environment (FpuEnv),
     * which takes the rounding mode and the rounding precision from its settings
     * and reports exceptions by raising flags. A Scope object connects the FPU
     * to this environment: When created, it loads the rounding mode and
     * precision from FPCR and clears the exception flags. When destroyed, it
     * transfers the raised exceptions into the exception status byte of FPSR
     * and restores the previous environment.
     *
     *     {
     *         FPU::Scope scope(fpu);
     *         fpu.fpr[0] = fpu.env.add(fpu.fpr[1], fpu.fpr[2]);
     *     }
     */
    class Scope {

        FPU &fpu;
        FpuEnv::Scope saved;

    public:

        explicit Scope(FPU &fpu);

        // Same as above, but overrides the rounding precision (FSADD, FDADD, ...)
        Scope(FPU &fpu, Precision precision);
        ~Scope();
    };

    // Transfers the exception flags of the environment into FPSR and clears them
    void collectFlags();


    //
    // Executing arithmetic instructions
    //

    /* The functions in this section implement the arithmetic of the Motorola
     * FPUs on top of FpuEnv. They have to be called inside a Scope, because
     * they use the rounding mode and precision of the floating-point
     * environment and report exceptions by raising flags in it.
     *
     * The functions differ from plain FpuEnv arithmetic in the way special
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
    static MFloat80 defaultNaN() { return MFloat80::defaultNaN(); }

    // Rounds a value to the precision and rounding mode of the environment
    MFloat80 round(const MFloat80 &value);

    // Resolves NaN operands (returns nothing if there is no NaN)
    std::optional<MFloat80> resolveNan(const MFloat80 &op1, const MFloat80 &op2);
    std::optional<MFloat80> resolveNan(const MFloat80 &op);

    // Prepares a register value for FMOVE FPn,<ea> in extended format. A signaling NaN is
    // reported and quieted. A denormal number is reported as tiny and rounded to the
    // precision in effect.
    MFloat80 moveOutExtended(const MFloat80 &value);

    // Monadic operations
    MFloat80 fabs(const MFloat80 &value);
    MFloat80 fneg(const MFloat80 &value);
    MFloat80 fsqrt(const MFloat80 &value);
    MFloat80 fint(const MFloat80 &value);
    MFloat80 fintrz(const MFloat80 &value);
    MFloat80 fgetexp(const MFloat80 &value);
    MFloat80 fsin(const MFloat80 &value);
    MFloat80 fcos(const MFloat80 &value);
    MFloat80 ftan(const MFloat80 &value);
    MFloat80 fasin(const MFloat80 &value);
    MFloat80 facos(const MFloat80 &value);
    MFloat80 fatan(const MFloat80 &value);
    MFloat80 fsinh(const MFloat80 &value);
    MFloat80 fcosh(const MFloat80 &value);
    MFloat80 ftanh(const MFloat80 &value);
    MFloat80 fatanh(const MFloat80 &value);
    MFloat80 fetox(const MFloat80 &value);
    MFloat80 fetoxm1(const MFloat80 &value);
    MFloat80 ftwotox(const MFloat80 &value);
    MFloat80 ftentox(const MFloat80 &value);
    MFloat80 flogn(const MFloat80 &value);
    MFloat80 flognp1(const MFloat80 &value);
    MFloat80 flog2(const MFloat80 &value);
    MFloat80 flog10(const MFloat80 &value);
    void fsincos(const MFloat80 &value, MFloat80 &sine, MFloat80 &cosine);
    MFloat80 fgetman(const MFloat80 &value);

    // Sets the condition codes according to a value (FTST)
    MFloat80 ftst(const MFloat80 &value);

    // Dyadic operations
    MFloat80 fadd(const MFloat80 &dst, const MFloat80 &src);
    MFloat80 fsub(const MFloat80 &dst, const MFloat80 &src);
    MFloat80 fmul(const MFloat80 &dst, const MFloat80 &src);
    MFloat80 fdiv(const MFloat80 &dst, const MFloat80 &src);
    MFloat80 fscal(const MFloat80 &dst, const MFloat80 &src);

    // Single precision multiplication and division (FSGLMUL, FSGLDIV)
    MFloat80 fsglmul(const MFloat80 &dst, const MFloat80 &src);
    MFloat80 fsgldiv(const MFloat80 &dst, const MFloat80 &src);

    // Remainders. These functions update the quotient byte in FPSR.
    MFloat80 fmod(const MFloat80 &dst, const MFloat80 &src);
    MFloat80 frem(const MFloat80 &dst, const MFloat80 &src);

    // Compares two values and sets the condition codes (FCMP)
    void fcmp(const MFloat80 &dst, const MFloat80 &src);


    //
    // Accessing the constant ROM
    //

public:

    // Reads a value from the constant ROM (FMOVECR). Sets INEX2 if the value is inexact.
    MFloat80 readCR(unsigned nr);


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
