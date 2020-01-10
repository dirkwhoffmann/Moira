// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "TestCPU.h"
#include "testrunner.h"

/* Signals the CPU clock to advance.
 *
 * Moira calls this function prior to each memory access and provides the
 * number of CPU cycles that have been elapsed since the previous call.
 * The demo application simply advances the CPU clock. A real-life would
 * emulate the surrounding hardware up the current CPU cycle to make sure
 * that the memory is up-to-date when the CPU accesses it.
 */
void
TestCPU::sync(int cycles)
{
    clock += cycles;
}

/* Reads a byte from memory.
 *
 * This function is called whenever the 68000 CPU reads a byte from memory.
 * It should emulate the read access including all side effects.
 */
u8
TestCPU::read8(u32 addr)
{
    if (MUSASHI) {
        return get8(moiraMem, addr);
    } else {
        return sandbox.replayPeek(PEEK8, addr, getClock());
    }
}

/* Reads a word from memory.
 *
 * This function is called whenever the 68000 CPU reads a word from memory.
 * It should emulate the read access including all side effects.
 */
u16
TestCPU::read16(u32 addr)
{
    if (MUSASHI) {
        return get16(moiraMem, addr);
    } else {
        return sandbox.replayPeek(PEEK16, addr, moiracpu->getClock());
    }
}

/* Reads a word from memory.
 *
 * This function is called by the disassembler to read a word from memory.
 * In contrast to read16, no side effects should be emulated.
 */
u16
TestCPU::read16Dasm(u32 addr)
{
    assert(MUSASHI);
    return get16(moiraMem, addr);
}

/* Reads a word from memory.
 *
 * This function is called by the reset routine to read a word from memory.
 * It's up to you if you want to emulate side effects here.
 */
u16
TestCPU::read16OnReset(u32 addr)
{
    if (MUSASHI) {
        switch (addr) {
            case 0: return 0x0000;
            case 2: return 0x2000;
            case 4: return 0x0000;
            case 6: return 0x1000;
        }
        return get16(moiraMem, addr);
    } else {
        return sandbox.replayPeek(PEEK16, addr, getClock());
    }
}

/* Writes a byte into memory.
 *
 * This function is called whenever the 68000 CPU writes a byte into memory.
 * It should emulate the write access including all side effects.
 */
void
TestCPU::write8(u32 addr, u8  val)
{
    sandbox.replayPoke(POKE8, addr, getClock(), val);
    set8(moiraMem, addr, val);
}

/* Writes a word into memory.
 *
 * This function is called whenever the 68000 CPU writes a word into memory.
 * It should emulate the write access including all side effects.
 */
void
TestCPU::write16 (u32 addr, u16 val)
{
    sandbox.replayPoke(POKE16, addr, moiracpu->getClock(), val);
    set16(moiraMem, addr, val);
}

/* DEPRECATED
 */
void
TestCPU::willPollIrq()
{
    if (!MUSASHI) {
        (void)sandbox.replayPoll(getClock());
    }
}

/* Returns the interrupt vector in IRQ_USER mode
 */
int
TestCPU::readIrqUserVector(moira::u8 level) { return 0; }

/* Breakpoint handler
 *
 * Moira calls this function to signal that a breakpoint has been reached.
 */
void
TestCPU::breakpointReached(moira::u32 addr) { }

/* Watchpoint handler
 *
 * Moira calls this function to signal that a watchpoint has been reached.
 */
void
TestCPU::watchpointReached(moira::u32 addr) { }
