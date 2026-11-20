// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "MusashiRef.h"

namespace runner {

bool
MusashiRef::skipAccessCheck(u16 op) const
{
    // Musashi leaves the flags alone on a division by zero. The stacked SR differs (see skipResult)
    auto instr = tr().moiracpu->getInstrInfo(op).I;
    return instr == Instr::DIVU || instr == Instr::DIVS || instr == Instr::DIVL;
}

bool
MusashiRef::skipResult(const Setup &s, const Result &rRef, const Result &rMoira) const
{
    auto instr = tr().moiracpu->getInstrInfo(s.opcode).I;
    auto completed = [](const Result &r) { return r.pc > 0x1000 && r.pc <= 0x1010; };

    // Musashi leaves the flags alone on a division by zero (Moira sets them like the real CPU)
    if ((instr == Instr::DIVU || instr == Instr::DIVS || instr == Instr::DIVL) && !completed(rRef)) return true;

    // N and Z are undefined if DIVL overflows (Moira sets them like UAE, Musashi leaves them alone).
    // Musashi misses the overflow of $80000000 / -1, so the V flag of both sides is checked.
    if (instr == Instr::DIVL && ((rRef.sr | rMoira.sr) & 0x02)) return true;

    return false;
}

bool
MusashiRef::skipSetup(const Setup &s) const
{
    Instr instr = tr().moiracpu->getInstrInfo(s.opcode).I;

    // Skip some instructions that Musashi handles differently (known problems)
    bool result =
    instr == Instr::BFCHG   ||
    instr == Instr::BFCLR   ||
    instr == Instr::BFEXTS  ||
    instr == Instr::BFEXTU  ||
    instr == Instr::BFFFO   ||
    instr == Instr::BFINS   ||
    instr == Instr::BFSET   ||
    instr == Instr::BFTST;

    if (tr().cpuModel != CPUModel::M68000) result |= instr == Instr::RTE;

    // BCD: Musashi computes different results for invalid BCD digits and for NBCD of $FF with X set
    if (instr == Instr::ABCD || instr == Instr::ABCD_LOOP ||
        instr == Instr::SBCD || instr == Instr::SBCD_LOOP ||
        instr == Instr::NBCD || instr == Instr::NBCD_LOOP) result = true;

    // CHK2: Musashi computes the C flag differently, which decides whether the instruction traps (see srMask)
    if (instr == Instr::CHK2) result = true;

    // LINK A7: Musashi pushes the decremented stack pointer (UAE and Moira push the old value, except on the 68040)
    if (instr == Instr::LINK && (s.opcode & 7) == 7 && tr().cpuModel < CPUModel::M68EC040) result = true;

    // CAS2.W with an address register as Rn: Musashi sign-extends the compare operand into all of Dc
    if (s.opcode == 0x0CFC && ((s.ext1 | s.ext2) & 0x8000)) result = true;

    // PACK and UNPK -(Ax),-(Ay): Musashi swaps the bytes of the word in memory
    if ((instr == Instr::PACK || instr == Instr::UNPK) && (s.opcode & 0x08)) result = true;

    // MOVEM <list>,-(An) with An in the list (68020): Musashi stores the initial value of An
    if (tr().cpuModel >= CPUModel::M68EC020 && (s.opcode & 0xFFB8) == 0x48A0 && (s.ext1 & (0x80 >> (s.opcode & 7)))) result = true;

    // MULL with a 64-bit result in Dh = Dl (undefined): Musashi writes Dh first, Moira writes Dl first
    if (instr == Instr::MULL && (s.ext1 & 0x400) && (s.ext1 >> 12 & 7) == (s.ext1 & 7)) result = true;

    return result;
}

u32
MusashiRef::srMask(Instr instr) const
{
    // Musashi computes N, Z, V and C of CHK and CMP2 differently (Moira follows MAME and UAE)
    if (instr == Instr::CHK || instr == Instr::CMP2) return ~0x0Fu;

    return 0xFFFFFFFF;
}

}
