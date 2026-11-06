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

// The state after the last execution
static uaecpu::State uaeState;

void disassembleUAE(const runner::Setup &s, runner::Result &r)
{
    clock_t elapsed = clock();
    r.dasmCntUAE = uaecpu::disassemble(s.pc, r.dasmUAE, sizeof(r.dasmUAE));
    r.elapsed[1] = clock() - elapsed;
}

void resetUAE(const runner::Setup &s, bool fpu)
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

    if (fpu) {
        for (int i = 0; i < 8; i++) st.fpse[i] = s.fpr[i].signExp();
        for (int i = 0; i < 8; i++) st.fpman[i] = s.fpr[i].mantissa();
        st.fpcr = s.fpcr;
        st.fpsr = s.fpsr;
        st.fpiar = s.fpiar;
    }

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
}

void recordUAEFpu(runner::Result &r)
{
    for (int i = 0; i < 8; i++) r.fpr[i] = moira::Float80::fromBits(uaeState.fpse[i], uaeState.fpman[i]);
    r.fpcr = uaeState.fpcr;
    r.fpsr = uaeState.fpsr;
    r.fpiar = uaeState.fpiar;
}

int lastExceptionUAE()
{
    return uaecpu::lastException();
}
