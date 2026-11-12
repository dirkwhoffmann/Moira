// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "UAECpu.h"

/* The wrapper is compiled together with newcpu.cpp, because it needs access
 * to some of its internal (static) functions.
 */
// WinUAE keeps this flag in a global, UAE in the preferences
static bool blitter_cycle_exact = false;
// newcpu.cpp resets this unconditionally, but declares it only for the threaded CPU
static int cpu_thread_tid = 0;

#include "newcpu.cpp"
#include "disasm.h"

namespace {

uint8_t *mem = nullptr;
uaecpu::WriteHook writeHook = nullptr;
uaecpu::Cpu cpuType = uaecpu::Cpu::M68000;
bool mmuExec = false;
bool addr24 = false;
uint32_t memMask = 0xFFFF;
uaecpu::AccessHook accessHook = nullptr;
bool stepping = false;           // Is an instruction being executed?
int lastExc = 0;                 // Vector of the last exception taken
std::vector<int> excLog;         // Vectors of all exceptions taken by the last step
bool seqFresh = false;           // First step after setState()?
int pinIpl = 0;                  // Interrupt priority level on the IPL pins
bool traceInStep = true;         // Process a pending trace exception in the same step?

inline uint32_t mask(uint32_t addr) { return addr & memMask; }

// The 68020 and above are not cycle exact. Their long accesses are observed as single bus accesses.
inline bool wideBus() { return cpuType != uaecpu::Cpu::M68000 && cpuType != uaecpu::Cpu::M68010; }

// MOVES accesses the address space selected by the DFC register (0x0EC0 - 0x0EFF is CAS.L)
inline bool isMoves(uint32_t op) { return (op & 0xFF00) == 0x0E00 && (op & 0xC0) != 0xC0; }
bool movesActive = false;
bool pendingByte = false;
uint32_t pendingValue = 0;
uint64_t pendingCycle = 0;
inline uint32_t fcOfWrite() { return movesActive ? (regs.dfc & 7) : regs.s ? 5 : 1; }

/* Reports a bus access to the access hook (if any). If the hook asks for a bus
 * error, the access is aborted and UAE's hardware bus error is raised. The
 * cycle-exact core checks the flag after each access and builds the frame.
 */
bool busAccess(bool write, bool fetch, uint32_t addr, int size, uint32_t value)
{
    if (!accessHook || !stepping) return false;

    /* The cycle-exact core fetches the first words of an exception handler with
     * the data access function, and PC-relative operands come from program space.
     * Both are program space accesses on a real 68000.
     */
    if (!write && !fetch && addr >= 0x400) {
        bool pcRelative = ((regs.opcode & 0x3F) == 0x3A || (regs.opcode & 0x3F) == 0x3B);
        if ((lastExc != 0 && !wideBus()) || pcRelative) fetch = true;
    }
    uint32_t fc = fetch ? (regs.s ? 6 : 2) : (regs.s ? 5 : 1);
    uaecpu::BusAccess a { write, fetch, addr & 0xFFFFFF, size, value, fc, uaecpu::cycles() };
    if (!accessHook(a)) return false;

    hardware_exception2(addr, value, !write, fetch, size);
    return true;
}

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
    if (busAccess(false, false, a, 1, 0)) return 0;
    return mem[mask(a)];
}

uae_u32 memory_get_word(uaecptr a)
{
    if (busAccess(false, false, a, 2, 0)) return 0;
    return mem[mask(a)] << 8 | mem[mask(a + 1)];
}

uae_u32 memory_get_long(uaecptr a)
{
    /* The cycle-exact core accesses the bus in words. Long accesses are free
     * peeks (e.g., the early look at an exception vector) and not observed.
     */
    if (wideBus() && busAccess(false, false, a, 4, 0)) return 0;
    return mem[mask(a)] << 24 | mem[mask(a + 1)] << 16 | mem[mask(a + 2)] << 8 | mem[mask(a + 3)];
}

uae_u32 memory_get_wordi(uaecptr a)
{
    if (busAccess(false, true, a, 2, 0)) return 0;
    return mem[mask(a)] << 8 | mem[mask(a + 1)];
}
uae_u32 memory_get_longi(uaecptr a)
{
    if (wideBus()) {
        if (busAccess(false, true, a, 4, 0)) return 0;
        return mem[mask(a)] << 24 | mem[mask(a + 1)] << 16 | mem[mask(a + 2)] << 8 | mem[mask(a + 3)];
    }
    return memory_get_long(a);
}

