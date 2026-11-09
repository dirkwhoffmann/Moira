// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* Self check of the MAME reference CPU
 *
 * The SingleStepTests for the 68000 have been recorded from MAME's microcoded
 * 68000. If the reference CPU (MameCpu) reproduces them exactly, we know that
 * the framework glue around MAME's core does not distort the core's behavior.
 * This is the prerequisite for using MameCpu as a reference for cases that
 * the test suite does not cover (bus errors in particular).
 */

#include "MameCpu.h"
#include "SSTFile.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>

namespace {

constexpr uint32_t ADDR_MASK = 0xFFFFFF;
constexpr uint32_t SR_MASK = 0xA71F;

std::vector<uint8_t> ram(1 << 24);
std::vector<uint32_t> touched;

uint16_t peek16(uint32_t a) { return uint16_t(ram[a & ADDR_MASK] << 8 | ram[(a + 1) & ADDR_MASK]); }
void poke8(uint32_t a, uint8_t v) { ram[a & ADDR_MASK] = v; touched.push_back(a & ADDR_MASK); }

// STOP does not prefetch, so no offset applies afterwards
uint32_t pcOffset(const sst::Test &t)
{
    bool stops = t.initial.prefetch[0] == 0x4E72 && (t.initial.sr & 0x2000);
    return stops ? 0 : 4;
}

struct Expected { int cycle; bool write; uint32_t addr; uint16_t mask; uint16_t data; int fc; bool ae; };

std::vector<Expected> expectedBus(const sst::Test &t)
{
    std::vector<Expected> result;
    int cycle = 0;

    for (auto &b : t.bus) {
        if (b.type != sst::Bus::IDLE) {
            uint16_t mask = b.uds && b.lds ? 0xFFFF : b.uds ? 0xFF00 : 0x00FF;
            bool write = b.type == sst::Bus::WRITE || b.type == sst::Bus::TAS || b.type == sst::Bus::WRITE_AE;
            bool ae = b.type == sst::Bus::READ_AE || b.type == sst::Bus::WRITE_AE;
            result.push_back({ cycle, write, b.addr & ~1u, mask, uint16_t(b.data & mask), int(b.fc), ae });
        }
        cycle += b.cycles;
    }
    return result;
}

bool busEqual(const sst::Test &t, const std::vector<mame::BusCycle> &got, std::string &why)
{
    auto exp = expectedBus(t);

    if (exp.size() != got.size()) { why = "count " + std::to_string(exp.size()) + "/" + std::to_string(got.size()); return false; }

    for (size_t i = 0; i < exp.size(); i++) {

        auto &e = exp[i]; auto &g = got[i];
        std::string d;
        if (e.cycle != g.cycle) d += "cycle(" + std::to_string(e.cycle) + "/" + std::to_string(g.cycle) + ") ";
        if (e.write != g.write) d += "rw ";
        if (e.addr != g.addr) d += "addr ";
        if (e.mask != g.mask) d += "mask ";
        if (!e.ae && (e.data & e.mask) != (g.data & g.mask)) d += "data ";
        if (e.fc != g.fc) d += "fc ";
        if (!d.empty()) { why = "#" + std::to_string(i) + ": " + d; return false; }
    }
    return true;
}

struct Result { bool state, cycles, bus; };

Result run(mame::Cpu &cpu, const sst::Test &t, std::string &why)
{
    // Set up memory
    for (auto a : touched) ram[a] = 0;
    touched.clear();
    for (auto &[addr, val] : t.initial.ram) poke8(addr, val);

    // Set up the CPU
    mame::State s;
    for (int i = 0; i < 8; i++) s.d[i] = t.initial.d[i];
    for (int i = 0; i < 7; i++) s.a[i] = t.initial.a[i];
    s.usp = t.initial.usp;
    s.ssp = t.initial.ssp;
    s.sr = uint16_t(t.initial.sr);
    s.pc = (t.initial.pc - 4) & ADDR_MASK;
    s.ird = uint16_t(t.initial.prefetch[0]);
    s.irc = uint16_t(t.initial.prefetch[1]);
    cpu.load(s);

    int cycles = cpu.execute();

    // Compare
    auto r = cpu.state();
    auto &f = t.final;
    Result res { true, cycles == int(t.cycles), true };

    for (int i = 0; i < 8; i++) res.state &= r.d[i] == f.d[i];
    for (int i = 0; i < 7; i++) res.state &= r.a[i] == f.a[i];
    res.state &= r.usp == f.usp && r.ssp == f.ssp;
    res.state &= (r.sr & SR_MASK) == (f.sr & SR_MASK);
    res.state &= ((r.pc + pcOffset(t)) & ADDR_MASK) == (f.pc & ADDR_MASK);
    for (auto &[addr, val] : f.ram) res.state &= ram[addr & ADDR_MASK] == val;

    res.bus = busEqual(t, cpu.log(), why);
    if (!res.cycles && why.empty()) why = "cycles " + std::to_string(t.cycles) + "/" + std::to_string(cycles);
    return res;
}

std::string defaultDir()
{
    std::string file = __FILE__;
    return file.substr(0, file.find_last_of("/\\")) + "/../ThirdParty/SingleStepTests/m68000/v1";
}

}

