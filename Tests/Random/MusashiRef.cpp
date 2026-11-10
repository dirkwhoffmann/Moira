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
MusashiRef::skipSetup(const Setup &s) const
{
    Instr instr = moiracpu->getInstrInfo(s.opcode).I;

    // Skip some instructions that Musashi handles differently (known problems)
    bool result =
    instr == Instr::ABCD    ||
    instr == Instr::SBCD    ||
    instr == Instr::NBCD    ||
    instr == Instr::BFCHG   ||
    instr == Instr::BFCLR   ||
    instr == Instr::BFEXTS  ||
    instr == Instr::BFEXTU  ||
    instr == Instr::BFFFO   ||
    instr == Instr::BFINS   ||
    instr == Instr::BFSET   ||
    instr == Instr::BFTST;

    if (cpuModel != CPUModel::M68000) result |= instr == Instr::RTE;

    return result;
}

}
