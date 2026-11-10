// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* Exception differential test
 *
 * Runs the same instruction on MAME's microcoded 68000 (the reference) and on
 * Moira, and compares how both CPUs handle exceptions that occur in the middle
 * of an instruction. The instructions and the initial states are taken from the
 * SingleStepTests. The final states are compared: registers, status register
 * (including the condition codes), cycle count, bus cycles, and all memory that
 * has been written (including the stack frame).
 *
 * Bus errors:
 *   The n-th bus access of an instruction fails with a bus error. This is
 *   repeated for every bus access.
 *
 * Address errors:
 *   A bus access cannot be turned into an address error from the outside,
 *   because both CPUs decide on their own whether an address is odd. Hence, the
 *   initial state is perturbed and MAME decides whether the instruction ends in
 *   an address error. Those cases are replayed on Moira. Perturbations:
 *     A7^1          odd stack pointer
 *     An^1, Dn^1    odd address or data register
 *     IRC^1         odd extension word
 *     EXTn^1        the next four extension words (e.g. abs.l behind immediate data)
 *     (A7+n)^1      odd return address on the stack (RTS, RTR, RTE)
 *     A7^1,An^1     odd stack pointer together with an odd address register
 *   MAME logs (and sometimes performs) the aborted access of an address error,
 *   which never reaches a real bus. This access is excluded from the comparison.
 *
 * Double faults:
 *   If a fault occurs while an exception frame is stacked, the CPU halts. This
 *   happens, for example, if an instruction loads an odd value into A7. For bus
 *   errors, a second pass additionally makes the SSP odd (user mode tests only),
 *   such that every bus error double-faults. Moira has to halt as well and the
 *   registers, memory and bus cycles have to agree. The cycle count and the PC of
 *   a halted CPU are not compared, and MAME's endless repetition of the last bus
 *   cycle is cut off.
 *
 * Reference CPUs:
 *   --reference mame   MAME's microcoded 68000 (default)
 *   --reference uae    WinUAE's cycle-exact 68000. Where it disagrees with MAME, either one
 *                      has a bug or the behavior is not well defined. These spots are worth
 *                      a closer look. Note that Moira (MAME mode) follows MAME.
 *   --ignore ccr,cycles  Skips the comparison of condition codes (final and stacked) and of cycle
 *                      counts and bus timing. Helps to see the other differences between two
 *                      references that differ in these. "nbit" skips the I/N bit (bit 3) in
 *                      the first word of bus and address error frames, which UAE sets by
 *                      rules that Moira (UAE mode) does not model yet.
 *   --check            Runs the reference alone against the SingleStep expectations (no faults).
 *                      MAME has to reach 100%. This is also the calibration for other references.
 *
 * The SingleStep suite contains thousands of tests per instruction. --dedupe <n>
 * runs at most n cases per instruction form, access index and kind, which keeps
 * the coverage but drops the data variety. --coverage lists instruction forms
 * with a memory operand that never produced a case.
 */

#include "Moira.h"
#include "MameCpu.h"
#include "UAECpu.h"
#include "SSTFile.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <map>
#include <memory>
#include <set>

using namespace moira;

namespace {

constexpr u32 ADDR_MASK = 0xFFFFFF;
u32 srMask = 0xA71F;                  // Compared bits of the status register
u16 codeMask = 0xFFFF;                // Compared bits of the first word of bus and address error frames
bool ignoreCycles = false;            // Skip the comparison of cycle counts and bus cycle timing?
constexpr u32 HANDLER = 0x600000;       // Bus error handler
bool installVectors = true;             // Install the vectors of the bus error and address error handler?

// A flat 16 MB memory that can be reset quickly
struct Memory {

    std::vector<u8> ram = std::vector<u8>(1 << 24);
    std::vector<u32> touched;

    u8 peek(u32 a) const { return ram[a & ADDR_MASK]; }
    u16 peek16(u32 a) const { return u16(peek(a) << 8 | peek(a + 1)); }
    void poke(u32 a, u8 v) { ram[a & ADDR_MASK] = v; touched.push_back(a & ADDR_MASK); }
    void clear() { for (auto a : touched) ram[a] = 0; touched.clear(); }

    void load(const sst::State &s)
    {
        clear();
        for (auto &[addr, val] : s.ram) poke(addr, val);

        if (!installVectors) return;

        // Install the bus error vector
        for (int i = 0; i < 4; i++) poke(8 + i, u8(HANDLER >> (24 - 8 * i)));

        // Install the address error vector
        for (int i = 0; i < 4; i++) poke(12 + i, u8((HANDLER + 0x100) >> (24 - 8 * i)));
    }
};

// A bus access as seen by the memory
struct Access {

    i64 cycle;
    bool write;
    u32 addr;           // Word address
    u16 mask;           // Selected byte lanes
    u16 data;
    int fc;
};

// Decides whether a bus access fails
struct Fault {

    int index = -1;     // Index of the failing access (-1 = none)
    int count = 0;      // Number of accesses so far

    bool hit() { return count++ == index; }
};

//
// Moira
//

class TestCPU : public Moira {

public:

    Memory mem;
    Fault fault;
    mutable std::vector<Access> log;
    bool logging = false;

    TestCPU() { setModel(CPUModel::M68000); }

    void record(bool write, u32 addr, int size, u16 data) const
    {
        u16 mask = size == 2 ? 0xFFFF : (addr & 1) ? 0x00FF : 0xFF00;
        u16 word = size == 2 ? data : (addr & 1) ? data & 0xFF : u16(data << 8);
        if (logging) log.push_back({ getClock() - 2, write, addr & ADDR_MASK & ~1u, mask, word, readFC() });
    }