int main(int argc, char **argv)
{
    std::string dir = defaultDir();
    std::vector<std::string> only;
    long limit = 0;
    int verbose = 0;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        bool more = i + 1 < argc;
        if (a == "--dir" && more) dir = argv[++i];
        else if (a == "--only" && more) only.push_back(argv[++i]);
        else if (a == "--limit" && more) limit = atol(argv[++i]);
        else if (a == "-v" && more) verbose = atoi(argv[++i]);
        else { printf("Usage: %s [--dir <folder>] [--only <text>]... [--limit <n>] [-v <n>]\n", argv[0]); return 1; }
    }

    std::vector<std::string> names;
    if (DIR *d = opendir(dir.c_str())) {
        while (auto *e = readdir(d)) {
            std::string n = e->d_name;
            if (n.size() > 9 && n.compare(n.size() - 9, 9, ".json.bin") == 0 &&
                (only.empty() || std::any_of(only.begin(), only.end(), [&](auto &s) { return n.find(s) != std::string::npos; })))
                names.push_back(n);
        }
        closedir(d);
    }
    std::sort(names.begin(), names.end());
    if (names.empty()) { printf("No test files found in %s\n", dir.c_str()); return 1; }

    mame::Cpu cpu;
    cpu.read = [](uint32_t addr, uint16_t) { return peek16(addr); };
    cpu.write = [](uint32_t addr, uint16_t val, uint16_t mask) {
        if (mask & 0xFF00) poke8(addr, uint8_t(val >> 8));
        if (mask & 0x00FF) poke8(addr + 1, uint8_t(val));
    };

    printf("MAME reference CPU, run against the SingleStepTests 68000 suite\n\n");
    printf("%-16s %9s %9s %9s %9s\n", "Test", "Count", "State", "Cycles", "Bus");

    long total = 0, okState = 0, okCycles = 0, okBus = 0;

    for (auto &name : names) {

        sst::File file;
        if (!file.open(dir + "/" + name)) { printf("Cannot read %s\n", name.c_str()); return 1; }

        long n = 0, s = 0, c = 0, b = 0;
        int shown = 0;
        sst::Test t;
        while (file.next(t)) {
            if (limit && n >= limit) break;
            std::string why;
            auto r = run(cpu, t, why);
            n++; s += r.state; c += r.cycles; b += r.bus;
            if ((!r.state || !r.cycles || !r.bus) && shown < verbose) {
                shown++;
                printf("    %s:%s%s%s %s\n", t.name.c_str(), r.state ? "" : " state", r.cycles ? "" : " cycles", r.bus ? "" : " bus", why.c_str());
                if (verbose > 2) {
                    int c = 0;
                    for (auto &b : t.bus) { printf("      exp %3d %d dur %d addr %06x data %04x fc %u\n", c, int(b.type), b.cycles, b.addr, b.data, b.fc); c += b.cycles; }
                    for (auto &b : cpu.log()) printf("      got %3d %c addr %06x data %04x mask %04x fc %d\n", b.cycle, b.write ? 'w' : 'r', b.addr, b.data, b.mask, b.fc);
                }
            }
        }
        printf("%-16s %9ld %8.2f%% %8.2f%% %8.2f%%\n", name.substr(0, name.size() - 9).c_str(), n,
               n ? 100.0 * s / n : 100.0, n ? 100.0 * c / n : 100.0, n ? 100.0 * b / n : 100.0);
        fflush(stdout);
        total += n; okState += s; okCycles += c; okBus += b;
    }

    printf("\n%-16s %9ld %8.2f%% %8.2f%% %8.2f%%\n", "TOTAL", total,
           total ? 100.0 * okState / total : 100.0, total ? 100.0 * okCycles / total : 100.0, total ? 100.0 * okBus / total : 100.0);
    return (okState == total && okCycles == total && okBus == total) ? 0 : 1;
}
