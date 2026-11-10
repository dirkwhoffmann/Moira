// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "UAERef.h"

namespace runner {

// Returns the command field of an FPU instruction that operates on the registers (or -1)
static int fpuCommand(u16 op)
{
    if ((op & 0xFFC0) != 0xF200) return -1;

    u16 ext = get16(refMem, pc + 2);
    u16 cod = ext >> 13;
    return (cod == 0 || cod == 2) ? (ext & 0x7F) : -1;
}

// Returns true if the instruction is a sine, cosine, or tangent function. The reference
// reduces the argument with a pi of limited precision, so its results degrade for large
// arguments (Moira reduces exactly). FpuRefTests checks the accuracy.
static bool isTrigonometric(u16 op)
{
    int cmd = fpuCommand(op);
    return cmd == 0x0E || cmd == 0x0F || cmd == 0x1D || (cmd >= 0x30 && cmd <= 0x37);
}

// Returns true if the instruction is a transcendental function. Moira's implementation
// differs from the reference in the last bits, so these results are compared with a tolerance.
static bool isTranscendental(u16 op)
{
    switch (fpuCommand(op)) {

        case 0x02: case 0x06: case 0x08: case 0x09: case 0x0A: case 0x0C: case 0x0D:
        case 0x10: case 0x11: case 0x12: case 0x14: case 0x15: case 0x16: case 0x19:
        case 0x1C:
            return true;

        default:
            return isTrigonometric(op);
    }
}

static bool identicalFPR(const Result &rRef, const Result &rMoira)
{
    for (int i = 0; i < 8; i++) if (!rRef.fpr[i].identical(rMoira.fpr[i])) return false;
    return true;
}

// Maps a finite value to an integer such that adjacent values differ by 1
static __int128 ulpIndex(const MFloat80 &value)
{
    __int128 e = value.biasedExponent();
    u64 man = value.mantissa();
    __int128 pos = (e << 63) + (e ? __int128(man & ~(1ULL << 63)) : __int128(man >> 1));
    return value.sign() ? -pos : pos;
}

/* Test cases that are excluded from the comparison. Every entry is tagged with the reason:
 *
 *   [undefined]  The behavior is undefined or reserved. No emulator can be right or wrong.
 *   [UAE]        UAE deviates from the manual or from real hardware. Moira follows the hardware.
 *   [Moira]      Moira has a known gap (mostly the 68040 models, which run on the 68020 core).
 *   [open]       Not yet understood.
 *
 * Only the [Moira] and [open] entries are candidates for removal.
 */
bool
UAERef::skipSetup(const Setup &s) const
{
    u8 *mem = const_cast<u8 *>(s.mem);   // The reference has not been reset yet
    u16 op = s.opcode;
    Instr instr = moiracpu->getInstrInfo(op).I;


    // [undefined] The FPCR precision value 3 is reserved. UAE and Moira round tiny results differently in this mode.
    if ((op & 0xFE00) == 0xF200 && (s.fpcr & 0xC0) == 0xC0) return true;

    // [UAE] UAE doesn't implement the module instructions
    if (instr == Instr::CALLM || instr == Instr::RTM) return true;

    // [Moira] RTE processes random stack frames (format errors, bus error frames)
    if (instr == Instr::RTE && cpuModel != CPUModel::M68000 && cpuModel != CPUModel::M68010) return true;

    // [Moira] The 68010 supports the bus error stack frame (format $8), which Moira doesn't process.
    // The format word sits at offset 6 of the frame (the supervisor stack pointer is $2000).
    if (instr == Instr::RTE && cpuModel == CPUModel::M68010 && (get16(mem, 0x2006) >> 12) == 8) return true;

    // [UAE] UAE emulates FPU instructions that the 68040 traps (Moira follows real hardware)
    if ((op & 0xFFC0) == 0xF200 && cpuModel == CPUModel::M68040) {
        u16 e = get16(mem, 0x1002);
        u16 cod = e >> 13, cmd = e & 0x7F;
        if ((cod == 0 || cod == 2) && (cmd == 0x02 || cmd == 0x06 || cmd == 0x08 || cmd == 0x09 ||
            (cmd >= 0x0A && cmd <= 0x12 && cmd != 0x0B && cmd != 0x13) ||
            cmd == 0x14 || cmd == 0x15 || cmd == 0x16 || cmd == 0x19 ||
            (cmd >= 0x1C && cmd <= 0x1F) || cmd == 0x21 || cmd == 0x25 || cmd == 0x26 ||
            (cmd >= 0x30 && cmd <= 0x37))) return true;
    }

    // [undefined] FMOVECR from an undefined ROM offset returns implementation specific values
    if ((op & 0xFFC0) == 0xF200) {
        u16 e = get16(mem, 0x1002);
        u16 off = e & 0x7F;
        if ((e & 0xFC00) == 0x5C00 && off != 0x00 && !(off >= 0x0B && off <= 0x0F) && !(off >= 0x30 && off <= 0x3F)) return true;
    }

    // [undefined] FMOVE to <ea> with a non-zero k-factor field is only defined for packed formats
    if ((op & 0xFFC0) == 0xF200) {
        u16 e = get16(mem, 0x1002);
        u16 fmt = (e >> 10) & 7;
        if ((e >> 13) == 3 && (e & 0x7F) && fmt != 3 && fmt != 7) return true;
    }

    // [undefined] FMOVEM to/from control registers with an empty register list or non-zero reserved bits is undefined
    if ((op & 0xFFC0) == 0xF200) {
        u16 e = get16(mem, 0x1002);
        if (((e >> 13) == 4 || (e >> 13) == 5) && ((e & 0x1C00) == 0 || (e & 0x3FF))) return true;
    }

    // [UAE] FMOVEM <ea>,<control registers> with several registers and Dn/An as source. Moira
    // accepts this, UAE (and the 6888x manual) don't. Flagged, not mimicked.
    if ((op & 0xFFC0) == 0xF200 && ((op >> 3) & 7) <= 1) {
        u16 e = get16(mem, 0x1002);
        u16 lst = (e >> 10) & 7;
        if ((e >> 13) == 4 && lst != 1 && lst != 2 && lst != 4) return true;
        if (((e >> 13) == 4 || (e >> 13) == 5) && ((op >> 3) & 7) == 1 && lst != 1) return true;
    }

    // [UAE] Register-to-register FPU operations and FMOVECR with a non-zero <ea> field: Moira raises line F, UAE ignores the field
    if ((op & 0xFFC0) == 0xF200 && (op & 0x3F) && ((get16(mem, 0x1002) >> 13) == 0 ||
        (get16(mem, 0x1002) & 0xFC00) == 0x5C00)) return true;

    // [open] Rounded results of the 68040 FS/FD instructions differ on underflow (under investigation)
    if ((op & 0xFFC0) == 0xF200) {
        u16 e = get16(mem, 0x1002);
        if (((e >> 13) == 0 || (e >> 13) == 2) && (e & 0x40)) return true;
    }

    // [undefined] Packed decimal: random input contains invalid digits and the digit rounding on output differs
    if ((op & 0xFFC0) == 0xF200) {
        u16 e = get16(mem, 0x1002);
        if (((e >> 13) == 2 || (e >> 13) == 3) && (((e >> 10) & 7) == 3 || ((e >> 10) & 7) == 7)) return true;
    }

    // [undefined] FMOVEM data registers with an empty static list or reserved mode bits
    if ((op & 0xFFC0) == 0xF200) {
        u16 e = get16(mem, 0x1002);
        if (((e >> 13) == 6 || (e >> 13) == 7) && !(e & 0x0800) && (e & 0xFF) == 0) return true;
        if (((e >> 13) == 6 || (e >> 13) == 7) && (((e >> 11) & 3) < 2 || (e & 0x0700) || (((e >> 11) & 3) == 3 && (e & 0x8F)))) return true;
    }

    // [undefined] MULL with identical high and low registers has an undefined result
    if (instr == Instr::MULL) {
        u16 e = get16(mem, 0x1002);
        if ((e & 0x400) && ((e >> 12) & 7) == (e & 7)) return true;
    }

    // [undefined] DIVL with identical quotient and remainder registers (64 bit dividend) has an undefined result
    if (instr == Instr::DIVL) {
        u16 e = get16(mem, 0x1002);
        if ((e & 0x400) && ((e >> 12) & 7) == (e & 7)) return true;
    }

    // [undefined] CAS2 with identical compare registers is degenerate
    if (instr == Instr::CAS2) {
        u16 e1 = get16(mem, 0x1002), e2 = get16(mem, 0x1004);
        if ((e1 & 7) == (e2 & 7) || ((e1 >> 6) & 7) == ((e2 >> 6) & 7)) return true;
    }

    /* [undefined] The behavior is undefined for reserved encodings of the full extension word
     * (indexed modes of the 68020 and above). Skip the instruction if one of its
     * extension words looks like one.
     */
    auto reserved = [](u16 w) {
        return (w & 0x100) && ((w & 0x30) == 0 || (w & 0x08) || (w & 7) == 4 || ((w & 0x40) && (w & 7) >= 5));
    };

    int mode = (op >> 3) & 7, reg = op & 7;
    int dmode = (op >> 6) & 7, dreg = (op >> 9) & 7;
    bool indexed = mode == 6 || (mode == 7 && reg == 3);

    if (cpuModel < CPUModel::M68EC020) return false;

    if (op >= 0x1000 && op < 0x4000) {

        // MOVE: Walk through the extension words of the source and the destination
        int size = (op >> 12) == 1 ? 1 : (op >> 12) == 3 ? 2 : 4;
        u32 off = 0x1002;

        auto operand = [&](int m, int r) {

            if (m == 6 || (m == 7 && r == 3)) {
                u16 w = get16(mem, off);
                if (reserved(w)) return true;
                if (w & 0x100) {
                    int bd = (w & 0x30) >> 4, od = w & 3;
                    off += 2 + 2 * ((bd == 2 ? 1 : bd == 3 ? 2 : 0) + (od == 2 ? 1 : od == 3 ? 2 : 0));
                } else {
                    off += 2;
                }
            } else if (m == 5 || (m == 7 && (r == 0 || r == 2))) {
                off += 2;
            } else if (m == 7 && r == 1) {
                off += 4;
            } else if (m == 7 && r == 4) {
                off += size == 4 ? 4 : 2;
            }
            return false;
        };
        return operand(mode, reg) || operand(dmode, dreg);
    }

    // Other instructions: The extension word follows the first words
    if (indexed) {
        for (int off = 2; off <= 6; off += 2) if (reserved(get16(mem, 0x1000 + off))) return true;
    }
    return false;
}

bool
UAERef::skipAccessCheck(u16 op) const
{
    // UAE only writes back the bytes of a bit field that are affected,
    // whereas Moira writes back the whole long word. The final memory
    // contents are still compared.
    if ((op & 0xF8C0) == 0xE8C0) return true;

    // FPU opcodes may be legal for one side only (final memory contents are still compared)
    if ((op & 0xFE00) == 0xF200) return true;

    // The same applies to MMU opcodes
    if (isMmuOpcode(op)) return true;

    // N and V are undefined for CHK2, which shows up in the exception frame
    auto instr = moiracpu->getInstrInfo(op).I;
    return instr == Instr::CHK2 || instr == Instr::CMP2;
}

bool
UAERef::skipResult(const Setup &s, const Result &rRef, const Result &rMoira) const
{
    // FPU opcodes where only one side raises line F (reserved bits set in the opcode or extension word)
    return (s.opcode & 0xFE00) == 0xF200 && ((rRef.pc >> 12) == 1) != ((rMoira.pc >> 12) == 1);
}

bool
UAERef::equalPC(const Result &ref, const Result &moira) const
{
    if ((moira.pc & 1) && ref.pc == (moira.pc & ~1u)) return true;
    return ref.pc == moira.pc;
}

u32
UAERef::srMask(Instr instr) const
{
    // N and V are undefined for CMP2 and CHK2
    if (instr == Instr::CMP2 || instr == Instr::CHK2) return ~0x0Au;

    // N and V are undefined for BCD instructions (68040 differs from older models)
    if (cpuModel >= CPUModel::M68EC040 &&
        (instr == Instr::ABCD || instr == Instr::SBCD || instr == Instr::NBCD)) return ~0x0Au;

    // Z, V and C are undefined for CHK on the 68020 and above
    if (instr == Instr::CHK && cpuModel != CPUModel::M68000 && cpuModel != CPUModel::M68010) return ~0x07u;

    return 0xFFFFFFFF;
}

bool
UAERef::equalFPR(const Result &rRef, const Result &rMoira) const
{
    // The reference loses up to 13 bits for small arguments of the hyperbolic functions.
    // With directed rounding, the results can also differ by an ulp of the selected precision.
    __int128 ulpTolerance = 1 << 14;
    if ((rRef.fpcr & 0xC0) == 0x40) ulpTolerance = __int128(1) << 40;     // Single precision

    for (int i = 0; i < 8; i++) {

        if (rRef.fpr[i].identical(rMoira.fpr[i])) continue;

        // On underflow, UAE returns a zero significand with the minimum exponent of the
        // format instead of a true zero (same exemption as in FpuRefTests)
        auto &ref = rRef.fpr[i], &moira = rMoira.fpr[i];
        bool pseudoZero = ref.mantissa() == 0 && ref.biasedExponent() != 0 && ref.biasedExponent() != 0x7FFF;
        if (pseudoZero && moira.isZero() && moira.sign() == ref.sign()) continue;

        // The transcendental functions are accurate to only a few ulps
        if (isTranscendental(rRef.opcode) && !ref.isNaN() && !ref.isInfinite() && !moira.isNaN() && !moira.isInfinite()) {
            __int128 diff = ulpIndex(ref) - ulpIndex(moira);
            if ((diff < 0 ? -diff : diff) <= ulpTolerance) continue;
            if (isTrigonometric(rRef.opcode)) continue;
        }
        return false;
    }
    return true;
}

bool
UAERef::equalFPCR(const Result &ref, const Result &moira) const
{
    return (ref.fpcr & ~0xF) == (moira.fpcr & ~0xF);
}

bool
UAERef::equalFPSR(const Result &rRef, const Result &rMoira) const
{
    // UAE modifies FPSR even if the instruction ends up in a line F exception
    if ((rRef.pc >> 12) != 1) return true;

    /* An overflow always implies an inexact result. UAE only raises INEX2 if the
     * rounding discards non-zero bits, which isn't the case if the operand is a
     * huge unnormalized number whose low mantissa bits are zero after normalization.
     */
    u32 ref = rRef.fpsr, moira = rMoira.fpsr;
    if ((ref & 0x1000) && (moira & 0x1000)) { ref |= 0x0200; moira |= 0x0200; }

    // If the reference degrades, the result may even differ in sign
    if (isTrigonometric(rRef.opcode) && !identicalFPR(rRef, rMoira)) {
        ref &= ~0x0C000000; moira &= ~0x0C000000;
    }

    // UAE doesn't raise OPERR when it converts a signaling NaN to an integer
    if (ref & 0x4000) return (ref | 0x2000) == (moira | 0x2000);

    return ref == moira;
}

bool
UAERef::equalFPIAR(const Result &rRef, const Result &rMoira) const
{
    // UAE updates FPIAR even if the instruction ends up in a line F exception
    if ((rRef.pc >> 12) != 1) return true;

    // UAE skips the FPIAR update if a conversion in FMOVE <ea> raises an operand error
    if (rRef.fpsr & 0x6000) return true;

    return rRef.fpiar == rMoira.fpiar;
}

}