    u8 read8(u32 addr) const override
    {
        auto self = const_cast<TestCPU *>(this);
        u8 v = mem.peek(addr);
        if (logging && self->fault.hit()) { record(false, addr, 1, 0); throw BusError(addr & ADDR_MASK); }
        record(false, addr, 1, v);
        return v;
    }
    u16 read16(u32 addr) const override
    {
        auto self = const_cast<TestCPU *>(this);
        u16 v = mem.peek16(addr);
        if (logging && self->fault.hit()) { record(false, addr, 2, 0); throw BusError(addr & ADDR_MASK); }
        record(false, addr, 2, v);
        return v;
    }
    u32 read32(u32 addr) const override { return u32(read16(addr) << 16 | read16(addr + 2)); }

    void write8(u32 addr, u8 val) const override
    {
        auto self = const_cast<TestCPU *>(this);
        if (logging && self->fault.hit()) { record(true, addr, 1, val); throw BusError(addr & ADDR_MASK); }
        record(true, addr, 1, val);
        self->mem.poke(addr, val);
    }
    void write16(u32 addr, u16 val) const override
    {
        auto self = const_cast<TestCPU *>(this);
        if (logging && self->fault.hit()) { record(true, addr, 2, val); throw BusError(addr & ADDR_MASK); }
        record(true, addr, 2, val);
        self->mem.poke(addr, u8(val >> 8));
        self->mem.poke(addr + 1, u8(val));
    }
    void write32(u32 addr, u32 val) const override { write16(addr, u16(val >> 16)); write16(addr + 2, u16(val)); }

    void load(const sst::State &s)
    {
        logging = false;
        mem.load(s);
        reset();
        setSR(u16(s.sr));
        setUSP(s.usp);
        setISP(s.ssp);
        for (int i = 0; i < 8; i++) setD(i, s.d[i]);
        for (int i = 0; i < 7; i++) setA(i, s.a[i]);
        setPC(s.pc - 4);
        setPC0(s.pc - 4);
        setIRD(u16(s.prefetch[0]));
        setIRC(u16(s.prefetch[1]));
        setClock(0);
        log.clear();
        fault = Fault();
    }
};

//
// Reference CPUs
//

// Common interface of the reference CPUs (the results are reported in MAME's types)
struct RefCore {

    virtual ~RefCore() = default;

    virtual void load(const sst::State &s) = 0;                         // Sets the registers (the memory is loaded already)
    virtual int execute() = 0;                                          // Runs one instruction and returns the elapsed cycles
    virtual mame::State state() const = 0;
    virtual const std::vector<mame::BusCycle> &log() const = 0;
    virtual bool halted() const = 0;
};

// MAME's microcoded 68000
struct MameCore : RefCore {

    mame::Cpu cpu;

    MameCore(Memory &mem, Fault &fault)
    {
        cpu.read = [&mem](u32 addr, u16) { return mem.peek16(addr); };
        cpu.write = [&mem](u32 addr, u16 val, u16 mask) {
            if (mask & 0xFF00) mem.poke(addr, u8(val >> 8));
            if (mask & 0x00FF) mem.poke(addr + 1, u8(val));
        };
        cpu.busError = [&fault](u32, bool, int) { return fault.hit(); };
    }

    void load(const sst::State &s) override
    {
        mame::State m;
        for (int i = 0; i < 8; i++) m.d[i] = s.d[i];
        for (int i = 0; i < 7; i++) m.a[i] = s.a[i];
        m.usp = s.usp;
        m.ssp = s.ssp;
        m.sr = u16(s.sr);
        m.pc = (s.pc - 4) & ADDR_MASK;
        m.ird = u16(s.prefetch[0]);
        m.irc = u16(s.prefetch[1]);
        cpu.load(m);
    }

    int execute() override { return cpu.execute(); }
    mame::State state() const override { return cpu.state(); }
    const std::vector<mame::BusCycle> &log() const override { return cpu.log(); }
    bool halted() const override { return cpu.halted(); }
};

// The 68000 core of UAE (WinUAE, cycle exact). The core is a singleton.
struct UaeCore : RefCore {

    static inline UaeCore *self = nullptr;

    Memory &mem;
    Fault &fault;
    std::vector<mame::BusCycle> cycles;
    bool didHalt = false;
    uint64_t cycleBase = 0;

    UaeCore(Memory &m, Fault &f) : mem(m), fault(f)
    {
        self = this;
        uaecpu::init(uaecpu::Cpu::M68000, uaecpu::Fpu::None, mem.ram.data(), [](int size, uint32_t addr, uint32_t, uint32_t, uint64_t) {

            // Writes go directly into the memory. Remember them, so that the memory can be reset.
            for (int i = 0; i < size; i++) self->mem.touched.push_back((addr + i) & ADDR_MASK);
        });
        uaecpu::setMemoryMask(ADDR_MASK);
        uaecpu::setTraceInStep(false);
        uaecpu::setAccessHook([](const uaecpu::BusAccess &a) { return self->access(a); });
    }

    // Is called for each bus access of the CPU
    bool access(const uaecpu::BusAccess &a)
    {
        u16 mask = a.size == 2 ? 0xFFFF : (a.addr & 1) ? 0x00FF : 0xFF00;
        u32 word = a.addr & ~1u;
        u16 data = a.write ? (a.size == 2 ? u16(a.value) : (a.addr & 1) ? u16(a.value & 0xFF) : u16(a.value << 8)) : mem.peek16(word);
        cycles.push_back({ int(a.cycle - cycleBase), a.write, word, data, mask, int(a.fc) });
        return fault.hit();
    }

