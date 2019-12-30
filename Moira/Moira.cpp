// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <assert.h>

#include "Moira.h"

namespace moira {

#include "MoiraInit_cpp.h"
#include "MoiraLogic_cpp.h"
#include "MoiraMemory_cpp.h"
#include "MoiraTiming_cpp.h"
#include "MoiraExec_cpp.h"
#include "StrWriter_cpp.h"
#include "MoiraDasm_cpp.h"

Moira::Moira()
{
    createJumpTables();
}

void
Moira::power()
{
    reset();
}

void
Moira::reset()
{
    clock = -40;

    for(int i = 0; i < 8; i++) {
        reg.d[i] = 0;
        reg.a[i] = 0;
    }
    reg.usp = 0;

    sr.t = 0;
    sr.s = 1;
    sr.x = 0;
    sr.n = 0;
    sr.z = 0;
    sr.v = 0;
    sr.c = 0;
    sr.ipl = 7;

    reg.ipl = 0;

    sync(16);

    // Read the initial (supervisor) stack pointer from memory
    reg.sp = reg.ssp = readOnReset(0) << 16 | readOnReset(2);

    // Read the initial program counter from memory
    reg.pc = readOnReset(4) << 16 | readOnReset(6);

    // Fill the prefetch queue
    queue.irc = readOnReset(reg.pc);
    prefetch();
}

void
Moira::execute()
{
    reg.pc += 2;
    (this->*exec[queue.ird])(queue.ird);
}

template<Size S> u32
Moira::readD(int n)
{
    return CLIP<S>(reg.d[n]);
}

template<Size S> u32
Moira::readA(int n)
{
    return CLIP<S>(reg.a[n]);
}

template<Size S> u32
Moira::readR(int n)
{
    return CLIP<S>(reg.r[n]);
}

template<Size S> void
Moira::writeD(int n, u32 v)
{
    reg.d[n] = WRITE<S>(reg.d[n], v);
}

template<Size S> void
Moira::writeA(int n, u32 v)
{
    reg.a[n] = WRITE<S>(reg.a[n], v);
}

template<Size S> void
Moira::writeR(int n, u32 v)
{
    reg.r[n] = WRITE<S>(reg.r[n], v);
}

u16
Moira::getSR()
{
    return sr.t << 15 | sr.s << 13 | sr.ipl << 8 | getCCR();
}

void
Moira::setSR(u16 value)
{
    bool t = (value >> 15) & 1;
    bool s = (value >> 13) & 1;
    u8 ipl = (value >>  8) & 7;

    sr.ipl = ipl;
    sr.t = t;

    setCCR((u8)value);
    setSupervisorMode(s);
}

u8
Moira::getCCR()
{
    return sr.c << 0 | sr.v << 1 | sr.z << 2 | sr.n << 3 | sr.x << 4;
}

void
Moira::setCCR(u8 value)
{
    sr.c = (value >> 0) & 1;
    sr.v = (value >> 1) & 1;
    sr.z = (value >> 2) & 1;
    sr.n = (value >> 3) & 1;
    sr.x = (value >> 4) & 1;
}

void
Moira::setSupervisorMode(bool enable)
{
    if (sr.s == enable) return;

    if (enable) {
        sr.s = 1;
        reg.usp = reg.a[7];
        reg.a[7] = reg.ssp;
    } else {
        sr.s = 0;
        reg.ssp = reg.a[7];
        reg.a[7] = reg.usp;
    }
}

template<bool last> void
Moira::prefetch()
{
    queue.ird = queue.irc;
    queue.irc = readM<Word,last>(reg.pc + 2);
}

template<bool last> void
Moira::fullPrefetch()
{
    queue.irc = readM<Word>(reg.pc);
    prefetch<last>();
}

template<bool skip> void
Moira::readExtensionWord()
{
    reg.pc += 2;
    if (!skip) queue.irc = readM<Word>(reg.pc);
}

void
Moira::dummyRead(u32 pc)
{
    (void)readM<Word>(pc);
}

void
Moira::jumpToVector(u8 nr)
{
    // Update the program counter
    reg.pc = readM<Long>(4 * nr);

    // Update the prefetch queue
    queue.ird = readM<Word>(reg.pc);
    sync(2);
    queue.irc = readM<Word,LAST_BUS_CYCLE>(reg.pc + 2);
}

void
Moira::pollIrq()
{
    reg.ipl = readIPL();
}

int
Moira::disassemble(u32 addr, char *str)
{
    u32 pc     = addr;
    u16 opcode = read16Dasm(pc);

    StrWriter writer(str, hex);

    (this->*dasm[opcode])(writer, pc, opcode);
    writer << Finish{};

    return pc - addr + 2;
}

}
