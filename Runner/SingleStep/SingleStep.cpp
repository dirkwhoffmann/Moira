// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* SingleStepTests runner
 *
 * Runs Moira against the 68000 test suite from https://github.com/SingleStepTests/m68000.
 * Each test sets up the CPU and the memory, executes a single instruction, and
 * checks the registers, the memory and the number of elapsed cycles.
 *
 * The expected values have been recorded from MAME's microcoded 68000 core. Because
 * that core has bugs of its own, Moira offers a MAME compatibility mode
 * (MOIRA_COMPATIBILITY=MAME), which is the mode this runner is meant to be linked against.
 */

#include "Moira.h"
#include "SSTFile.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

using namespace moira;

namespace {

//
// Test CPU with a flat 16 MB address space
//

class TestCPU : public Moira {

    std::vector<u8> ram = std::vector<u8>(1 << 24);
    std::vector<u32> touched;

public:

    u8 read8(u32 addr) const override { return ram[addr & 0xFFFFFF]; }
    u16 read16(u32 addr) const override { return u16(read8(addr) << 8 | read8(addr + 1)); }
    u32 read32(u32 addr) const override { return u32(read16(addr) << 16 | read16(addr + 2)); }

    void write8(u32 addr, u8 val) const override { poke(addr, val); }
    void write16(u32 addr, u16 val) const override { poke(addr, u8(val >> 8)); poke(addr + 1, u8(val)); }
    void write32(u32 addr, u32 val) const override { write16(addr, u16(val >> 16)); write16(addr + 2, u16(val)); }

    // Writes a byte and remembers the address, so that the memory can be cleared quickly
    void poke(u32 addr, u8 val) const {
        auto *self = const_cast<TestCPU *>(this);
        self->ram[addr & 0xFFFFFF] = val;
        self->touched.push_back(addr & 0xFFFFFF);
    }

    void clearMemory() {
        for (auto addr : touched) ram[addr] = 0;
        touched.clear();
    }

    TestCPU() { setModel(CPUModel::M68000); }

    // Sets up the CPU as described by a test
    void load(const sst::State &s) {

        clearMemory();
        for (auto &[addr, val] : s.ram) poke(addr, val);

        reset();
        setSR(u16(s.sr));
        setUSP(s.usp);
        setISP(s.ssp);
        for (int i = 0; i < 8; i++) setD(i, s.d[i]);
        for (int i = 0; i < 7; i++) setA(i, s.a[i]);

        // The test's PC is the address of the next prefetch, which is 4 bytes behind the instruction
        setPC(s.pc - 4);
        setPC0(s.pc - 4);
        setIRD(u16(s.prefetch[0]));
        setIRC(u16(s.prefetch[1]));
        setClock(0);
    }
};

//
// Comparing
//

// Result of a single test
struct Outcome {

    bool dataRegs = true, addrRegs = true, stackPtrs = true, sr = true, pc = true, mem = true;
    bool cycles = true;
    i64 elapsed = 0;

    bool state() const { return dataRegs && addrRegs && stackPtrs && sr && pc && mem; }
    bool all() const { return state() && cycles; }
};

// Counters
struct Stats {

    long tests = 0, state = 0, cycles = 0, all = 0;

    void add(const Outcome &o) { tests++; state += o.state(); cycles += o.cycles; all += o.all(); }
    void add(const Stats &s) { tests += s.tests; state += s.state; cycles += s.cycles; all += s.all; }
};

struct Options {

