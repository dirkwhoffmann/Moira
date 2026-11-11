// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* SingleStepTests runner
 *
 * Runs Moira against the 68000 test suite from
 *
 *    https://github.com/SingleStepTests/m68000.
 *
 * Each test sets up the CPU and the memory, executes a single instruction, and
 * checks the registers, the memory, the number of elapsed cycles and the bus
 * transactions. The bus check compares every read and write access (address,
 * data, function code, and the cycle in which it happens).
 *
 * The expected values have been recorded from MAME's microcoded 68000 core.
 * Because that core has bugs of its own, Moira offers a MAME compatibility mode,
 * which is the mode this runner is meant to be linked against.
 */

#include "Moira.h"
#include "SSTFile.h"
#include "Common.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

using namespace moira;

namespace {

//
// Test CPU with a flat 16 MB address space
//

/* A bus access as seen by Moira. Moira reports each access through the memory
 * callbacks of the CPU. The access size is 1 (byte), 2 (word) or 4 (long word).
 */
struct BusEvent {

    i64 cycle;                  // Clock at the time of the access
    bool write;
    int size;
    u32 addr;
    u32 data;
    u8 fc;
};

class TestCPU : public Moira {

    std::vector<u8> ram = std::vector<u8>(1 << 24);
    std::vector<u32> touched;

public:

    // Recorded bus accesses (only while logging is enabled)
    mutable std::vector<BusEvent> log;
    bool logging = false;

    void record(bool write, int size, u32 addr, u32 data) const {
        if (logging) log.push_back({ getClock(), write, size, addr & 0xFFFFFF, data, readFC() });
    }

    // Memory access without logging
    u8 peek8(u32 addr) const { return ram[addr & 0xFFFFFF]; }
    u16 peek16(u32 addr) const { return u16(peek8(addr) << 8 | peek8(addr + 1)); }

    u8 read8(u32 addr) const override { record(false, 1, addr, peek8(addr)); return peek8(addr); }
    u16 read16(u32 addr) const override { record(false, 2, addr, peek16(addr)); return peek16(addr); }
    u32 read32(u32 addr) const override { return u32(read16(addr) << 16 | read16(addr + 2)); }

    void write8(u32 addr, u8 val) const override { record(true, 1, addr, val); poke(addr, val); }
    void write16(u32 addr, u16 val) const override { record(true, 2, addr, val); poke(addr, u8(val >> 8)); poke(addr + 1, u8(val)); }
    void write32(u32 addr, u32 val) const override { write16(addr, u16(val >> 16)); write16(addr + 2, u16(val)); }

    void poke(u32 addr, u8 val) const {

        auto *self = const_cast<TestCPU *>(this);
        self->ram[addr & 0xFFFFFF] = val;

        // Remember the written address, so memory can be cleared quickly
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
    bool bus = true;
    i64 elapsed = 0;

    bool state() const { return dataRegs && addrRegs && stackPtrs && sr && pc && mem; }
    bool all() const { return state() && cycles && bus; }
};

// Counters
struct Stats {

    long tests = 0, state = 0, cycles = 0, bus = 0, all = 0;

    void add(const Outcome &o) { tests++; state += o.state(); cycles += o.cycles; bus += o.bus; all += o.all(); }
    void add(const Stats &s) { tests += s.tests; state += s.state; cycles += s.cycles; bus += s.bus; all += s.all; }
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
    
    // (in user mode, STOP is a privilege violation)
    bool stops = t.initial.prefetch[0] == 0x4E72 && (t.initial.sr & 0x2000);