void memory_put_byte(uaecptr a, uae_u32 v)
{
    if (busAccess(true, false, a, 1, v & 0xFF)) return;
    if (addr24) a &= 0xFFFFFF;

    /* A misaligned word write that crosses the end of the 24-bit address
     * space arrives as two byte writes. Report it as the word write it is.
     */
    if (writeHook && addr24) {
        if (a == 0xFFFFFF) {
            pendingByte = true;
            pendingValue = v & 0xFF;
            pendingCycle = uaecpu::cycles() + 2;
        } else if (pendingByte) {
            pendingByte = false;
            if (a == 0) writeHook(2, 0xFFFFFF, pendingValue << 8 | (v & 0xFF), fcOfWrite(), pendingCycle);
            else { writeHook(1, 0xFFFFFF, pendingValue, fcOfWrite(), pendingCycle); writeHook(1, a, v & 0xFF, fcOfWrite(), uaecpu::cycles() + 2); }
        } else {
            writeHook(1, a, v & 0xFF, fcOfWrite(), uaecpu::cycles() + 2);
        }
    } else if (writeHook) {
        writeHook(1, a, v & 0xFF, fcOfWrite(), uaecpu::cycles() + 2);
    }
    mem[mask(a)] = uint8_t(v);
}

static void putWord(uaecptr a, uae_u32 v)
{
    if (addr24) a &= 0xFFFFFF;
    if (writeHook) writeHook(2, a, v & 0xFFFF, fcOfWrite(), uaecpu::cycles() + 2);
    mem[mask(a)] = uint8_t(v >> 8);
    mem[mask(a + 1)] = uint8_t(v);
}

void memory_put_word(uaecptr a, uae_u32 v)
{
    if (busAccess(true, false, a, 2, v & 0xFFFF)) return;
    putWord(a, v);
}

void memory_put_long(uaecptr a, uae_u32 v)
{
    if (wideBus() && busAccess(true, false, a, 4, v)) return;
    if (addr24) a &= 0xFFFFFF;
    if (writeHook) { writeHook(2, a, v >> 16, fcOfWrite(), uaecpu::cycles() + 2); writeHook(2, a + 2, v & 0xFFFF, fcOfWrite(), uaecpu::cycles() + 2); }
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

// Called whenever an exception is processed
void debug_exception(int nr) { lastExc = nr; excLog.push_back(nr); movesActive = false; }

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

// UAE initializes the x87 control word on x86 hosts. Moira's reference runs on
// softfloat only, so there is nothing to do (the real functions live in the
// host-specific part of Amiberry, which is not compiled).
void init_fpucw_x87(void) { }
void init_fpucw_x87_80(void) { }

namespace uaecpu {

void init(Cpu model, Fpu fpu, uint8_t *memory, WriteHook hook)
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
    p.fpu_model = fpu == Fpu::M68881 ? 68881 : fpu == Fpu::M68882 ? 68882 : fpu == Fpu::Internal ? 68040 : 0;
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
        case Cpu::M68040:    p.cpu_model = 68040; break;
    }
    addr24 = p.address_space_24;
    currprefs = changed_prefs;

    init_m68k();
    build_cpufunctbl();
    set_x_funcs();
    m68k_reset();
}

void setAccessHook(AccessHook hook) { accessHook = hook; }

void setTraceInStep(bool enable) { traceInStep = enable; }

void setMemoryMask(uint32_t m)
{
    memMask = m;
    ram.mask = m;
    ram.allocated_size = ram.reserved_size = m + 1;
}

void setPrefetch(uint16_t ir, uint16_t irc)
{
    regs.ir = ir;
    regs.ird = ir;
    regs.irc = irc;
    regs.opcode = ir;
}

