// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Shared.h"
#include "config.h"
#include "UAEGlue/UAECpu.h"

static void uaeWrite(int size, u32 addr, u32 value, u32 fc, u64 cycle)
{
    if (CHECK_MEM_WRITES)
        sandbox.record(size == 1 ? POKE8 : POKE16, addr, cycle, fc, size == 1 ? u8(value) : u16(value));
}

void setupUAE(moira::Model model)
{
    using uaecpu::Cpu;
    Cpu cpu =
    model == moira::Model::M68000   ? Cpu::M68000 :
    model == moira::Model::M68010   ? Cpu::M68010 :
    model == moira::Model::M68EC020 ? Cpu::M68EC020 :
    model == moira::Model::M68020   ? Cpu::M68020 :
    model == moira::Model::M68EC030 ? Cpu::M68EC030 :
    model == moira::Model::M68030   ? Cpu::M68030 :
    model == moira::Model::M68EC040 ? Cpu::M68EC040 :
    model == moira::Model::M68LC040 ? Cpu::M68LC040 : Cpu::M68040;

    uaecpu::init(cpu, refMem, uaeWrite);
}
