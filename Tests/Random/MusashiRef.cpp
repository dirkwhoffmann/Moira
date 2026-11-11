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
    return (op & 0xFFC0) == 0x4800 && (op & 0x38) != 0x08;
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

    return result;
}

}