bool halted() { return regs.halted; }

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

    // Forget the long word held in the 68020 prefetch pipeline. Otherwise, the
    // instruction at an unchanged PC is taken from the previous test case.
    regs.cacheholdingaddr020 = 0xffffffff;
    regs.prefetch020addr = 0xffffffff;

    for (int i = 0; i < 8; i++) {
        regs.fp[i].fpx.high = s.fpse[i];
        regs.fp[i].fpx.low = s.fpman[i];
    }
    fpp_set_fpcr(s.fpcr);
    regs.fpsr = s.fpsr;
    regs.fpiar = s.fpiar;

    if (mmuExec) {
        mmu030_reset(1);
        crp_030 = s.crp;
        srp_030 = s.srp;
        tc_030 = s.tc;
        tt0_030 = s.tt0;
        tt1_030 = s.tt1;
        mmusr_030 = uint16_t(s.mmusr);
        mmu030_decode_tc(tc_030, false);
    }

    regs.stopped = 0;
    regs.halted = 0;
    regs.spcflags = 0;
    seqFresh = true;
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

    // UAE leaves the PC at the STOP instruction while the CPU
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

    if (mmuExec) {
        s.crp = crp_030;
        s.srp = srp_030;
        s.tc = tc_030;
        s.tt0 = tt0_030;
        s.tt1 = tt1_030;
        s.mmusr = mmusr_030;
    }
    return s;
}

static uint32_t mmuTC = 0;
void enableMMU(bool enable)
{
    mmuExec = enable && cpuType == Cpu::M68030;
    changed_prefs.mmu_model = mmuExec ? 68030 : 0;
    currprefs = changed_prefs;
    build_cpufunctbl();
    set_x_funcs();
    mmu030_set_funcs();
}

void flushATC()
{
    mmu030_flush_atc_all();
}

void setMMU(uint64_t crp, uint64_t srp, uint32_t tc, uint32_t tt0, uint32_t tt1)
{
    crp_030 = crp;
    srp_030 = srp;
    tc_030 = mmuTC = tc;
    tt0_030 = tt1_030 = 0;

    // UAE decodes the transparent translation registers in PMOVE only. Run
    // PMOVE (A0),TTx with the value stored in a scratch area of the memory.
    uint8_t backup[4];
    memcpy(backup, mem + 0xFFF0, 4);
    for (int i = 0; i < 2; i++) {
        uint32_t tt = i ? tt1 : tt0;
        for (int j = 0; j < 4; j++) mem[0xFFF0 + j] = uint8_t(tt >> (24 - 8 * j));
        mmu_op30_pmove(0, 0xF010, uint16_t((i ? 0x03 : 0x02) << 10), 0xFFF0);
    }
    memcpy(mem + 0xFFF0, backup, 4);
}

bool ptest(uint32_t addr, uint8_t fc, bool write, uint32_t level, uint32_t *mmusr, uint32_t *descr)
{
    currprefs.mmu_model = 68030;
    mmu030_set_funcs();
    regs.s = fc & 4;
    tc_030 = mmuTC;
    mmu030_decode_tc(tc_030, false);

    // PTEST{R|W} #fc,(A0),#level{,A1}
    uint16_t next = uint16_t(0x8000 | level << 10 | (write ? 0 : 1) << 9 | (level ? 1 : 0) << 8 | 1 << 5 | 0x10 | (fc & 7));
    regs.regs[8 + 1] = 0xDEADBEEF;
    bool illegal = mmu_op30_ptest(0, 0xF010, next, addr);
    *mmusr = mmusr_030;
    *descr = regs.regs[8 + 1];

    tc_030 = 0;
    mmu030_decode_tc(tc_030, false);
    currprefs.mmu_model = mmuExec ? 68030 : 0;
    set_x_funcs();
    mmu030_set_funcs();
    return !illegal;
}

bool translate(uint32_t addr, bool super, bool data, bool write, uint32_t *physical)
{
    bool success = false;

    // The MMU functions are only active for the 68030 MMU model
    currprefs.mmu_model = 68030;
    mmu030_set_funcs();
    regs.s = super;
    tc_030 = mmuTC;
    mmu030_decode_tc(tc_030, false);

    // Perform a real memory access. Unlike mmu030_translate, which always walks
    // the tables as if a read access was performed, this updates the descriptors
    // exactly as a read or write access does.
    uint32_t fc = (super ? 4 : 0) | (data ? 1 : 2);
    static uint8_t backup[0x10000];
    memcpy(backup, mem, sizeof backup);

    TRY(prb) {
        if (write) mmu030_put_byte(addr, 0, fc); else mmu030_get_byte(addr, fc);
        *physical = mmu030_translate(addr, super, data, write);
        if (write) mem[mask(*physical)] = backup[mask(*physical)];
        success = true;
    } CATCH(prb) {
        success = false;
    } ENDTRY

    // Switch the MMU off again, so it doesn't affect instruction execution
    tc_030 = 0;
    mmu030_decode_tc(tc_030, false);
    currprefs.mmu_model = mmuExec ? 68030 : 0;
    set_x_funcs();
    mmu030_set_funcs();
    return success;
}