    std::string dir;                // Folder with the test files
    std::vector<std::string> only;  // Only run files containing one of these strings
    long limit = 0;                 // Maximum number of tests per file (0 = all)
    int verbose = 0;                // Number of failures to print per file
    FILE *dump = nullptr;           // Failure dump
};

// Addresses are 24 bit wide
constexpr u32 ADDR_MASK = 0xFFFFFF;

/* The tests report the PC as the address of the next prefetch, which is 4 bytes behind
 * the next instruction. A stopped CPU does not prefetch, so no offset applies after STOP.
 */
u32 pcOffset(const sst::Test &t) {
    bool stops = t.initial.prefetch[0] == 0x4E72 && (t.initial.sr & 0x2000);   // (in user mode, STOP is a privilege violation)
    return stops ? 0 : 4;
}

// Status register bits that are compared (T, S, interrupt mask is ignored, XNZVC)
constexpr u32 SR_MASK = 0xA71F;

Outcome compare(TestCPU &cpu, const sst::Test &t, i64 elapsed)
{
    Outcome o;
    auto &f = t.final;

    for (int i = 0; i < 8; i++) o.dataRegs &= cpu.getD(i) == f.d[i];
    for (int i = 0; i < 7; i++) o.addrRegs &= cpu.getA(i) == f.a[i];
    o.stackPtrs = cpu.getUSP() == f.usp && cpu.getISP() == f.ssp;
    o.sr = (cpu.getSR() & SR_MASK) == (f.sr & SR_MASK);
    o.pc = ((cpu.getPC() + pcOffset(t)) & ADDR_MASK) == (f.pc & ADDR_MASK);
    for (auto &[addr, val] : f.ram) o.mem &= cpu.read8(addr) == val;

    o.elapsed = elapsed;
    o.cycles = elapsed == i64(t.cycles);
    return o;
}

/* A test expects an address error if it ends in the address error handler.
 * The handler address is stored in the exception vector 3, which the test
 * provides in its initial memory. The final PC is the next prefetch address,
 * i.e., it is 4 bytes behind the first instruction of the handler.
 */
bool expectsAddressError(const sst::Test &t)
{
    u32 vec = 0; int bytes = 0;

    for (auto &[addr, val] : t.initial.ram) {
        if (addr >= 12 && addr <= 15) { vec |= u32(val) << (8 * (15 - addr)); bytes++; }
    }
    return bytes == 4 && ((t.final.pc - 4) & ADDR_MASK) == (vec & ADDR_MASK);
}

void describe(const sst::Test &t, const Outcome &o, TestCPU &cpu)
{
    printf("    %s:", t.name.c_str());
    if (!o.dataRegs) printf(" D");
    if (!o.addrRegs) printf(" A");
    if (!o.stackPtrs) printf(" SP");
    if (!o.sr) printf(" SR(exp %04x, got %04x)", t.final.sr & SR_MASK, cpu.getSR() & SR_MASK);
    if (!o.pc) printf(" PC(exp %06x, got %06x)", t.final.pc & ADDR_MASK, (cpu.getPC() + pcOffset(t)) & ADDR_MASK);
    if (!o.mem) printf(" RAM");
    if (!o.cycles) printf(" cycles(exp %u, got %lld)", t.cycles, (long long)o.elapsed);
    printf("\n");
}

void dump(FILE *f, const char *file, const sst::Test &t, const Outcome &o, TestCPU &cpu, bool ae)
{
    auto &e = t.final;
    fprintf(f, "%s|%s|%d|%u|%lld|%04x|%04x|", file, t.name.c_str(), ae, t.cycles, (long long)o.elapsed,
            e.sr, cpu.getSR());
    for (int i = 0; i < 8; i++) if (cpu.getD(i) != e.d[i]) fprintf(f, "d%d:%08x:%08x ", i, e.d[i], cpu.getD(i));
    for (int i = 0; i < 7; i++) if (cpu.getA(i) != e.a[i]) fprintf(f, "a%d:%08x:%08x ", i, e.a[i], cpu.getA(i));
    if (cpu.getUSP() != e.usp) fprintf(f, "usp:%08x:%08x ", e.usp, cpu.getUSP());
    if (cpu.getISP() != e.ssp) fprintf(f, "ssp:%08x:%08x ", e.ssp, cpu.getISP());
    if (!o.pc) fprintf(f, "pc:%06x:%06x ", e.pc & ADDR_MASK, (cpu.getPC() + pcOffset(t)) & ADDR_MASK);
    for (auto &[addr, val] : e.ram) if (cpu.read8(addr) != val) fprintf(f, "m%06x:%02x:%02x ", addr, val, cpu.read8(addr));
    fprintf(f, "\n");
}

//
// Running
//

struct FileStats { Stats regular, addressError; };

bool runFile(TestCPU &cpu, const Options &opt, const std::string &path, const std::string &name, FileStats &fs)
{
    sst::File file;
    if (!file.open(path)) { printf("Cannot read %s\n", path.c_str()); return false; }

    int shown = 0;
    sst::Test t;
    for (long n = 0; file.next(t); n++) {

        if (opt.limit && n >= opt.limit) break;

        cpu.load(t.initial);
        cpu.execute();
        auto o = compare(cpu, t, cpu.getClock());

        (expectsAddressError(t) ? fs.addressError : fs.regular).add(o);

        if (!o.all()) {
            if (shown < opt.verbose) { shown++; describe(t, o, cpu); }
            if (opt.dump) dump(opt.dump, name.c_str(), t, o, cpu, expectsAddressError(t));
        }
    }
    return true;
}

std::vector<std::string> collectFiles(const Options &opt)
{
    std::vector<std::string> names;
    if (DIR *d = opendir(opt.dir.c_str())) {
        while (auto *e = readdir(d)) {
            std::string n = e->d_name;
            if (n.size() > 9 && n.compare(n.size() - 9, 9, ".json.bin") == 0) {
                if (opt.only.empty() || std::any_of(opt.only.begin(), opt.only.end(), [&](auto &s) { return n.find(s) != std::string::npos; })) {
                    names.push_back(n);
                }
            }
        }
        closedir(d);
    }
    std::sort(names.begin(), names.end());
    return names;
}

// The test files shipped with the repository (located relative to this source file)
std::string defaultDir()
{
    std::string file = __FILE__;
    return file.substr(0, file.find_last_of("/\\")) + "/../ThirdParty/SingleStepTests/m68000/v1";
}

double percent(long a, long b) { return b ? 100.0 * double(a) / double(b) : 100.0; }

void usage(const char *prog)
{
    printf("Usage: %s [--dir <folder>] [--only <text>]... [--limit <n>] [-v <n>] [--dump <file>]\n", prog);
    printf("  --dir <folder>   Folder containing the *.json.bin files of the SingleStepTests 68000 suite.\n");
    printf("                   Default: $SST_DIR if set, otherwise the files shipped in Runner/ThirdParty/SingleStepTests\n");
    printf("  --only <text>    Only run files whose name contains <text>. May be repeated.\n");
    printf("  --limit <n>      Run at most <n> tests per file\n");
    printf("  -v <n>           Print the first <n> failures of each file\n");
    printf("  --dump <file>    Write one line per failing test to <file>\n");
}

}

