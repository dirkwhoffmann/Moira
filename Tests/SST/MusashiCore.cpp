// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "MusashiCore.h"

extern "C" {
#include "m68k.h"
}

namespace sstrunner {

static Memory *musashiMem = nullptr;

extern "C" {

unsigned int m68k_read_memory_8(unsigned int addr) { return musashiMem->peek(addr); }
unsigned int m68k_read_memory_16(unsigned int addr) { return musashiMem->peek16(addr); }
unsigned int m68k_read_memory_32(unsigned int addr) { return musashiMem->peek16(addr) << 16 | musashiMem->peek16(addr + 2); }
unsigned int m68k_read_disassembler_16(unsigned int addr) { return musashiMem->peek16(addr); }
unsigned int m68k_read_disassembler_32(unsigned int addr) { return m68k_read_memory_32(addr); }
void m68k_write_memory_8(unsigned int addr, unsigned int value) { musashiMem->poke(addr, u8(value)); }
void m68k_write_memory_16(unsigned int addr, unsigned int value) { musashiMem->poke(addr, u8(value >> 8)); musashiMem->poke(addr + 1, u8(value)); }
void m68k_write_memory_32(unsigned int addr, unsigned int value) { m68k_write_memory_16(addr, value >> 16); m68k_write_memory_16(addr + 2, value & 0xFFFF); }
int interrupt_handler(int) { return M68K_INT_ACK_AUTOVECTOR; }
void my_fc_handler(unsigned int) { }
int read_sp_on_reset(void) { return 0x2000; }
int read_pc_on_reset(void) { return 0x1000; }

}

MusashiCore::MusashiCore(Memory &m) : mem(m)
{
    musashiMem = &mem;
    m68k_init();
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
}

void MusashiCore::setModel(CPUModel m)
{
    m68k_set_cpu_type(m == CPUModel::M68010   ? M68K_CPU_TYPE_68010 :
                      m == CPUModel::M68EC020 ? M68K_CPU_TYPE_68EC020 :
                      m == CPUModel::M68020   ? M68K_CPU_TYPE_68020 : M68K_CPU_TYPE_68000);
}

void MusashiCore::load(const sst::State &s)
{
    // The queue holds the opcode and the next extension word (the PC points behind them)
    u32 pc = (s.pc - 4) & ADDR_MASK;
    mem.poke(pc, u8(s.prefetch[0] >> 8)); mem.poke(pc + 1, u8(s.prefetch[0]));
    mem.poke(pc + 2, u8(s.prefetch[1] >> 8)); mem.poke(pc + 3, u8(s.prefetch[1]));

    m68k_set_reg(M68K_REG_USP, 0);
    m68k_set_reg(M68K_REG_ISP, 0);
    m68k_pulse_reset();
    m68k_set_reg(M68K_REG_SR, s.sr);
    m68k_set_reg(M68K_REG_USP, s.usp);
    m68k_set_reg(M68K_REG_ISP, s.ssp);
    for (int i = 0; i < 8; i++) m68k_set_reg((m68k_register_t)(M68K_REG_D0 + i), s.d[i]);
    for (int i = 0; i < 7; i++) m68k_set_reg((m68k_register_t)(M68K_REG_A0 + i), s.a[i]);
    m68k_set_reg(M68K_REG_PC, pc);
}

int MusashiCore::execute()
{
    return m68k_execute(1);
}

mame::State MusashiCore::state() const
{
    mame::State m;
    for (int i = 0; i < 8; i++) m.d[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + i));
    for (int i = 0; i < 7; i++) m.a[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_A0 + i));
    m.usp = m68k_get_reg(NULL, M68K_REG_USP);
    m.ssp = m68k_get_reg(NULL, M68K_REG_ISP);
    m.sr = u16(m68k_get_reg(NULL, M68K_REG_SR));
    m.pc = m68k_get_reg(NULL, M68K_REG_PC) & ADDR_MASK;
    return m;
}

}