    return stops ? 0 : 4;
}

/* Bus transactions
 *
 * The tests record every bus cycle of the instruction. Idle periods are not
 * reported by Moira, they are implied by the cycle at which an access happens.
 * Read and write address errors (types READ_AE and WRITE_AE) are cycles in which
 * the bus is not accessed. They are skipped, because Moira reports no access.
 */
constexpr i64 BUS_CYCLE_OFFSET = 2;

std::vector<BusEvent> expectedBus(const sst::Test &t)
{
    std::vector<BusEvent> result;
    i64 cycle = 0;

    for (auto &b : t.bus) {

        if (b.type == sst::Bus::READ || b.type == sst::Bus::WRITE || b.type == sst::Bus::TAS) {

            // The address bus has no A0. A byte is transferred on the lane selected by UDS or LDS.
            int size = b.uds && b.lds ? 2 : 1;
            u32 addr = size == 2 ? b.addr : b.addr | (b.lds ? 1 : 0);
            u32 data = size == 2 ? b.data : (b.uds ? b.data >> 8 : b.data) & 0xFF;
            result.push_back({ cycle + BUS_CYCLE_OFFSET, b.type == sst::Bus::WRITE, size, addr, data, u8(b.fc) });
        }
        cycle += b.cycles;
    }
    return result;
}

// Status register bits that are compared (T, S, interrupt mask is ignored, XNZVC)
constexpr u32 SR_MASK = 0xA71F;

// Compares the recorded bus accesses with the expected ones. Returns a description
// of the first difference or an empty string if both agree.
std::string busDiff(const std::vector<BusEvent> &exp, const std::vector<BusEvent> &got)
{
    size_t n = std::min(exp.size(), got.size());

    for (size_t i = 0; i < n; i++) {

        auto &e = exp[i], &g = got[i];
        std::string d;
        if (e.cycle != g.cycle) d += "cycle ";
        if (e.write != g.write) d += "rw ";
        if (e.size != g.size) d += "size ";
        if (e.addr != g.addr) d += "addr ";
        if (e.data != g.data) d += "data ";
        if (e.fc != g.fc) d += "fc ";
        if (!d.empty()) { d.pop_back(); return "#" + std::to_string(i) + ":" + d; }
    }
    if (exp.size() != got.size()) return "count:" + std::to_string(exp.size()) + "/" + std::to_string(got.size());
    return "";
}


Outcome compare(TestCPU &cpu, const sst::Test &t, i64 elapsed)
{
    Outcome o;
    auto &f = t.final;

    for (int i = 0; i < 8; i++) o.dataRegs &= cpu.getD(i) == f.d[i];
    for (int i = 0; i < 7; i++) o.addrRegs &= cpu.getA(i) == f.a[i];
    o.stackPtrs = cpu.getUSP() == f.usp && cpu.getISP() == f.ssp;
    o.sr = (cpu.getSR() & SR_MASK) == (f.sr & SR_MASK);
    o.pc = ((cpu.getPC() + pcOffset(t)) & ADDR_MASK) == (f.pc & ADDR_MASK);
    for (auto &[addr, val] : f.ram) o.mem &= cpu.peek8(addr) == val;

    o.elapsed = elapsed;
    o.cycles = elapsed == i64(t.cycles);
    o.bus = busDiff(expectedBus(t), cpu.log).empty();
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
    if (!o.bus) printf(" BUS");
    printf("\n");
}

void printBus(const sst::Test &t, TestCPU &cpu)
{
    auto line = [](const char *tag, const BusEvent &e) {
        printf("      %s cycle %3lld %c%d addr %06x data %04x fc %d\n", tag, (long long)e.cycle, e.write ? 'w' : 'r', e.size, e.addr, e.data, e.fc);
    };
    for (auto &e : expectedBus(t)) line("exp", e);
    for (auto &e : cpu.log) line("got", e);
}

void dump(FILE *f, const char *file, const sst::Test &t, const Outcome &o, TestCPU &cpu, bool ae)
{
    auto &e = t.final;
    fprintf(f, "%s|%s|%d|%u|%lld|%04x|%04x|", file, t.name.c_str(), ae, t.cycles, (long long)o.elapsed,
            e.sr, cpu.getSR());
    for (int i = 0; i < 8; i++) if (cpu.getD(i) != e.d[i]) fprintf(f, "d%d:%08x:%08x ", i, e.d[i], cpu.getD(i));
    for (int i = 0; i < 7; i++) if (cpu.getA(i) != e.a[i]) fprintf(f, "a%d:%08x:%08x:%08x ", i, t.initial.a[i], e.a[i], cpu.getA(i));
    if (cpu.getUSP() != e.usp) fprintf(f, "usp:%08x:%08x ", e.usp, cpu.getUSP());
    if (cpu.getISP() != e.ssp) fprintf(f, "ssp:%08x:%08x ", e.ssp, cpu.getISP());
    if (!o.pc) fprintf(f, "pc:%06x:%06x ", e.pc & ADDR_MASK, (cpu.getPC() + pcOffset(t)) & ADDR_MASK);
    for (auto &[addr, val] : e.ram) if (cpu.peek8(addr) != val) fprintf(f, "m%06x:%02x:%02x ", addr, val, cpu.peek8(addr));
    fprintf(f, "|ssp0=%08x usp0=%08x sp=%08x pc0=%06x sr0=%04x", t.initial.ssp, t.initial.usp, (e.sr & 0x2000) ? e.ssp : e.usp, t.initial.pc & ADDR_MASK, t.initial.sr);
    fprintf(f, " bus=%s", busDiff(expectedBus(t), cpu.log).c_str());
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
        cpu.logging = true;
        cpu.log.clear();
        cpu.execute();
        cpu.logging = false;
        auto o = compare(cpu, t, cpu.getClock());

        (expectsAddressError(t) ? fs.addressError : fs.regular).add(o);

        if (!o.all() || !o.bus) {
            if (shown < opt.verbose) { shown++; describe(t, o, cpu); if (!o.bus) printBus(t, cpu); }
            if (opt.dump) dump(opt.dump, name.c_str(), t, o, cpu, expectsAddressError(t));
        }
    }
    return true;
}

std::vector<std::string> collectFiles(const Options &opt)
{
    std::vector<std::string> names;
    std::error_code ec;
    for (auto &e : std::filesystem::directory_iterator(opt.dir, ec)) {
        std::string n = e.path().filename().string();
        if (n.size() > 9 && n.compare(n.size() - 9, 9, ".json.bin") == 0) {
            if (opt.only.empty() || std::any_of(opt.only.begin(), opt.only.end(), [&](auto &s) { return n.find(s) != std::string::npos; })) {
                names.push_back(n);
            }
        }
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


}

static inline const char *reference()
{
    switch (compat) {

        case Compatibility::MUSASHI: return "Musashi";
        case Compatibility::UAE:     return "UAE";
        case Compatibility::MAME:    return "MAME";
            
        default:
            return "Moira";
    }
}

int main(int argc, char **argv)
{
    Options opt;

    opt.dir = defaultDir();
    if (const char *env = getenv("SST_DIR")) opt.dir = env;

    std::string dumpFile;
    common::Args args(argc, argv);
    args.text("--dir", opt.dir, "Folder with the *.json.bin files of the SingleStepTests 68000 suite\n(default: $SST_DIR if set, otherwise the files shipped in Tests/ThirdParty/SingleStepTests)", "<folder>");
    args.list("--only", opt.only, "Only run files whose name contains <text>. May be repeated.");
    args.integer("--limit", opt.limit, "Run at most <n> tests per file");
    args.integer("-v", opt.verbose, "Print the first <n> failures of each file");
    args.text("--dump", dumpFile, "Write one line per failing test to <file>", "<file>");
    if (!args.parse()) return 1;
    if (!dumpFile.empty()) opt.dump = fopen(dumpFile.c_str(), "w");

    auto files = collectFiles(opt);
    if (files.empty()) { printf("No test files found in %s\n", opt.dir.c_str()); return 1; }

    common::printBanner("Runs Moira against the SingleStepTests suite for the Motorola 68000 and\n"
                        "compares registers, memory and bus cycles after each instruction.\n"
                        "https://github.com/SingleStepTests/m68000\n");
    common::printSetting("Core under test", "Moira %s", moira::Moira::build().c_str());
    common::printSetting("Reference", "%s (Timing : %s)", reference(), MOIRA_PRECISE_TIMING ? "precise" : "fast");
    printf("\n");

    TestCPU cpu;
    FileStats total;

//     printf("%-16s %-32s %-32s\n%-16s ", "", "Standard Tests", "Address errors", "");
    printf("%-16s ", "Test");
    printf("%9s %9s %9s %9s ", "Count", "State", "Cycles", "Bus");
    printf("%9s %9s %9s %9s\n", "AE", "State", "Cycles", "Bus");

    for (auto &name : files) {

        FileStats fs;
        if (!runFile(cpu, opt, opt.dir + "/" + name, name, fs)) return 1;

        auto &r = fs.regular, &a = fs.addressError;
        printf("%-16s ", name.substr(0, name.size() - 9).c_str());
        printf("%9ld %8.2f%% %8.2f%% %8.2f%% ",
               r.tests, percent(r.state, r.tests), percent(r.cycles, r.tests), percent(r.bus, r.tests));
        printf("%9ld %8.2f%% %8.2f%% %8.2f%%\n",
               a.tests, percent(a.state, a.tests), percent(a.cycles, a.tests), percent(a.bus, a.tests));

        /*
        printf("%-16s %8ld %8.2f%% %8.2f%% %8.2f%%   %ld, %5.1f%%, %5.1f%%, %5.1f%%\n",
               name.substr(0, name.size() - 9).c_str(), r.tests,
               percent(r.state, r.tests), percent(r.cycles, r.tests), percent(r.all, r.tests),
               a.tests, percent(a.state, a.tests), percent(a.cycles, a.tests), percent(a.all, a.tests));
         */
        fflush(stdout);

        total.regular.add(r);
        total.addressError.add(a);
    }

    auto &r = total.regular, &a = total.addressError;
    
    printf("\n%-16s ", "TOTAL");
    printf("%9ld %8.2f%% %8.2f%% %8.2f%% ",
           r.tests, percent(r.state, r.tests), percent(r.cycles, r.tests), percent(r.bus, r.tests));
    printf("%9ld %8.2f%% %8.2f%% %8.2f%%\n",
           a.tests, percent(a.state, a.tests), percent(a.cycles, a.tests), percent(a.bus, a.tests));
    
    /*
    printf("\n%-16s %8ld %8.2f%% %8.2f%% %8.2f%%   %ld, %.1f%%, %.1f%%, %.1f%%\n", "TOTAL", r.tests,
           percent(r.state, r.tests), percent(r.cycles, r.tests), percent(r.all, r.tests),
           a.tests, percent(a.state, a.tests), percent(a.cycles, a.tests), percent(a.all, a.tests));
     */
    
    if (opt.dump) fclose(opt.dump);
    return (r.all == r.tests && a.all == a.tests) ? 0 : 1;
}