    void load(const sst::State &s) override
    {
        uaecpu::State u;
        for (int i = 0; i < 8; i++) u.d[i] = s.d[i];
        for (int i = 0; i < 7; i++) u.a[i] = s.a[i];
        u.usp = s.usp;
        u.isp = s.ssp;
        u.sr = u16(s.sr);
        u.pc = (s.pc - 4) & ADDR_MASK;

        // Loading the state fetches the prefetch queue. This is not part of the instruction.
        uaecpu::setAccessHook(nullptr);
        uaecpu::setState(u);
        uaecpu::setPrefetch(u16(s.prefetch[0]), u16(s.prefetch[1]));
        uaecpu::setAccessHook([](const uaecpu::BusAccess &a) { return self->access(a); });
        cycles.clear();
        didHalt = false;
    }

    int execute() override
    {
        didHalt = !uaecpu::step();
        return int(uaecpu::cycles());
    }

    mame::State state() const override
    {
        auto u = uaecpu::getState();
        mame::State m;
        for (int i = 0; i < 8; i++) m.d[i] = u.d[i];
        for (int i = 0; i < 7; i++) m.a[i] = u.a[i];
        m.usp = u.usp;
        m.ssp = u.isp;
        m.sr = u.sr;
        m.pc = u.pc & ADDR_MASK;
        return m;
    }

    const std::vector<mame::BusCycle> &log() const override { return cycles; }
    bool halted() const override { return didHalt; }
};

struct Reference {

    Memory mem;
    Fault fault;
    std::unique_ptr<RefCore> core;
    RefCore &cpu;

    Reference(bool uae) : core(uae ? (RefCore *)new UaeCore(mem, fault) : (RefCore *)new MameCore(mem, fault)), cpu(*core) { }

    void load(const sst::State &s)
    {
        mem.load(s);
        cpu.load(s);
        fault = Fault();
    }
};

//
// Comparing
//

bool expectsAddressError(const sst::Test &t)
{
    u32 vec = 0; int bytes = 0;

    for (auto &[addr, val] : t.initial.ram) {
        if (addr >= 12 && addr <= 15) { vec |= u32(val) << (8 * (15 - addr)); bytes++; }
    }
    return bytes == 4 && ((t.final.pc - 4) & ADDR_MASK) == (vec & ADDR_MASK);
}

const char *kind(const mame::BusCycle &c)
{
    if (c.fc == 2 || c.fc == 6) return "fetch";
    return c.write ? "write" : "read";
}

struct Settings {

    std::string dir;
    std::vector<std::string> only;
    long limit = 0;                 // Tests per file
    int verbose = 0;                // Failures to print per file
    int dedupe = 0;                 // Maximum number of cases per instruction form, access, and kind (0 = all)
    bool coverage = false;          // Report instruction forms without any case?
#ifdef EXC_DEFAULT_REFERENCE_UAE
    bool uae = true;                // Use UAE instead of MAME as reference?
#else
    bool uae = false;               // Use UAE instead of MAME as reference?
#endif
    bool check = false;             // Only check the reference against the SingleStep expectations?
    std::string tsv;                // Optional result table
};

// Instruction form: operands with register numbers removed (A7 is kept), e.g. "ADD.l (d8, A7, Xn), Dn"
std::string formOf(const std::string &name)
{
    std::string r = name.substr(name.find(' ') + 1);
    size_t sp = r.find_last_of(' ');
    if (sp != std::string::npos && r.size() - sp == 5) r.resize(sp);        // Opcode
    std::string out;
    for (size_t i = 0; i < r.size(); i++) {
        if ((r[i] == 'D' || r[i] == 'A') && i + 1 < r.size() && isdigit((unsigned char)r[i + 1]) && !(r[i] == 'A' && r[i + 1] == '7')) {
            out += r[i]; out += 'n'; i++;
        } else out += r[i];
    }
    return out;
}

struct Counter {

