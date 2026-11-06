// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Shared.h"

extern "C" unsigned int m68k_read_memory_8(unsigned int addr)
{
    return get8(refMem, addr);
}

extern "C" unsigned int m68k_read_memory_16(unsigned int addr)
{
    return get16(refMem, addr);
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
    set8(refMem, addr, (u8)value);
}

extern "C" void m68k_write_memory_16(unsigned int addr, unsigned int value)
{
    if (CHECK_MEM_WRITES)
        sandbox.record(POKE16, addr, 0, musashiFC, (u16)value);
    set16(refMem, addr, (u16)value);
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

// The CPU type of the selected model (needed by the disassembler)
static unsigned musashiType = M68K_CPU_TYPE_68000;

void setupMusashi(moira::Model model)
{
    m68k_init();

    switch (model) {

        case moira::Model::M68000:    musashiType = M68K_CPU_TYPE_68000;     break;
        case moira::Model::M68010:    musashiType = M68K_CPU_TYPE_68010;     break;
        case moira::Model::M68EC020:  musashiType = M68K_CPU_TYPE_68EC020;   break;
        case moira::Model::M68020:    musashiType = M68K_CPU_TYPE_68020;     break;
        case moira::Model::M68EC030:  musashiType = M68K_CPU_TYPE_68EC030;   break;
        case moira::Model::M68030:    musashiType = M68K_CPU_TYPE_68030;     break;
        case moira::Model::M68EC040:  musashiType = M68K_CPU_TYPE_68EC040;   break;
        case moira::Model::M68LC040:  musashiType = M68K_CPU_TYPE_68LC040;   break;
        case moira::Model::M68040:    musashiType = M68K_CPU_TYPE_68040;     break;
    }
    m68k_set_cpu_type(musashiType);
}

void resetMusashi(const runner::Setup &s, bool fpu)
{
    m68k_set_reg(M68K_REG_USP, 0);
    m68k_set_reg(M68K_REG_ISP, 0);
    m68k_set_reg(M68K_REG_MSP, 0);

    m68k_pulse_reset();

    for (int i = 0; i < 8; i++) {
        m68k_set_reg((m68k_register_t)(M68K_REG_D0 + i), s.d[i]);
    }
    for (int i = 0; i < 7; i++) {
        m68k_set_reg((m68k_register_t)(M68K_REG_A0 + i), s.a[i]);
    }
    m68k_set_reg(M68K_REG_VBR, s.vbr);
    m68k_set_reg(M68K_REG_SFC, s.sfc);
    m68k_set_reg(M68K_REG_DFC, s.dfc);
    m68k_set_reg(M68K_REG_CACR, s.cacr);
    m68k_set_reg(M68K_REG_CAAR, s.caar);
    m68ki_set_ccr(s.ccr);
    s.supervisor ? m68ki_set_sm_flag(SFLAG_SET) : m68ki_set_sm_flag(SFLAG_CLEAR);

    if (fpu) {
        for (int i = 0; i < 8; i++) {
            REG_FP[i].high = s.fpr[i].signExp();
            REG_FP[i].low = s.fpr[i].mantissa();
        }
        REG_FPCR = s.fpcr;
        REG_FPSR = s.fpsr;
        REG_FPIAR = s.fpiar;
    }
}

// Also determines the address and opcode of the instruction to execute
void disassembleMusashi(const runner::Setup &s, runner::Result &r)
{
    r.oldpc = m68k_get_reg(NULL, M68K_REG_PC);
    r.opcode = get16(refMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;

    clock_t elapsed = clock();
    r.dasmCntMusashi = m68k_disassemble(r.dasmMusashi, s.pc, musashiType);
    r.elapsed[1] = clock() - elapsed;
}

void executeMusashi(runner::Result &r)
{
    clock_t elapsed = clock();
    r.cycles = m68k_execute(1);
    r.elapsed[0] = clock() - elapsed;

    r.pc = m68k_get_reg(NULL, M68K_REG_PC);
    r.sr = (u16)m68k_get_reg(NULL, M68K_REG_SR);
    r.usp = m68k_get_reg(NULL, M68K_REG_USP);
    r.isp = m68k_get_reg(NULL, M68K_REG_ISP);
    r.msp = m68k_get_reg(NULL, M68K_REG_MSP);
    r.fc = musashiFC;
    r.vbr = m68k_get_reg(NULL, M68K_REG_VBR);
    r.sfc = m68k_get_reg(NULL, M68K_REG_SFC);
    r.dfc = m68k_get_reg(NULL, M68K_REG_DFC);
    r.cacr = m68k_get_reg(NULL, M68K_REG_CACR);
    r.caar = m68k_get_reg(NULL, M68K_REG_CAAR);
    for (int i = 0; i < 8; i++) {
        r.d[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_D0 + i));
        r.a[i] = m68k_get_reg(NULL, (m68k_register_t)(M68K_REG_A0 + i));
    }
}

void recordMusashiFpu(runner::Result &r)
{
    for (int i = 0; i < 8; i++) {
        r.fpr[i] = moira::Float80::fromBits(REG_FP[i].high, REG_FP[i].low);
    }
    r.fpcr = REG_FPCR;
    r.fpsr = REG_FPSR;
    r.fpiar = REG_FPIAR;
}
