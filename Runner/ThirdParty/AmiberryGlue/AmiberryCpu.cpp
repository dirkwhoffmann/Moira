// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "AmiberryCpu.h"

/* The wrapper is compiled together with newcpu.cpp, because it needs access
 * to some of its internal (static) functions.
 */
#include "newcpu.cpp"

namespace {

uint8_t *mem = nullptr;
amicpu::WriteHook writeHook = nullptr;
amicpu::Cpu cpuType = amicpu::Cpu::M68000;
bool addr24 = false;

inline uint32_t mask(uint32_t addr) { return addr & 0xFFFF; }

// MOVES accesses the address space selected by the DFC register
bool movesActive = false;
bool pendingByte = false;
uint32_t pendingValue = 0;
uint64_t pendingCycle = 0;
inline uint32_t fcOfWrite() { return movesActive ? (regs.dfc & 7) : regs.s ? 5 : 1; }

}

//
// Memory
//

addrbank dummy_bank;
static addrbank ram;
addrbank *mem_banks[MEMORY_BANKS];
addrbank *thread_mem_banks[MEMORY_BANKS];
uae_u8 *baseaddr[MEMORY_BANKS];

uae_u32 memory_get_byte(uaecptr a)
{
    return mem[mask(a)];
}

uae_u32 memory_get_word(uaecptr a)
{
    return mem[mask(a)] << 8 | mem[mask(a + 1)];
}

uae_u32 memory_get_long(uaecptr a)
{
    return memory_get_word(a) << 16 | memory_get_word(a + 2);
}

uae_u32 memory_get_wordi(uaecptr a) { return memory_get_word(a); }
uae_u32 memory_get_longi(uaecptr a) { return memory_get_long(a); }

void memory_put_byte(uaecptr a, uae_u32 v)
{
    if (addr24) a &= 0xFFFFFF;

    /* A misaligned word write that crosses the end of the 24-bit address
     * space arrives as two byte writes. Report it as the word write it is.
     */
    if (writeHook && addr24) {
        if (a == 0xFFFFFF) {
            pendingByte = true;
            pendingValue = v & 0xFF;
            pendingCycle = amicpu::cycles() + 2;
        } else if (pendingByte) {
            pendingByte = false;
            if (a == 0) writeHook(2, 0xFFFFFF, pendingValue << 8 | (v & 0xFF), fcOfWrite(), pendingCycle);
            else { writeHook(1, 0xFFFFFF, pendingValue, fcOfWrite(), pendingCycle); writeHook(1, a, v & 0xFF, fcOfWrite(), amicpu::cycles() + 2); }
        } else {
            writeHook(1, a, v & 0xFF, fcOfWrite(), amicpu::cycles() + 2);
        }
    } else if (writeHook) {
        writeHook(1, a, v & 0xFF, fcOfWrite(), amicpu::cycles() + 2);
    }
    mem[mask(a)] = uint8_t(v);
}

void memory_put_word(uaecptr a, uae_u32 v)
{
    if (addr24) a &= 0xFFFFFF;
    if (writeHook) writeHook(2, a, v & 0xFFFF, fcOfWrite(), amicpu::cycles() + 2);
    mem[mask(a)] = uint8_t(v >> 8);
    mem[mask(a + 1)] = uint8_t(v);
}

void memory_put_long(uaecptr a, uae_u32 v)
{
    if (addr24) a &= 0xFFFFFF;
    if (writeHook) { writeHook(2, a, v >> 16, fcOfWrite(), amicpu::cycles() + 2); writeHook(2, a + 2, v & 0xFFFF, fcOfWrite(), amicpu::cycles() + 2); }
    mem[mask(a)] = uint8_t(v >> 24);
    mem[mask(a + 1)] = uint8_t(v >> 16);
    mem[mask(a + 2)] = uint8_t(v >> 8);
    mem[mask(a + 3)] = uint8_t(v);
}

uae_u8 *memory_get_real_address(uaecptr a) { return mem + mask(a); }
addrbank *get_mem_bank_real(uaecptr) { return &ram; }

uae_u32 get_byte_debug(uaecptr a) { return memory_get_byte(a); }
uae_u32 get_word_debug(uaecptr a) { return memory_get_word(a); }
uae_u32 get_long_debug(uaecptr a) { return memory_get_long(a); }

void m68k_handle_trap(unsigned int) { }

//
// Cycles and bus accesses of the cycle-exact cores
//

static uint64_t cycleCounter = 0;
static int lastExc = 0;

// Called whenever an exception is processed
void debug_exception(int nr) { lastExc = nr; movesActive = false; }