    long cases = 0, ok = 0;
    std::map<std::string, long> signatures;
};

std::string words(const Memory &m, u32 sp, int n)
{
    char buf[16]; std::string s;
    for (int i = 0; i < n; i++) { snprintf(buf, sizeof buf, "%04x ", m.peek16(sp + 2 * i)); s += buf; }
    return s;
}


//
// Checking a reference CPU against the SingleStep expectations
//

struct Expected { int cycle; bool write; u32 addr; u16 mask; u16 data; int fc; bool ae; };

std::vector<Expected> expectedBus(const sst::Test &t)
{
    std::vector<Expected> result;
    int cycle = 0;

    for (auto &b : t.bus) {
        if (b.type != sst::Bus::IDLE) {
            u16 mask = b.uds && b.lds ? 0xFFFF : b.uds ? 0xFF00 : 0x00FF;
            bool write = b.type == sst::Bus::WRITE || b.type == sst::Bus::TAS || b.type == sst::Bus::WRITE_AE;
            bool ae = b.type == sst::Bus::READ_AE || b.type == sst::Bus::WRITE_AE;
            result.push_back({ cycle, write, b.addr & ~1u, mask, u16(b.data & mask), int(b.fc), ae });
        }
        cycle += b.cycles;
    }
    return result;
}

// Returns an empty string if the bus cycles match. A reference that doesn't log the aborted access
// of an address error (UAE) is compared without it.
std::string compareBus(const sst::Test &t, const std::vector<mame::BusCycle> &got, bool dropAborted)
{
    auto exp = expectedBus(t);
    if (dropAborted) exp.erase(std::remove_if(exp.begin(), exp.end(), [](auto &e) { return e.ae; }), exp.end());
    if (exp.size() != got.size()) return "count";

    for (size_t i = 0; i < exp.size(); i++) {
        auto &e = exp[i]; auto &g = got[i];
        std::string d;
        if (!ignoreCycles && e.cycle != g.cycle) d += "cycle ";
        if (e.write != g.write) d += "rw ";
        if (e.addr != g.addr) d += "addr ";
        if (e.mask != g.mask) d += "mask ";
        if (!e.ae && (e.data & e.mask) != (g.data & g.mask)) d += "data ";
        if (e.fc != g.fc) d += "fc ";
        if (!d.empty()) { d.pop_back(); return "bus#" + std::to_string(i) + "(" + d + ")"; }
    }
    return "";
}

// Runs the reference without faults and compares the result with the SingleStep expectations
int checkReference(const Settings &opt, const std::vector<std::string> &names, Reference &ref)
{
    printf("Reference CPU (%s), run against the SingleStepTests 68000 suite\n\n", opt.uae ? "UAE" : "MAME");
    printf("%-16s %9s %9s %9s %9s\n", "Test", "Count", "State", "Cycles", "Bus");

    long total = 0, okState = 0, okCycles = 0, okBus = 0;
    std::map<std::string, long> signatures;

    for (auto &name : names) {

        sst::File file;
        if (!file.open(opt.dir + "/" + name)) { printf("Cannot read %s\n", name.c_str()); return 1; }

        long n = 0, st = 0, cy = 0, bu = 0;
        int shown = 0;
        sst::Test t;

        while (file.next(t)) {

            if (opt.limit && n >= opt.limit) break;

            ref.load(t.initial);
            int cycles = ref.cpu.execute();
            auto r = ref.cpu.state();
            auto &f = t.final;

            // STOP does not prefetch, so no offset applies afterwards
            u32 pcOffset = (t.initial.prefetch[0] == 0x4E72 && (t.initial.sr & 0x2000)) ? 0 : 4;

            std::string why;
            auto fail = [&](const char *w) { if (!why.empty()) why += " "; why += w; };
            bool state = true;
            for (int i = 0; i < 8; i++) if (r.d[i] != f.d[i]) { state = false; fail("D"); break; }
            for (int i = 0; i < 7; i++) if (r.a[i] != f.a[i]) { state = false; fail("A"); if (getenv("EXC_REGS")) printf("      A%d: initial %08x exp %08x got %08x\n", i, t.initial.a[i], f.a[i], r.a[i]); break; }
            if (r.usp != f.usp) { state = false; fail("USP"); }
            if (r.ssp != f.ssp) { state = false; fail("SSP"); }
            if ((r.sr & srMask) != (f.sr & srMask)) { state = false; fail("SR"); }
            if (((r.pc + pcOffset) & ADDR_MASK) != (f.pc & ADDR_MASK)) { state = false; fail("PC"); }
            for (auto &[addr, val] : f.ram) if (ref.mem.peek(addr) != val) { state = false; fail("mem"); break; }
            bool cyc = cycles == int(t.cycles);
            if (!cyc && !ignoreCycles) fail("cycles");
            auto busDiff = compareBus(t, ref.cpu.log(), opt.uae);
            if (!busDiff.empty()) fail(busDiff.c_str());

            n++; st += state; cy += cyc; bu += busDiff.empty();
            if (!why.empty() && getenv("EXC_REGS") && shown < opt.verbose && (!getenv("EXC_SIG") || why.find(getenv("EXC_SIG")) != std::string::npos)) {
                printf("      ssp: initial %08x exp %08x got %08x; pc: exp %08x got %08x (+%u); cycles exp %u got %d\n", t.initial.ssp, f.ssp, r.ssp, f.pc, r.pc, pcOffset, t.cycles, cycles);
                for (auto &b : ref.cpu.log()) printf("      got c%-3d %s %06x m%04x d%04x fc%d\n", b.cycle, b.write ? "W" : "R", b.addr, b.mask, b.data, b.fc);
                int c = 0;
                for (auto &b : t.bus) { printf("      exp c%-3d type %d dur %d addr %06x data %04x fc %u\n", c, int(b.type), b.cycles, b.addr, b.data, b.fc); c += b.cycles; }
            }
            if (!why.empty()) {
                signatures[(expectsAddressError(t) ? "[AE] " : "") + why]++;
                if (shown < opt.verbose && (!getenv("EXC_SIG") || why.find(getenv("EXC_SIG")) != std::string::npos)) { shown++; printf("    %s: %s%s\n", t.name.c_str(), expectsAddressError(t) ? "[AE] " : "", why.c_str()); }
            }
        }
        printf("%-16s %9ld %8.2f%% %8.2f%% %8.2f%%\n", name.substr(0, name.size() - 9).c_str(), n,
               n ? 100.0 * st / n : 100.0, n ? 100.0 * cy / n : 100.0, n ? 100.0 * bu / n : 100.0);
        fflush(stdout);
        total += n; okState += st; okCycles += cy; okBus += bu;
    }

    printf("\n%-16s %9ld %8.2f%% %8.2f%% %8.2f%%\n\n", "TOTAL", total,
           total ? 100.0 * okState / total : 100.0, total ? 100.0 * okCycles / total : 100.0, total ? 100.0 * okBus / total : 100.0);

    std::vector<std::pair<long, std::string>> sorted;
    for (auto &[sig, n] : signatures) sorted.push_back({ n, sig });
    std::sort(sorted.rbegin(), sorted.rend());
    printf("Most frequent differences:\n");
    for (size_t i = 0; i < sorted.size() && i < 25; i++) printf("  %8ld  %s\n", sorted[i].first, sorted[i].second.c_str());

    return total == okState && total == okCycles && total == okBus ? 0 : 1;
}

}

