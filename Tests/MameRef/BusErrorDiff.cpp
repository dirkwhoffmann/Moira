// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* Bus error differential test
 *
 * Runs the same instruction on MAME's microcoded 68000 (the reference) and on
 * Moira, and lets the n-th bus access of the instruction fail with a bus
 * error. This is repeated for every bus access of the instruction. The final
 * states are compared: registers, status register, cycle count, bus cycles,
 * and all memory that has been written (including the bus error stack frame).
 *
 * The instructions and the initial states are taken from the SingleStepTests.
 * Tests that end in an address error are skipped.
 */

#include "Moira.h"
#include "MameCpu.h"
#include "SSTFile.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <map>
#include <set>

using namespace moira;

namespace {

constexpr u32 ADDR_MASK = 0xFFFFFF;
constexpr u32 SR_MASK = 0xA700;       // Condition codes are not compared (see --flags)
u32 srMask = SR_MASK;
constexpr u32 HANDLER = 0x600000;       // Bus error handler

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

        // Install the bus error vector
        for (int i = 0; i < 4; i++) poke(8 + i, u8(HANDLER >> (24 - 8 * i)));
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
// MAME
//

struct Reference {

    Memory mem;
    Fault fault;
    mame::Cpu cpu;

    Reference()
    {
        cpu.read = [this](u32 addr, u16) { return mem.peek16(addr); };
        cpu.write = [this](u32 addr, u16 val, u16 mask) {
            if (mask & 0xFF00) mem.poke(addr, u8(val >> 8));
            if (mask & 0x00FF) mem.poke(addr + 1, u8(val));
        };
        cpu.busError = [this](u32, bool, int) { return fault.hit(); };
    }

