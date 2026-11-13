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

static void uaeWrite(int size, uint32_t addr, uint32_t value, uint32_t fc, uint64_t cycle)
{
    if (CHECK_MEM_WRITES)
        sandbox.record(size == 1 ? POKE8 : POKE16, addr, cycle, fc, size == 1 ? u8(value) : u16(value));
}

void setupUAE(moira::CPUModel model, FPUType fpuModel)
{
    using uaecpu::Cpu;
    using uaecpu::Fpu;
    Cpu cpu =
    model == moira::CPUModel::M68000   ? Cpu::M68000 :
    model == moira::CPUModel::M68010   ? Cpu::M68010 :
    model == moira::CPUModel::M68EC020 ? Cpu::M68EC020 :
    model == moira::CPUModel::M68020   ? Cpu::M68020 :
    model == moira::CPUModel::M68EC030 ? Cpu::M68EC030 :
    model == moira::CPUModel::M68030   ? Cpu::M68030 :
    model == moira::CPUModel::M68EC040 ? Cpu::M68EC040 :
    model == moira::CPUModel::M68LC040 ? Cpu::M68LC040 : Cpu::M68040;

    Fpu fpu =
    fpuModel == FPUType::M68881   ? Fpu::M68881 :
    fpuModel == FPUType::M68882   ? Fpu::M68882 :
    fpuModel == FPUType::BUILTIN ? Fpu::Internal : Fpu::None;

    uaecpu::init(cpu, fpu, refMem, uaeWrite);
}

// The state after the last execution
static uaecpu::State uaeState;

void disassembleUAE(const runner::Setup &s, runner::Result &r)
{
    clock_t elapsed = clock();
    r.dasmCntUAE = uaecpu::disassemble(s.pc, r.dasmUAE, sizeof(r.dasmUAE));
    r.elapsed[1] = clock() - elapsed;
}

void resetUAE(const runner::Setup &s)
{
    uaecpu::State st;

    for (int i = 0; i < 8; i++) st.d[i] = s.d[i];
    for (int i = 0; i < 7; i++) st.a[i] = s.a[i];
    st.usp = 0;
    st.isp = 0x2000;
    st.msp = 0;
    st.pc = s.pc;
    st.sr = (s.supervisor ? 0x2000 : 0) | 0x0700 | s.ccr;
    st.vbr = s.vbr;
    st.sfc = s.sfc;
    st.dfc = s.dfc;
    st.cacr = s.cacr;
    st.caar = s.caar;

    for (int i = 0; i < 8; i++) st.fpse[i] = s.fpr[i].signExp();
    for (int i = 0; i < 8; i++) st.fpman[i] = s.fpr[i].mantissa();
    st.fpcr = s.fpcr;
    st.fpsr = s.fpsr;
    st.fpiar = s.fpiar;
    st.crp = s.crp; st.srp = s.srp; st.tc = s.tc; st.tt0 = s.tt0; st.tt1 = s.tt1; st.mmusr = s.mmusr;

    uaecpu::setState(st);
}

void executeUAE(runner::Result &r)
{
    clock_t elapsed = clock();
    uaecpu::step();
    r.elapsed[0] = clock() - elapsed;
    r.cycles = int(uaecpu::cycles());

    uaeState = uaecpu::getState();
    r.pc = uaeState.pc;
    r.sr = uaeState.sr;
    r.usp = uaeState.usp;
    r.isp = uaeState.isp;
    r.msp = uaeState.msp;
    r.fc = 0;
    r.vbr = uaeState.vbr;
    r.sfc = uaeState.sfc & 7;
    r.dfc = uaeState.dfc & 7;
    r.cacr = uaeState.cacr;
    r.caar = uaeState.caar;
    for (int i = 0; i < 8; i++) r.d[i] = uaeState.d[i];
    for (int i = 0; i < 7; i++) r.a[i] = uaeState.a[i];
    r.a[7] = uaeState.a7;

    for (int i = 0; i < 8; i++) r.fpr[i] = moira::MFloat80::fromBits(uaeState.fpse[i], uaeState.fpman[i]);
    r.fpcr = uaeState.fpcr;
    r.fpsr = uaeState.fpsr;
    r.fpiar = uaeState.fpiar;
    r.crp = uaeState.crp; r.srp = uaeState.srp; r.tc = uaeState.tc;
    r.tt0 = uaeState.tt0; r.tt1 = uaeState.tt1; r.mmusr = uaeState.mmusr;
}

int lastExceptionUAE()
{
    return uaecpu::lastException();
}

void setMMUUAE(u64 crp, u64 srp, u32 tc, u32 tt0, u32 tt1)
{
    uaecpu::setMMU(crp, srp, tc, tt0, tt1);
}

bool translateUAE(u32 addr, bool super, bool data, bool write, u32 *physical)
{
    return uaecpu::translate(addr, super, data, write, physical);
}

bool ptestUAE(u32 addr, u8 fc, bool write, u32 level, u32 *mmusr, u32 *descr)
{
    return uaecpu::ptest(addr, fc, write, level, mmusr, descr);
}

void flushATCUAE()
{
    uaecpu::flushATC();
}

void setMMU040UAE(u32 urp, u32 srp, u32 tc, u32 itt0, u32 itt1, u32 dtt0, u32 dtt1)
{
    uaecpu::setMMU040(urp, srp, tc, itt0, itt1, dtt0, dtt1);
}

void endMMU040UAE() { uaecpu::endMMU040(); }
void flushATC040UAE() { uaecpu::flushATC040(); }

bool translate040UAE(u32 addr, bool super, bool data, bool write, u32 *physical)
{
    return uaecpu::translate040(addr, super, data, write, physical);
}

u32 ptest040UAE(u32 addr, u8 dfc, bool write)
{
    return uaecpu::ptest040(addr, dfc, write);
}

void enableMMUUAE(bool enable)
{
    uaecpu::enableMMU(enable);
}