int main(int argc, char **argv)
{
    Settings opt;
    {
        std::string file = __FILE__;
        opt.dir = file.substr(0, file.find_last_of("/\\")) + "/../ThirdParty/SingleStepTests/m68000/v1";
    }
    if (const char *env = getenv("SST_DIR")) opt.dir = env;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        bool more = i + 1 < argc;
        if (a == "--dir" && more) opt.dir = argv[++i];
        else if (a == "--only" && more) opt.only.push_back(argv[++i]);
        else if (a == "--limit" && more) opt.limit = atol(argv[++i]);
        else if (a == "--tsv" && more) opt.tsv = argv[++i];
        else if (a == "--dedupe" && more) opt.dedupe = atoi(argv[++i]);
        else if (a == "--coverage") opt.coverage = true;
        else if (a == "--check") opt.check = true;
        else if (a == "--ignore" && more) {
            std::string list = "," + std::string(argv[++i]) + ",";
            if (list.find(",ccr,") != std::string::npos) srMask = 0xA700;
            if (list.find(",cycles,") != std::string::npos) ignoreCycles = true;
            if (list.find(",nbit,") != std::string::npos) codeMask = 0xFFF7;
        }
        else if (a == "--reference" && more) { std::string r = argv[++i]; if (r != "mame" && r != "uae") { printf("Unknown reference %s\n", r.c_str()); return 1; } opt.uae = r == "uae"; }
        else if (a == "-v" && more) opt.verbose = atoi(argv[++i]);
        else {
            printf("Usage: %s [--dir <folder>] [--only <text>]... [--limit <n>] [-v <n>] [--dedupe <n>] [--coverage] [--reference mame|uae] [--check] [--ignore ccr,cycles,nbit] [--tsv <file>]\n", argv[0]);
            return 1;
        }
    }

    std::vector<std::string> names;
    if (DIR *d = opendir(opt.dir.c_str())) {
        while (auto *e = readdir(d)) {
            std::string n = e->d_name;
            if (n.size() > 9 && n.compare(n.size() - 9, 9, ".json.bin") == 0 &&
                (opt.only.empty() || std::any_of(opt.only.begin(), opt.only.end(), [&](auto &s) { return n.find(s) != std::string::npos; })))
                names.push_back(n);
        }
        closedir(d);
    }
    std::sort(names.begin(), names.end());
    if (names.empty()) { printf("No test files found in %s\n", opt.dir.c_str()); return 1; }

    printf("Exception differential test: Moira (%s mode) vs. %s\n\n", moira::compat == moira::Compatibility::UAE ? "UAE" : "MAME", opt.uae ? "UAE (WinUAE, cycle exact 68000)" : "MAME's microcoded 68000");
    printf("%-16s %8s %8s %8s\n", "Instruction", "Cases", "OK", "OK%");

    FILE *tsv = opt.tsv.empty() ? nullptr : fopen(opt.tsv.c_str(), "w");
    TestCPU moira;
    Reference ref(opt.uae);
    if (opt.check) { installVectors = false; return checkReference(opt, names, ref); }
    std::map<std::string, Counter> byKind;
    std::map<std::string, long> signatures;
    long total = 0, totalOk = 0, skippedStacking = 0;
    std::map<std::string, int> keyCount;
    std::set<std::string> allForms, hitForms;

    for (auto &name : names) {

        sst::File file;
        if (!file.open(opt.dir + "/" + name)) { printf("Cannot read %s\n", name.c_str()); return 1; }

        Counter c;
        long failedReads = 0, failedWrites = 0;
        int shown = 0;
        sst::Test t;

        auto runCase = [&](const sst::State &init, int k, int accesses, const mame::BusCycle &pk, const char *kd, bool isFetch, bool bus, u16 plainSR, const std::string &label) {

            // Count distinct cases and skip duplicates
            std::string key = formOf(t.name) + label.substr(0, label.find(']') + 1) + " #" + std::to_string(k) + " " + kd;
            keyCount[key]++;
            hitForms.insert(formOf(t.name));
            if (opt.dedupe && keyCount[key] > opt.dedupe) return;

            // Reference
            ref.load(init);
            ref.fault.index = k;
            int refCycles = ref.cpu.execute();
            bool halted = ref.cpu.halted();
            auto rs = ref.cpu.state();

            // Moira
            moira.load(init);
            moira.fault.index = k;
            moira.logging = true;
            moira.execute();
            moira.logging = false;

            // Compare
            std::string diff;
            auto add = [&](const std::string &s) { diff += (diff.empty() ? "" : " ") + s; };
            if (halted) kd = "halt";
            if (moira.isHalted() != halted) add("halt");

            // The registers of a halted UAE are not compared (e.g., it keeps the trace bit and ends up with a different SSP)
            bool regs = !(halted && opt.uae);
            for (int i = 0; regs && i < 8; i++) if (moira.getD(i) != rs.d[i]) {
                add("D" + std::to_string(i));
                if (getenv("EXC_REGS")) printf("      D%d: initial %08x exp %08x got %08x\n", i, init.d[i], rs.d[i], moira.getD(i));
                break;
            }
            for (int i = 0; regs && i < 7; i++) if (moira.getA(i) != rs.a[i]) {
                add("A" + std::to_string(i));
                if (getenv("EXC_REGS")) printf("      A%d: initial %08x exp %08x got %08x\n", i, init.a[i], rs.a[i], moira.getA(i));
                break;
            }
            if (regs && moira.getUSP() != rs.usp) { add("USP"); if (getenv("EXC_REGS")) printf("      USP: initial %08x exp %08x got %08x\n", init.usp, rs.usp, moira.getUSP()); }
            if (regs && moira.getISP() != rs.ssp) { add("SSP"); if (getenv("EXC_REGS")) printf("      SSP: initial %08x exp %08x got %08x\n", init.ssp, rs.ssp, moira.getISP()); }
            if (regs && (moira.getSR() & srMask) != (rs.sr & srMask)) { add("SR"); if (getenv("EXC_REGS")) printf("      SR: initial %04x exp %04x got %04x\n", init.sr, rs.sr, moira.getSR()); }
            // The PC of a halted CPU depends on the microcode state (not compared)
            if (!halted && (moira.getPC() & ADDR_MASK) != (rs.pc & ADDR_MASK)) { add("PC"); if (getenv("EXC_REGS")) printf("      PC: initial %08x exp %08x got %08x\n", init.pc, rs.pc, moira.getPC()); }
            if (!halted && !ignoreCycles && i64(moira.getClock()) != refCycles) { add("cycles"); if (getenv("EXC_REGS")) printf("      cycles: exp %lld got %lld\n", (long long)refCycles, (long long)moira.getClock()); }

            // The read of a TAS is followed by a write in MAME, which a real bus error would prevent
            // Address of the aborted access of an address error (MAME may still perform it)
            u32 aborted = ~0u;
            if (!bus) for (size_t i = 1; i < ref.cpu.log().size(); i++) {
                auto &e = ref.cpu.log()[i];
                u32 sp = rs.ssp & ADDR_MASK;
                if (e.write && e.fc == 5 && e.addr >= sp && e.addr < sp + 14) { aborted = ref.cpu.log()[i - 1].addr; break; }
            }

            // A halted MAME repeats the last bus cycle forever (the aborted access of the double fault)
            if (halted) for (size_t i = 0; i + 1 < ref.cpu.log().size(); i++) {
                auto &a = ref.cpu.log()[i], &b = ref.cpu.log()[i + 1];
                if (a.write && b.write && a.addr == b.addr && a.fc == b.fc && a.data == b.data) { aborted = a.addr; break; }
            }

            // MAME may perform writes that a real bus would abort and that Moira does not log
            std::set<u32> extraWrites;
            if (halted) for (auto &e : ref.cpu.log()) {
                if (!e.write) continue;
                bool found = std::any_of(moira.log.begin(), moira.log.end(), [&](auto &g) { return g.write && g.addr == e.addr && g.data == e.data; });
                if (!found) extraWrites.insert(e.addr);
            }

            bool tasRead = bus && t.name.find(" TAS ") != std::string::npos && !isFetch && !pk.write;

            // Memory: compare everything that has been written by either side
            std::set<u32> addrs(moira.mem.touched.begin(), moira.mem.touched.end());
            addrs.insert(ref.mem.touched.begin(), ref.mem.touched.end());
            std::set<int> badWords;
            for (auto a : addrs) {
                if (tasRead && (a & ~1u) == pk.addr) continue;
                if (extraWrites.count(a & ~1u)) continue;
                if ((a & ~1u) == aborted && aborted != ~0u && (!bus || halted)) continue;       // MAME may perform the aborted access    // MAME still completes the write half
                if (a == ((rs.ssp & ADDR_MASK) + 2)) continue;      // Undefined upper byte of the fault address
                if (a == ((rs.ssp & ADDR_MASK) + 1) && !halted && ((moira.mem.peek(a) ^ ref.mem.peek(a)) & codeMask & 0xFF) == 0) continue;   // Ignored bits of the code word
                if (a == ((rs.ssp & ADDR_MASK) + 9) && !(srMask & 0x1F)) {      // Condition codes in the stacked SR
                    if (((moira.mem.peek(a) ^ ref.mem.peek(a)) & ~0x1F) == 0) continue;
                }
                if (moira.mem.peek(a) != ref.mem.peek(a)) {
                    u32 sp = rs.ssp & ADDR_MASK;
                    if (a >= sp && a < sp + 14) badWords.insert(int((a - sp) / 2)); else { badWords.insert(-1); if (getenv("EXC_REGS")) printf("      mem %06x: exp %02x got %02x (ssp %06x)\n", a, ref.mem.peek(a), moira.mem.peek(a), sp); }
                }
            }
            static const char *fields[] = { "frame.code", "frame.addr", "frame.addr", "frame.ird", "frame.sr", "frame.pc", "frame.pc" };
            std::set<std::string> bad;
            for (int w : badWords) bad.insert(w < 0 ? "mem" : fields[w]);
            for (auto &b : bad) add(b);

            // Bus cycles
            auto mlog = ref.cpu.log();

            // MAME logs the aborted access of an address error, which never reaches a real bus. Drop it.
            if (!bus) {
                u32 sp = rs.ssp & ADDR_MASK;
                for (size_t i = 1; i < mlog.size(); i++) {
                    if (mlog[i].write && mlog[i].fc == 5 && mlog[i].addr >= sp && mlog[i].addr < sp + 14) {
                        if (mlog.size() == moira.log.size() + 1) mlog.erase(mlog.begin() + (i - 1));
                        break;
                    }
                }
            }

            // A halted MAME repeats the last bus cycle forever. Cut the loop.
            if (halted) {
                for (size_t i = 0; i + 1 < mlog.size(); i++) {
                    auto &a = mlog[i], &b = mlog[i + 1];
                    if (a.write && b.write && a.addr == b.addr && a.fc == b.fc && a.data == b.data) { mlog.resize(i); break; }
                }
            }
            if (halted && !bus && mlog.size() == moira.log.size() + 1) mlog.pop_back();
            if (tasRead) {}
            else if (mlog.size() != moira.log.size()) add("bus.count");
            else {
                for (size_t i = 0; i < mlog.size(); i++) {
                    auto &e = mlog[i]; auto &g = moira.log[i];
                    std::string d;
                    if (getenv("EXC_DEBUG") && i == 0) printf("      first bus cycle: mame %lld moira %lld; total mame %d moira %lld\n", (long long)e.cycle, (long long)g.cycle, refCycles, (long long)moira.getClock());
                    if (!ignoreCycles && e.cycle != g.cycle) d += "cycle ";
                    if (e.write != g.write) d += "rw ";
                    if (e.addr != g.addr) d += "addr ";
                    if (e.mask != g.mask) d += "mask ";
                    if (e.fc != g.fc) d += "fc ";
                    u16 dm = 0xFFFF;
                    if (e.write && e.fc == 5 && e.addr == (((rs.ssp & ADDR_MASK) + 2) & ~1u)) dm = 0x00FF;        // Undefined upper byte of the fault address
                    if (e.write && e.fc == 5 && e.addr == (((rs.ssp & ADDR_MASK) + 8) & ~1u)) dm = ~srMask & 0x1F ? 0xFFE0 : 0xFFFF;   // Condition codes
                    if (e.write && !halted && e.fc == 5 && e.addr == ((rs.ssp & ADDR_MASK) & ~1u)) dm = codeMask;   // Ignored bits of the code word
                    if (e.write && (e.data & e.mask & dm) != (g.data & g.mask & dm)) d += "data ";
                    if (!d.empty()) { d.pop_back(); add("bus#" + std::to_string(i) + "(" + d + ")"); break; }
                }
            }

            if (tsv) fprintf(tsv, "%s\t%d\t%d\t%s\t%s\t%s\n", t.name.c_str(), k, accesses, kd, diff.empty() ? "ok" : "FAIL", diff.c_str());
            std::string kkey = std::string(kd);
            c.cases++;
            byKind[kkey].cases++;
            if (diff.empty()) { c.ok++; byKind[kkey].ok++; return; }

            (isFetch ? failedReads : (pk.write ? failedWrites : failedReads))++;
            signatures[diff]++;
            byKind[kkey].signatures[diff]++;

            if (shown < opt.verbose) {
                shown++;
                if (getenv("EXC_DEBUG")) {
                    if (!bus) for (auto &e : ref.cpu.log()) printf("      raw   c%-3lld %s %06x m%04x d%04x fc%d\n", (long long)e.cycle, e.write ? "W" : "R", e.addr, e.mask, e.data, e.fc);
                    for (auto &e : mlog) printf("      mame  c%-3lld %s %06x m%04x d%04x fc%d\n", (long long)e.cycle, e.write ? "W" : "R", e.addr, e.mask, e.data, e.fc);
                    for (auto &g : moira.log) printf("      moira c%-3lld %s %06x m%04x d%04x fc%d\n", (long long)g.cycle, g.write ? "W" : "R", g.addr, g.mask, g.data, g.fc);
                }
                printf("    %s%s  fault #%d (%s %06x fc %d): %s\n", t.name.c_str(), label.c_str(), k, kd, pk.addr, pk.fc, diff.c_str());
                if (opt.verbose > 1) {
                    printf("        initial prefetch: %04x %04x  pc %06x  sr %04x ssp %08x usp %08x\n", init.prefetch[0], init.prefetch[1], init.pc, init.sr, init.ssp, init.usp);
                    printf("        exp frame: %s\n", words(ref.mem, rs.ssp, 7).c_str());
                    printf("        got frame: %s\n", words(moira.mem, rs.ssp, 7).c_str());
                }
            }

        };

        for (long n = 0; file.next(t); n++) {

            if (opt.limit && n >= opt.limit) break;
            if (expectsAddressError(t)) continue;
            allForms.insert(formOf(t.name));

            {

                // Perturb the initial state such that an instruction ends in an address error
                struct Perturbation { std::string name; sst::State state; };
                std::vector<Perturbation> variants;

                {
                    sst::State s = t.initial;
                    bool supervisor = s.sr & 0x2000;
                    u32 &sp = supervisor ? s.ssp : s.usp;
                    if (!(sp & 1)) { sp ^= 1; variants.push_back({ " [A7^1]", s }); }
                }
                for (int i = 0; i < 7; i++) {
                    if (t.initial.a[i] & 1) continue;
                    sst::State s = t.initial; s.a[i] ^= 1;
                    variants.push_back({ " [A" + std::to_string(i) + "^1]", s });
                }
                for (int i = 0; i < 8; i++) {
                    sst::State s = t.initial; s.d[i] ^= 1;
                    variants.push_back({ " [D" + std::to_string(i) + "^1]", s });
                }
                auto flipByte = [](sst::State &st, u32 addr) {
                    uint8_t v = 0;
                    for (auto &[a, b] : st.ram) if (a == addr) v = b;
                    st.ram.push_back({ addr, uint8_t(v ^ 1) });
                };
                bool sup = t.initial.sr & 0x2000;
                u32 sp0 = sup ? t.initial.ssp : t.initial.usp;
                {

                    // Odd absolute address (abs.w/abs.l, also behind immediate data), odd branch displacement
                    for (u32 i = 0; i < 4; i++) {
                        sst::State s = t.initial; flipByte(s, t.initial.pc + 1 + 2 * i);
                        variants.push_back({ " [EXT" + std::to_string(i) + "^1]", s });
                    }
                }
                for (u32 off : { 1u, 3u, 5u, 7u }) {

                    // Odd return address on the stack (RTS, RTR, RTE)
                    sst::State s = t.initial; flipByte(s, sp0 + off);
                    variants.push_back({ " [(A7+" + std::to_string(off) + ")^1]", s });
                }
                if (!(sp0 & 1)) for (int i = 0; i < 7; i++) {

                    // Odd stack pointer together with an odd address register
                    if (t.initial.a[i] & 1) continue;
                    sst::State s = t.initial; s.a[i] ^= 1;
                    (sup ? s.ssp : s.usp) ^= 1;
                    variants.push_back({ " [A7^1,A" + std::to_string(i) + "^1]", s });
                }
                {
                    sst::State s = t.initial; s.prefetch[1] ^= 1;
                    variants.push_back({ " [IRC^1]", s });
                }

                for (auto &v : variants) {

                    ref.load(v.state);
                    ref.cpu.execute();
                    if (ref.cpu.halted()) {
                        mame::BusCycle none { 0, false, 0, 0, 0, 0 };
                        runCase(v.state, -1, 0, none, "halt", false, false, 0, v.name);
                        continue;
                    }
                    auto log = ref.cpu.log();

                    // Did the instruction end in an address error? (vector 3 is read)
                    bool ae = std::any_of(log.begin(), log.end(), [](auto &c) { return !c.write && c.fc == 5 && c.addr == 12; });
                    if (!ae) continue;

                    // Classify the failing access by the function code and the R/W bit of the stacked frame
                    auto rs = ref.cpu.state();
                    u16 code = ref.mem.peek16(rs.ssp);
                    const char *kd = ((code & 7) == 2 || (code & 7) == 6) ? "ae-fetch" : (code & 0x10) ? "ae-read" : "ae-write";
                    bool isFetch = !strcmp(kd, "ae-fetch");
                    mame::BusCycle none { 0, false, 0, 0, 0, 0 };
                    runCase(v.state, -1, 0, none, kd, isFetch, false, ref.cpu.state().sr, v.name);
                }
            }

            // The second pass makes the SSP odd (user mode only), so that stacking an exception frame double-faults
            for (int pass = 0; pass < 2; pass++) {

                sst::State st = t.initial;
                if (pass == 1) {
                    if ((st.sr & 0x2000) || (st.ssp & 1)) break;
                    st.ssp ^= 1;
                } else if (st.ssp & 1) continue;

                // Run the instruction without faults to find out how many bus accesses there are
                ref.load(st);
                ref.cpu.execute();
                if (ref.cpu.halted()) {
                    // The instruction halts the CPU by itself (e.g., an address error with an odd SSP)
                    mame::BusCycle none { 0, false, 0, 0, 0, 0 };
                    runCase(st, -1, 0, none, "halt", false, true, 0, pass ? " [SSP^1]" : "");
                    continue;
                }
                int accesses = int(ref.cpu.log().size());
                auto plain = ref.cpu.log();
                u16 plainSR = ref.cpu.state().sr;

                // Find the exception vector fetch of instructions that raise an exception themselves
                int vectorFetch = -1;
                for (int k = 0; k < accesses && vectorFetch < 0; k++) if (!plain[k].write && plain[k].addr < 0x100 && plain[k].fc == 5) vectorFetch = k;

                for (int k = 0; k < accesses; k++) {

                    // UAE and MAME disagree on the frames of bus errors that hit the stacking, the vector fetch, or the first fetches of the handler of a group 1 or 2 exception
                    if (opt.uae && vectorFetch >= 3 && k >= vectorFetch - 3) { skippedStacking++; continue; }

                    const char *kd = kind(plain[k]);
                    bool isFetch = !strcmp(kd, "fetch");
                    runCase(st, k, accesses, plain[k], kd, isFetch, true, plainSR, pass ? " [SSP^1]" : "");
                }
            }
        }

        printf("%-16s %8ld %8ld %7.2f%%", name.substr(0, name.size() - 9).c_str(), c.cases, c.ok,
               c.cases ? 100.0 * c.ok / c.cases : 100.0);
        if (failedReads || failedWrites) printf("  (%ld read errors, %ld write errors)", failedReads, failedWrites);
        printf("\n");
        fflush(stdout);
        total += c.cases; totalOk += c.ok;
    }

    if (skippedStacking) printf("\nSkipped %ld cases that fault while a group 1/2 exception is processed (UAE and MAME disagree on these frames)\n", skippedStacking);
    printf("\n%-16s %8ld %8ld %7.2f%%\n\n", "TOTAL", total, totalOk, total ? 100.0 * totalOk / total : 100.0);

    printf("\nDistinct cases (instruction form x access x kind): %zu\n", keyCount.size());
    if (opt.coverage) {
        std::vector<std::string> missing;

        // Byte accesses and register-only forms cannot raise an address error by themselves
        for (auto &f : allForms) {
            if (hitForms.count(f)) continue;
            bool byte = f.find(".b ") != std::string::npos || f.find(".b\0") != std::string::npos;
            bool mem = f.find('(') != std::string::npos || f.find("xxx") != std::string::npos;
            static const char *byteOps[] = { "BCHG ", "BCLR ", "BSET ", "BTST ", "ABCD ", "SBCD ", "NBCD ", "TAS ", "Scc ", "LEA ", "MOVEP." };
            for (auto b : byteOps) if (f.compare(0, strlen(b), b) == 0) byte = true;
            if (!byte && mem) missing.push_back(f);
        }
        printf("Instruction forms with a memory operand (not byte) without a case: %zu of %zu forms\n", missing.size(), allForms.size());
        for (auto &f : missing) printf("  %s\n", f.c_str());
    }

    for (auto &[k, c] : byKind) printf("%-8s %8ld cases, %8ld ok (%.2f%%)\n", k.c_str(), c.cases, c.ok, c.cases ? 100.0 * c.ok / c.cases : 100.0);

    printf("\nMost frequent differences:\n");
    std::vector<std::pair<long, std::string>> sorted;
    for (auto &[s, n] : signatures) sorted.push_back({ n, s });
    std::sort(sorted.rbegin(), sorted.rend());
    for (size_t i = 0; i < sorted.size() && i < 25; i++) printf("  %8ld  %s\n", sorted[i].first, sorted[i].second.c_str());

    return totalOk == total ? 0 : 1;
}