    void load(const sst::State &s)
    {
        mem.load(s);
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
    bool fetch = true;              // Fault program fetches?
    bool data = true;               // Fault data accesses?
    std::string tsv;                // Optional result table
    bool exceptions = false;        // Include instructions that raise an exception?
};

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
        else if (a == "--exceptions") opt.exceptions = true;
        else if (a == "--tsv" && more) opt.tsv = argv[++i];
        else if (a == "--flags") srMask = 0xA71F;
        else if (a == "-v" && more) opt.verbose = atoi(argv[++i]);
        else if (a == "--kinds" && more) {
            std::string k = argv[++i];
            opt.data = k == "data" || k == "all";
            opt.fetch = k == "fetch" || k == "all";
        }
        else {
            printf("Usage: %s [--dir <folder>] [--only <text>]... [--limit <n>] [-v <n>] [--kinds data|fetch|all]\n", argv[0]);
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

    printf("Bus error differential test: Moira (MAME mode) vs. MAME's microcoded 68000\n\n");
    printf("%-16s %8s %8s %8s %8s %8s\n", "Instruction", "Cases", "OK", "OK%", "Reads", "Writes");

    FILE *tsv = opt.tsv.empty() ? nullptr : fopen(opt.tsv.c_str(), "w");
    TestCPU moira;
    Reference ref;
    std::map<std::string, Counter> byKind;
    std::map<std::string, long> signatures;
    long total = 0, totalOk = 0;

    for (auto &name : names) {

        sst::File file;
        if (!file.open(opt.dir + "/" + name)) { printf("Cannot read %s\n", name.c_str()); return 1; }

        Counter c;
        long failedReads = 0, failedWrites = 0;
        int shown = 0;
        sst::Test t;

        for (long n = 0; file.next(t); n++) {

            if (opt.limit && n >= opt.limit) break;
            if (expectsAddressError(t) || (t.initial.ssp & 1)) continue;

            // Run the instruction without faults to find out how many bus accesses there are
            ref.load(t.initial);
            ref.cpu.execute();
            int accesses = int(ref.cpu.log().size());
            auto plain = ref.cpu.log();

            // Skip instructions that raise an exception (faults while stacking a frame are double bus faults)
            if (!opt.exceptions && std::any_of(plain.begin(), plain.end(), [](auto &c) { return !c.write && c.fc == 5 && c.addr < 0x400; })) continue;

            for (int k = 0; k < accesses; k++) {

                const char *kd = kind(plain[k]);
                bool isFetch = !strcmp(kd, "fetch");
                if (isFetch ? !opt.fetch : !opt.data) continue;

                // Reference
                ref.load(t.initial);
                ref.fault.index = k;
                int refCycles = ref.cpu.execute();
                if (ref.cpu.halted()) continue;
                auto rs = ref.cpu.state();

                // Moira
                moira.load(t.initial);
                moira.fault.index = k;
                moira.logging = true;
                moira.execute();
                moira.logging = false;

                // Compare
                std::string diff;
                auto add = [&](const std::string &s) { diff += (diff.empty() ? "" : " ") + s; };

                for (int i = 0; i < 8; i++) if (moira.getD(i) != rs.d[i]) { add("D" + std::to_string(i)); break; }
                for (int i = 0; i < 7; i++) if (moira.getA(i) != rs.a[i]) {
                    add("A" + std::to_string(i));
                    if (getenv("BED_REGS")) printf("      A%d: initial %08x exp %08x got %08x\n", i, t.initial.a[i], rs.a[i], moira.getA(i));
                    break;
                }
                if (moira.getUSP() != rs.usp) add("USP");
                if (moira.getISP() != rs.ssp) add("SSP");
                if ((moira.getSR() & srMask) != (rs.sr & srMask)) add("SR");
                if ((moira.getPC() & ADDR_MASK) != (rs.pc & ADDR_MASK)) add("PC");
                if (i64(moira.getClock()) != refCycles) add("cycles");

                // Memory: compare everything that has been written by either side
                std::set<u32> addrs(moira.mem.touched.begin(), moira.mem.touched.end());
                addrs.insert(ref.mem.touched.begin(), ref.mem.touched.end());
                std::set<int> badWords;
                for (auto a : addrs) {
                    if (a == rs.ssp + 2) continue;      // Undefined upper byte of the fault address
                    if (a == rs.ssp + 9 && !(srMask & 0x1F)) {      // Condition codes in the stacked SR
                        if (((moira.mem.peek(a) ^ ref.mem.peek(a)) & ~0x1F) == 0) continue;
                    }
                    if (moira.mem.peek(a) != ref.mem.peek(a)) {
                        u32 sp = rs.ssp;
                        if (a >= sp && a < sp + 14) badWords.insert(int((a - sp) / 2)); else badWords.insert(-1);
                    }
                }
                static const char *fields[] = { "frame.code", "frame.addr", "frame.addr", "frame.ird", "frame.sr", "frame.pc", "frame.pc" };
                std::set<std::string> bad;
                for (int w : badWords) bad.insert(w < 0 ? "mem" : fields[w]);
                for (auto &b : bad) add(b);

                // Bus cycles
                auto &mlog = ref.cpu.log();
                if (mlog.size() != moira.log.size()) add("bus.count");
                else {
                    for (size_t i = 0; i < mlog.size(); i++) {
                        auto &e = mlog[i]; auto &g = moira.log[i];
                        std::string d;
                        if (getenv("BED_DEBUG") && i == 0) printf("      first bus cycle: mame %lld moira %lld; total mame %d moira %lld\n", (long long)e.cycle, (long long)g.cycle, refCycles, (long long)moira.getClock());
                        if (e.cycle != g.cycle) d += "cycle ";
                        if (e.write != g.write) d += "rw ";
                        if (e.addr != g.addr) d += "addr ";
                        if (e.mask != g.mask) d += "mask ";
                        if (e.fc != g.fc) d += "fc ";
                        u16 dm = 0xFFFF;
                        if (e.write && e.fc == 5 && e.addr == ((rs.ssp + 2) & ~1u)) dm = 0x00FF;        // Undefined upper byte of the fault address
                        if (e.write && e.fc == 5 && e.addr == ((rs.ssp + 8) & ~1u)) dm = ~srMask & 0x1F ? 0xFFE0 : 0xFFFF;   // Condition codes
                        if (e.write && (e.data & e.mask & dm) != (g.data & g.mask & dm)) d += "data ";
                        if (!d.empty()) { d.pop_back(); add("bus#" + std::to_string(i) + "(" + d + ")"); break; }
                    }
                }

                if (tsv) fprintf(tsv, "%s\t%d\t%d\t%s\t%s\t%s\n", t.name.c_str(), k, accesses, kd, diff.empty() ? "ok" : "FAIL", diff.c_str());
                std::string key = std::string(kd);
                c.cases++;
                byKind[key].cases++;
                if (diff.empty()) { c.ok++; byKind[key].ok++; continue; }

                (isFetch ? failedReads : (plain[k].write ? failedWrites : failedReads))++;
                signatures[diff]++;
                byKind[key].signatures[diff]++;

                if (shown < opt.verbose) {
                    shown++;
                    if (getenv("BED_DEBUG")) {
                        for (auto &e : mlog) printf("      mame  c%-3lld %s %06x m%04x d%04x fc%d\n", (long long)e.cycle, e.write ? "W" : "R", e.addr, e.mask, e.data, e.fc);
                        for (auto &g : moira.log) printf("      moira c%-3lld %s %06x m%04x d%04x fc%d\n", (long long)g.cycle, g.write ? "W" : "R", g.addr, g.mask, g.data, g.fc);
                    }
                    printf("    %s  fault #%d (%s %06x fc %d): %s\n", t.name.c_str(), k, kd, plain[k].addr, plain[k].fc, diff.c_str());
                    if (opt.verbose > 1) {
                        printf("        initial prefetch: %04x %04x  pc %06x\n", t.initial.prefetch[0], t.initial.prefetch[1], t.initial.pc);
                        printf("        exp frame: %s\n", words(ref.mem, rs.ssp, 7).c_str());
                        printf("        got frame: %s\n", words(moira.mem, rs.ssp, 7).c_str());
                    }
                }
            }
        }

        printf("%-16s %8ld %8ld %7.2f%% %8ld %8ld\n", name.substr(0, name.size() - 9).c_str(), c.cases, c.ok,
               c.cases ? 100.0 * c.ok / c.cases : 100.0, failedReads, failedWrites);
        fflush(stdout);
        total += c.cases; totalOk += c.ok;
    }

    printf("\n%-16s %8ld %8ld %7.2f%%\n\n", "TOTAL", total, totalOk, total ? 100.0 * totalOk / total : 100.0);

    for (auto &[k, c] : byKind) printf("%-8s %8ld cases, %8ld ok (%.2f%%)\n", k.c_str(), c.cases, c.ok, c.cases ? 100.0 * c.ok / c.cases : 100.0);

    printf("\nMost frequent differences:\n");
    std::vector<std::pair<long, std::string>> sorted;
    for (auto &[s, n] : signatures) sorted.push_back({ n, s });
    std::sort(sorted.rbegin(), sorted.rend());
    for (size_t i = 0; i < sorted.size() && i < 25; i++) printf("  %8ld  %s\n", sorted[i].first, sorted[i].second.c_str());

    return totalOk == total ? 0 : 1;
}