int main(int argc, char **argv)
{
    Options opt;

    opt.dir = defaultDir();
    if (const char *env = getenv("SST_DIR")) opt.dir = env;

    for (int i = 1; i < argc; i++) {

        std::string a = argv[i];
        bool more = i + 1 < argc;

        if (a == "--dir" && more)        { opt.dir = argv[++i]; }
        else if (a == "--only" && more)  { opt.only.push_back(argv[++i]); }
        else if (a == "--limit" && more) { opt.limit = atol(argv[++i]); }
        else if (a == "-v" && more)      { opt.verbose = atoi(argv[++i]); }
        else if (a == "--dump" && more)  { opt.dump = fopen(argv[++i], "w"); }
        else { usage(argv[0]); return 1; }
    }

    auto files = collectFiles(opt);
    if (files.empty()) { printf("No test files found in %s\n", opt.dir.c_str()); return 1; }

    printf("Moira SingleStepTests runner. (C) Dirk W. Hoffmann, 2019 - 2026\n");
    printf("Core %s (compatibility mode: %s, %s timing)\n\n", Moira::build().c_str(),
           compat == Compatibility::MAME ? "MAME" : compat == Compatibility::MUSASHI ? "Musashi" : compat == Compatibility::UAE ? "UAE" : "default",
           MOIRA_PRECISE_TIMING ? "precise" : "fast");

    TestCPU cpu;
    FileStats total;

    printf("%-16s %8s %9s %9s %9s   %s\n", "", "Tests", "State", "Cycles", "Both", "Address errors (tests, state, cycles, both)");

    for (auto &name : files) {

        FileStats fs;
        if (!runFile(cpu, opt, opt.dir + "/" + name, name, fs)) return 1;

        auto &r = fs.regular, &a = fs.addressError;
        printf("%-16s %8ld %8.2f%% %8.2f%% %8.2f%%   %ld, %.1f%%, %.1f%%, %.1f%%\n",
               name.substr(0, name.size() - 9).c_str(), r.tests,
               percent(r.state, r.tests), percent(r.cycles, r.tests), percent(r.all, r.tests),
               a.tests, percent(a.state, a.tests), percent(a.cycles, a.tests), percent(a.all, a.tests));
        fflush(stdout);

        total.regular.add(r);
        total.addressError.add(a);
    }

    auto &r = total.regular, &a = total.addressError;
    printf("\n%-16s %8ld %8.2f%% %8.2f%% %8.2f%%   %ld, %.1f%%, %.1f%%, %.1f%%\n", "TOTAL", r.tests,
           percent(r.state, r.tests), percent(r.cycles, r.tests), percent(r.all, r.tests),
           a.tests, percent(a.state, a.tests), percent(a.cycles, a.tests), percent(a.all, a.tests));

    if (opt.dump) fclose(opt.dump);
    return (r.all == r.tests && a.all == a.tests) ? 0 : 1;
}