// The 68040 MMU is only active while one of the functions below runs
void setMMU040(uint32_t urp, uint32_t srp, uint32_t tc, uint32_t itt0, uint32_t itt1, uint32_t dtt0, uint32_t dtt1)
{
    int model = currprefs.mmu_model;
    bool ec = currprefs.mmu_ec;
    currprefs.mmu_model = 68040;
    currprefs.mmu_ec = false;

    // Physical memory access used by the table walk
    x_phys_get_byte = memory_get_byte;
    x_phys_get_word = memory_get_word;
    x_phys_get_long = memory_get_long;
    x_phys_put_byte = memory_put_byte;
    x_phys_put_word = memory_put_word;
    x_phys_put_long = memory_put_long;

    regs.urp = urp;
    regs.srp = srp;
    regs.itt0 = itt0;
    regs.itt1 = itt1;
    regs.dtt0 = dtt0;
    regs.dtt1 = dtt1;
    mmu_tt_modified();
    mmu_set_tc(uint16_t(tc));
    regs.mmu_enabled = false;

    currprefs.mmu_model = model;
    currprefs.mmu_ec = ec;
}

void endMMU040()
{
    regs.itt0 = regs.itt1 = regs.dtt0 = regs.dtt1 = 0;
    mmu_tt_modified();
    mmu_set_tc(0);
}

void flushATC040()
{
    mmu_flush_atc_all(true);
}

bool translate040(uint32_t addr, bool super, bool data, bool write, uint32_t *physical)
{
    bool success = false;

    int model = currprefs.mmu_model;
    currprefs.mmu_model = 68040;
    regs.s = super;
    regs.mmu_enabled = true;

    // Same sequence as in mmu_get_byte and mmu_put_byte
    TRY(prb) {
        uint32_t phys = addr;
        int res = write ? mmu_match_ttr_write(addr, super, data, 0, sz_byte)
                        : mmu_match_ttr(addr, super, data);
        if (res == TTR_NO_MATCH) phys = mmu_translate(addr, 0, super, data, write, sz_byte);
        *physical = phys;
        success = true;
    } CATCH(prb) {
        success = false;
    } ENDTRY

    regs.mmu_enabled = false;
    currprefs.mmu_model = model;
    return success;
}

uint32_t ptest040(uint32_t addr, uint8_t dfc, bool write)
{
    int model = currprefs.mmu_model;
    currprefs.mmu_model = 68040;
    regs.dfc = dfc & 7;
    regs.regs[8] = addr;
    mmu_op_real(write ? 0xF548 : 0xF568, 0);
    currprefs.mmu_model = model;
    return regs.mmusr;
}

int disassemble(uint32_t addr, char *out, size_t size)
{
    // The disassembler is configured once. No instruction words, effective
    // addresses or operand values. Lowercase and '$' for hex numbers.
    static bool configured = false;
    if (!configured) {
        disasm_flags = DISASM_FLAG_LC_MNEMO | DISASM_FLAG_LC_REG | DISASM_FLAG_LC_SIZE | DISASM_FLAG_LC_HEX;
        strcpy(disasm_hexprefix, "$");
        disasm_init();
        configured = true;
    }

    char buf[2048];
    uaecptr next = addr;
    m68k_disasm_2(buf, sizeof buf, addr, nullptr, 0, &next, 1, nullptr, nullptr, 0xffffffff, 0);

    // Remove the leading address (8 hex digits and a blank) and trailing white space
    char *p = buf;
    if (strlen(p) > 9 && p[8] == ' ') p += 9;
    size_t n = strlen(p);
    while (n && isspace((unsigned char)p[n - 1])) p[--n] = 0;

    snprintf(out, size, "%s", p);
    return int(next - addr);
}

