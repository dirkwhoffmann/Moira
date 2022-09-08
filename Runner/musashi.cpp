// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Testrunner.h"

extern "C" unsigned int m68k_read_memory_8(unsigned int addr)
{
    return get8(musashiMem, addr);
}

extern "C" unsigned int m68k_read_memory_16(unsigned int addr)
{
    return get16(musashiMem, addr);
}

extern "C" unsigned int m68k_read_memory_32(unsigned int addr)
{
    int hi = m68k_read_memory_16(addr);
    int lo = m68k_read_memory_16(addr + 2);
    int result = hi << 16 | lo;

    return result;
}

extern "C" unsigned int m68k_read_disassembler_16 (unsigned int addr)
{
    return m68k_read_memory_16(addr);
}

extern "C" unsigned int m68k_read_disassembler_32 (unsigned int addr)
{
    return m68k_read_memory_16(addr) << 16 | m68k_read_memory_16(addr + 2);
}

extern "C" void m68k_write_memory_8(unsigned int addr, unsigned int value)
{
    if (CHECK_MEM_WRITES)
        sandbox.record(POKE8, addr, 0, musashiFC, (u8)value);
    set8(musashiMem, addr, (u8)value);
}

extern "C" void m68k_write_memory_16(unsigned int addr, unsigned int value)
{
    if (CHECK_MEM_WRITES)
        sandbox.record(POKE16, addr, 0, musashiFC, (u16)value);
    set16(musashiMem, addr, (u16)value);
}

extern "C" void m68k_write_memory_32(unsigned int addr, unsigned int value)
{
    m68k_write_memory_16(addr, (value >> 16) & 0xFFFF);
    m68k_write_memory_16((addr + 2) & CPU_ADDRESS_MASK, value & 0xFFFF);
}

extern "C" int interrupt_handler(int irqLevel)
{
    return M68K_INT_ACK_AUTOVECTOR;
}

extern "C" void my_fc_handler(unsigned int fc)
{
    musashiFC = fc;
}

extern "C" int read_sp_on_reset(void) { return 0x2000; }
extern "C" int read_pc_on_reset(void) { return 0x1000; }
