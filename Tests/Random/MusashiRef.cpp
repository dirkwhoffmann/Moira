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
    // Musashi omits the write-back of NBCD if the result is zero, whereas
    // Moira writes the unchanged operand back. The final memory contents
    // are still compared.
    if ((op & 0xFFC0) == 0x4800 && (op & 0x38) != 0x08) return true;

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

    // N and Z are undefined if DIVL overflows (Moira sets them like UAE, Musashi leaves them alone)
    if (instr == Instr::DIVL && (rRef.sr & 0x02)) return true;

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

    // LINK A7: Musashi pushes the decremented stack pointer (UAE and Moira push the old value, except on the 68040)
    if (instr == Instr::LINK && (s.opcode & 7) == 7 && tr().cpuModel < CPUModel::M68EC040) result = true;

    // MULL with a 64-bit result in Dh = Dl (undefined): Musashi writes Dh first, Moira writes Dl first
    if (instr == Instr::MULL && (s.ext1 & 0x400) && (s.ext1 >> 12 & 7) == (s.ext1 & 7)) result = true;

    return result;
}

u32
MusashiRef::srMask(Instr instr) const
{
    // Musashi computes N, Z, V and C of CHK differently (Moira follows MAME and UAE)
    if (instr == Instr::CHK) return ~0x0Fu;

    return 0xFFFFFFFF;
}

}