void do_cycles_ce(int cycles) { cycleCounter += cycles; }
void do_cycles_ce020(int cycles) { cycleCounter += cycles; }
void do_cycles_slow(int cycles) { cycleCounter += cycles; }
bool is_cycle_ce(uaecptr) { return false; }

uae_u32 wait_cpu_cycle_read(uaecptr addr, int mode)
{
    cycleCounter += 2 * CYCLE_UNIT;
    switch (mode) {
        case -1: return get_long(addr);
        case -2: return get_longi(addr);
        case 1:  return get_word(addr);
        case 2:  return get_wordi(addr);
        default: return get_byte(addr);
    }
}

void wait_cpu_cycle_write(uaecptr addr, int mode, uae_u32 v)
{
    cycleCounter += 2 * CYCLE_UNIT;
    if (mode < 0) put_long(addr, v);
    else if (mode > 0) put_word(addr, v);
    else put_byte(addr, v);
}

uae_u32 wait_cpu_cycle_read_ce020(uaecptr addr, int mode) { return wait_cpu_cycle_read(addr, mode); }
void wait_cpu_cycle_write_ce020(uaecptr addr, int mode, uae_u32 v) { wait_cpu_cycle_write(addr, mode, v); }

//
// Interface
//

namespace amicpu {

void init(Cpu model, uint8_t *memory, WriteHook hook)
{
    mem = memory;

    ram.lget = [](uaecptr a) -> uae_u32 { return memory_get_long(a); };
    ram.wget = [](uaecptr a) -> uae_u32 { return memory_get_word(a); };
    ram.bget = [](uaecptr a) -> uae_u32 { return memory_get_byte(a); };
    ram.lput = [](uaecptr a, uae_u32 v) { memory_put_long(a, v); };
    ram.wput = [](uaecptr a, uae_u32 v) { memory_put_word(a, v); };
    ram.bput = [](uaecptr a, uae_u32 v) { memory_put_byte(a, v); };
    ram.lgeti = ram.lget;
    ram.wgeti = ram.wget;
    ram.xlateaddr = [](uaecptr a) -> uae_u8 * { return mem + mask(a); };
    ram.check = [](uaecptr, uae_u32) -> int { return 1; };
    ram.baseaddr = memory;
    ram.label = ram.name = "Test memory";
    ram.flags = ABFLAG_RAM;
    ram.mask = 0xFFFF;
    ram.start = 0;
    ram.allocated_size = ram.reserved_size = 0x10000;

    writeHook = hook;
    cpuType = model;

    for (auto &b : mem_banks) b = &ram;
    for (auto &b : thread_mem_banks) b = &ram;

    auto &p = changed_prefs;
    p.cpu_compatible = true;
    p.cpu_cycle_exact = model == Cpu::M68000 || model == Cpu::M68010;
    p.cpu_memory_cycle_exact = false;
    p.cachesize = 0;
    p.mmu_model = 0;
    p.fpu_model = 0;
    p.fpu_mode = 1;
    p.fpu_no_unimplemented = false;
    p.int_no_unimplemented = false;
    p.m68k_speed = 0;
    p.address_space_24 = false;

    switch (model) {

        case Cpu::M68000:    p.cpu_model = 68000; p.address_space_24 = true; break;
        case Cpu::M68010:    p.cpu_model = 68010; p.address_space_24 = true; break;
        case Cpu::M68EC020:  p.cpu_model = 68020; p.address_space_24 = true; break;
        case Cpu::M68020:    p.cpu_model = 68020; break;
        case Cpu::M68EC030:
        case Cpu::M68030:    p.cpu_model = 68030; break;
        case Cpu::M68LC040:
        case Cpu::M68EC040:  p.cpu_model = 68040; break;
        case Cpu::M68040:    p.cpu_model = 68040; p.fpu_model = 68040; break;
    }
    addr24 = p.address_space_24;
    currprefs = changed_prefs;

    init_m68k();
    build_cpufunctbl();
    set_x_funcs();
    m68k_reset();
}

void setState(const State &s)
{
    // Decide which stack is active
    bool supervisor = s.sr & 0x2000;
    bool master = (s.sr & 0x1000) && currprefs.cpu_model >= 68020;

    regs.s = supervisor;
    regs.m = master;

    for (int i = 0; i < 8; i++) regs.regs[i] = s.d[i];
    for (int i = 0; i < 7; i++) regs.regs[8 + i] = s.a[i];

    regs.usp = s.usp;
    regs.isp = s.isp;
    regs.msp = s.msp;
    regs.regs[15] = !supervisor ? s.usp : master ? s.msp : s.isp;

    regs.sr = s.sr;
    MakeFromSR();

    regs.vbr = s.vbr;
    regs.sfc = s.sfc & 7;   // SFC and DFC are 3-bit registers
    regs.dfc = s.dfc & 7;
    regs.cacr = s.cacr;
    regs.caar = s.caar;

    // Start with empty caches
    flush_cpu_caches(true);

    for (int i = 0; i < 8; i++) {
        regs.fp[i].fpx.high = s.fpse[i];
        regs.fp[i].fpx.low = s.fpman[i];
    }
    fpp_set_fpcr(s.fpcr);
    regs.fpsr = s.fpsr;
    regs.fpiar = s.fpiar;

    regs.stopped = 0;
    regs.halted = 0;
    regs.spcflags = 0;
    m68k_setpc(s.pc);
    fill_prefetch();
}

State getState()
{
    State s;

    MakeSR();

    for (int i = 0; i < 8; i++) s.d[i] = regs.regs[i];
    for (int i = 0; i < 7; i++) s.a[i] = regs.regs[8 + i];
    s.a7 = regs.regs[15];

    bool supervisor = regs.s;
    bool master = regs.m && currprefs.cpu_model >= 68020;
    s.usp = supervisor ? regs.usp : regs.regs[15];
    s.isp = supervisor && !master ? regs.regs[15] : regs.isp;
    s.msp = supervisor && master ? regs.regs[15] : regs.msp;

    s.pc = m68k_getpc();

    // Amiberry leaves the PC at the STOP instruction while the CPU
    // is stopped. On hardware, it points to the following instruction.
    if (regs.stopped) s.pc += 4;

    s.sr = regs.sr;
    s.vbr = regs.vbr;
    s.sfc = regs.sfc;
    s.dfc = regs.dfc;
    s.cacr = regs.cacr;
    s.caar = regs.caar;

    for (int i = 0; i < 8; i++) {
        s.fpse[i] = regs.fp[i].fpx.high;
        s.fpman[i] = regs.fp[i].fpx.low;
    }
    s.fpcr = regs.fpcr;
    s.fpsr = regs.fpsr;
    s.fpiar = regs.fpiar;
    return s;
}

bool step()
{
    struct regstruct *r = &regs;
    bool exit = false;

    cycleCounter = 0;
    lastExc = 0;

    movesActive = false;
    pendingByte = false;

    TRY(prb) {

        if (currprefs.cpu_model <= 68010) {

            // Cycle-exact 68000/68010 core
            r->opcode = r->ir;
            movesActive = (r->opcode & 0xFF00) == 0x0E00;
            r->instruction_pc = m68k_getpc();
            (*cpufunctbl_noret[r->opcode])(r->opcode);
            if (!regs.loop_mode) regs.ird = regs.opcode;
            wait_memory_cycles();

        } else if (currprefs.cpu_model <= 68030) {

            // Prefetch-accurate 68020/68030 core
            r->instruction_pc = m68k_getpc();
            r->opcode = regs.irc;
            movesActive = (r->opcode & 0xFF00) == 0x0E00;
            int cycles = (*cpufunctbl[r->opcode])(r->opcode);
            cycleCounter = adjust_cycles(cycles >> 16);

        } else {

            // 68040 core
            r->instruction_pc = m68k_getpc();
            r->opcode = get_iword_cache_040(0);
            movesActive = (r->opcode & 0xFF00) == 0x0E00;
            (*cpufunctbl_noret[r->opcode])(r->opcode);
        }

        // Only the exceptions raised through special flags matter here. The
        // remaining work of do_specialties() (interrupts, custom chips, reset
        // and halt handling) would wait for events that never occur.
        if (r->spcflags & SPCFLAG_TRAP) {
            unset_special(SPCFLAG_TRAP);
            Exception(3);
        }
        if (r->spcflags & SPCFLAG_DOTRACE) Exception(9);
        if (r->spcflags & SPCFLAG_TRACE) do_trace();
        regs.spcflags = 0;

    } CATCH(prb) {

        bus_error();
        regs.spcflags = 0;

    } ENDTRY

    // Report a byte write at the very end of the address space that wasn't
    // followed by a wrapped second byte
    if (pendingByte && writeHook) {
        pendingByte = false;
        writeHook(1, 0xFFFFFF, pendingValue, fcOfWrite(), pendingCycle);
    }

    return !exit && !regs.halted;
}

// A color clock (CYCLE_UNIT) is two CPU clocks
int lastException() { return lastExc; }

uint64_t cycles() { return 2 * cycleCounter / CYCLE_UNIT; }

}