// Executes the instruction at the program counter
static void executeInstruction()
{
    struct regstruct *r = &regs;

    if (currprefs.cpu_model <= 68010) {

        // Cycle-exact 68000/68010 core
        r->opcode = r->ir;
        movesActive = isMoves(r->opcode);
        r->instruction_pc = m68k_getpc();
        (*cpufunctbl_noret[r->opcode])(r->opcode);
        if (!regs.loop_mode) regs.ird = regs.opcode;
        wait_memory_cycles();

    } else if (currprefs.cpu_model <= 68030) {

        // Prefetch-accurate 68020/68030 core
        r->instruction_pc = m68k_getpc();
        r->opcode = regs.irc;
        movesActive = isMoves(r->opcode);
        int cycles = (*cpufunctbl[r->opcode])(r->opcode);
        cycleCounter = adjust_cycles(cycles >> 16);

    } else {

        // 68040 core
        r->instruction_pc = m68k_getpc();
        r->opcode = get_iword_cache_040(0);
        movesActive = isMoves(r->opcode);
        (*cpufunctbl_noret[r->opcode])(r->opcode);
    }
}

bool step()
{
    struct regstruct *r = &regs;
    bool exit = false;

    cycleCounter = 0;
    lastExc = 0;

    movesActive = false;
    pendingByte = false;
    stepping = true;

    TRY(prb) {

        executeInstruction();

        // Only the exceptions raised through special flags matter here. The
        // remaining work of do_specialties() (interrupts, custom chips, reset
        // and halt handling) would wait for events that never occur.
        if (r->spcflags & SPCFLAG_TRAP) {
            unset_special(SPCFLAG_TRAP);
            Exception(3);
        }
        if (traceInStep) {
            if (r->spcflags & SPCFLAG_DOTRACE) Exception(9);
            if (r->spcflags & SPCFLAG_TRACE) do_trace();
        }
        regs.spcflags = 0;

    } CATCH(prb) {

        bus_error();
        regs.spcflags = 0;

    } ENDTRY

    stepping = false;

    // Report a byte write at the very end of the address space that wasn't
    // followed by a wrapped second byte
    if (pendingByte && writeHook) {
        pendingByte = false;
        writeHook(1, 0xFFFFFF, pendingValue, fcOfWrite(), pendingCycle);
    }

    return !exit && !regs.halted;
}



void setIPL(int level)
{
    level &= 7;

    // UAE detects the edge of level 7 by comparing with the level seen at the previous check. If the pins fall
    // and rise again between two checks, the falling edge would go unnoticed, so it is recorded here.
    if (pinIpl == 7 && level != 7) regs.lastipl = level;
    pinIpl = level;
}

bool stopped() { return regs.stopped != 0; }

const std::vector<int> &exceptions() { return excLog; }

bool stepSeq()
{
    struct regstruct *r = &regs;

    cycleCounter = 0;
    lastExc = 0;
    excLog.clear();
    movesActive = false;
    pendingByte = false;
    stepping = true;

    // Arm the trace logic for the first instruction (the main loop does this after each instruction)
    if (seqFresh) {
        seqFresh = false;
        if (regs.t1) { set_special(SPCFLAG_TRACE); do_trace(); }
    }

    TRY(prb) {

        // Execute an instruction (unless the CPU is stopped)
        if (!regs.stopped && !regs.halted) executeInstruction();

        // Process the special flags in the order of UAE's do_specialties()
        if (r->spcflags & SPCFLAG_DOTRACE) Exception(9);
        if (r->spcflags & SPCFLAG_TRAP) {
            unset_special(SPCFLAG_TRAP);
            Exception(3);
        }
        if (r->spcflags & SPCFLAG_TRACE) do_trace();

        // Take an interrupt (level 7 is edge triggered)
        int intr = pinIpl;
        if (!regs.halted && (intr > regs.intmask || (intr == 7 && intr > regs.lastipl))) {
            Exception(24 + intr);
        }
        regs.lastipl = intr;

        // The trace state survives, all other flags are consumed
        regs.spcflags &= (SPCFLAG_TRACE | SPCFLAG_DOTRACE);

    } CATCH(prb) {

        bus_error();
        regs.spcflags = 0;

    } ENDTRY

    stepping = false;
    return !regs.halted;
}

// A color clock (CYCLE_UNIT) is two CPU clocks
int lastException() { return lastExc; }

uint64_t cycles() { return 2 * cycleCounter / CYCLE_UNIT; }

}
