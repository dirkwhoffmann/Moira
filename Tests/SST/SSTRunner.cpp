// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "SSTRunner.h"
#include "Common.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace sstrunner {

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

struct Counter_unused {

    long cases = 0, ok = 0;
    std::map<std::string, long> signatures;
};

std::string words(const Memory &m, u32 sp, int n)
{
    char buf[16]; std::string s;
    for (int i = 0; i < n; i++) { snprintf(buf, sizeof buf, "%04x ", m.peek16(sp + 2 * i)); s += buf; }
    return s;
}

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

SSTRunner::SSTRunner(const CoreFactory &factory) : ref(factory)
{
    codeMask = ref.core->codeMask();
}

SSTRunner::~SSTRunner()
{
    if (tsv) fclose(tsv);
}

// Returns an empty string if the bus cycles match. A reference that doesn't log the aborted access
// of an address error (UAE) is compared without it.
std::string SSTRunner::compareBus(const sst::Test &t, const std::vector<mame::BusCycle> &got, bool dropAborted)
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
int SSTRunner::checkReference(const std::vector<std::string> &names)
{
    printf("Reference CPU (%s), run against the SingleStepTests 68000 suite\n\n", ref.core->shortName());
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
            auto busDiff = compareBus(t, ref.cpu.log(), !ref.core->logsAbortedAccess());
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

void SSTRunner::runCase(const sst::Test &t, const sst::State &init, int k, int accesses, const mame::BusCycle &pk, const char *kd, bool isFetch, bool bus, u16 plainSR, const std::string &label)
{

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
    bool regs = !(halted && !ref.core->comparesHaltedRegs());
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
    bool m68000 = model == CPUModel::M68000;
    u32 frameAt = rs.ssp & ADDR_MASK;
    bool format8 = !m68000 && (ref.mem.peek16(frameAt + 6) >> 12) == 8;
    std::set<std::string> badFields;
    for (auto a : addrs) {
        if (tasRead && (a & ~1u) == pk.addr) continue;
        if (extraWrites.count(a & ~1u)) continue;
        if ((a & ~1u) == aborted && aborted != ~0u && (!bus || halted)) continue;       // MAME may perform the aborted access    // MAME still completes the write half

        // The unused words and the internal information of a format 8 frame (68010) are undefined.
        // Moira writes zeros, UAE writes a counting pattern.
        if (format8 && a >= frameAt + 14 && a < frameAt + 58) {
            u32 off = a - frameAt;
            bool unused = off < 16 || (off >= 18 && off < 20) || (off >= 22 && off < 24) || (off >= 26 && off < 28);
            if (unused || off >= 28) continue;
        }
        if (model >= CPUModel::M68EC020) {

            // Frames of the 68020 (formats 0, 2, A, B): SR, PC, format word, and for the bus error frames the SSW, the pipe stages,
            // the fault address and the data output buffer. All other words are internal information.
            static const char *frame20[] = { "frame.sr", "frame.pc", "frame.pc", "frame.fmt", "frame.int", "frame.ssw", "frame.stagec", "frame.stageb",
                                             "frame.addr", "frame.addr", "frame.int", "frame.int", "frame.dob", "frame.dob" };
            if (moira.mem.peek(a) != ref.mem.peek(a)) {
                u32 off = a - frameAt;
                const char *field = a >= frameAt && off < 28 ? frame20[off / 2] : a >= frameAt && off < 92 ? "frame.int" : "mem";
                if (!strcmp(field, "mem") && getenv("EXC_REGS")) printf("      mem %06x: exp %02x got %02x (ssp %06x)\n", a, ref.mem.peek(a), moira.mem.peek(a), frameAt);
                if (ref.core->reliableFrameField(field)) badFields.insert(field);
            }
            continue;
        }
        if (!m68000) {
            if (moira.mem.peek(a) != ref.mem.peek(a)) {
                static const char *frame8[] = { "frame.sr", "frame.pc", "frame.pc", "frame.fmt", "frame.ssw", "frame.addr", "frame.addr", "frame.unused", "frame.dob", "frame.unused", "frame.dib", "frame.unused", "frame.iib" };
                u32 off = a - frameAt;
                const char *field = a >= frameAt && off < 26 && format8 ? frame8[off / 2] : a >= frameAt && off < 8 ? frame8[off / 2] : "mem";
                if (ref.core->reliableFrameField(field)) badFields.insert(field);
            }
            continue;
        }
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
    for (auto &f : badFields) bad.insert(f);
    for (auto &f : ignoredFields) bad.erase(f);
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
    if (tasRead || !ref.cpu.hasBusLog()) {}
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
            if (!d.empty() && getenv("EXC_BUS")) { for (size_t j = 0; j < mlog.size() || j < moira.log.size(); j++) { if (j < mlog.size()) printf("      ref   %2zu: cyc %3lld %c addr %06x fc %d\n", j, (long long)mlog[j].cycle, mlog[j].write ? 'W' : 'R', mlog[j].addr, mlog[j].fc); if (j < moira.log.size()) printf("      moira %2zu: cyc %3lld %c addr %06x fc %d\n", j, (long long)moira.log[j].cycle, moira.log[j].write ? 'W' : 'R', moira.log[j].addr, moira.log[j].fc); } }
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
            int nw = model >= CPUModel::M68EC020 ? 30 : 7;
            printf("        exp frame: %s\n", words(ref.mem, rs.ssp, nw).c_str());
            printf("        got frame: %s\n", words(moira.mem, rs.ssp, nw).c_str());
        }
    }
}

int SSTRunner::run(int argc, char **argv)
{
    {
        std::string file = __FILE__;
        opt.dir = file.substr(0, file.find_last_of("/\\")) + "/../ThirdParty/SingleStepTests/m68000/v1";
    }
    if (const char *env = getenv("SST_DIR")) opt.dir = env;

    common::Args args(argc, argv);
    args.text("--dir", opt.dir, "Folder with the test files (default: $SST_DIR or the SingleStepTests checkout)", "<folder>");
    args.list("--only", opt.only, "Only run files whose name contains <text>. May be repeated.");
    args.integer("--limit", opt.limit, "Run at most <n> cases per file");
    args.text("--tsv", opt.tsv, "Write one line per failing case to <file>", "<file>");
    args.integer("--dedupe", opt.dedupe, "Report at most <n> failures per distinct case");
    args.flag("--coverage", opt.coverage, "List instruction forms without an address error case");
    args.flag("--check", opt.check, "Check the reference against the recorded results");
    args.number("--min", opt.minPercent, "Exit with an error if less than <x> percent match");
    args.custom("--ignore", "<list>", [&](const char *s) {
        std::string list = "," + std::string(s) + ",";
        if (list.find(",ccr,") != std::string::npos) srMask = 0xA700;
        if (list.find(",cycles,") != std::string::npos) ignoreCycles = true;
        if (list.find(",nbit,") != std::string::npos) codeMask = 0xFFF7;
        for (size_t p = 0; (p = list.find(",frame.", p)) != std::string::npos; ) {
            size_t q = list.find(',', p + 1);
            ignoredFields.insert(list.substr(p + 1, q - p - 1));
            p = q;
        }
        return true;
    }, "Ignore differences (comma-separated: ccr, cycles, nbit, or a frame field such as frame.addr)");
    args.integer("-v", opt.verbose, "Print the first <n> failures of each file");
    std::string models;
    for (auto m : common::allCpuModels()) if (ref.core->supports(m) && acceptsModel(m)) models += std::string(" ") + common::modelName(m);
    args.custom("--cpu", "<model>", [&](const char *s) {
        auto m = common::parseModel(s);
        if (!m || !ref.core->supports(*m) || !acceptsModel(*m)) return false;
        model = *m;
        return true;
    }, ("CPU model (default: 68000). Only the 68000 is compared cycle by cycle.\nModels:" + models).c_str());
    if (!args.parse()) return 1;

    // The test data has been recorded on a 68000. For other models, the instructions and initial
    // states are used as input only, and registers and memory are compared.
    if (model != CPUModel::M68000) {

        if (opt.check) { printf("--check requires the 68000\n"); return 1; }
        ref.core->setModel(model);
        moira.setModel(model);
        ref.mem.prefetchInRam = moira.mem.prefetchInRam = true;
        ignoreCycles = true;
    }

    std::vector<std::string> names;
    std::error_code ec;
    for (auto &e : std::filesystem::directory_iterator(opt.dir, ec)) {
        std::string n = e.path().filename().string();
        if (n.size() > 9 && n.compare(n.size() - 9, 9, ".json.bin") == 0 &&
            (opt.only.empty() || std::any_of(opt.only.begin(), opt.only.end(), [&](auto &s) { return n.find(s) != std::string::npos; })))
            names.push_back(n);
    }
    std::sort(names.begin(), names.end());
    if (names.empty()) { printf("No test files found in %s\n", opt.dir.c_str()); return 1; }


    common::printBanner(description());
    common::printSetting("Core under test", "Moira %s (%s mode)", moira::Moira::build().c_str(),
                         moira::compat == moira::Compatibility::UAE ? "UAE" : moira::compat == moira::Compatibility::MUSASHI ? "Musashi" : "MAME");
    common::printSetting("Reference", "%s", ref.core->name());
    common::printSetting("CPU model", "%s", common::modelName(model));
    printf("\n");
    printf("%-16s %8s %8s %8s\n", "Instruction", "Cases", "OK", "OK%");

    if (!opt.tsv.empty()) tsv = fopen(opt.tsv.c_str(), "w");
    if (opt.check) { ref.mem.installVectors = false; return checkReference(names); }
    long total = 0, totalOk = 0;

    for (auto &name : names) {

        sst::File file;
        if (!file.open(opt.dir + "/" + name)) { printf("Cannot read %s\n", name.c_str()); return 1; }

        c = Counter();
        failedReads = failedWrites = 0;
        shown = 0;
        sst::Test t;

        for (long n = 0; file.next(t); n++) {

            if (opt.limit && n >= opt.limit) break;
            if (!accepts(t)) continue;
            allForms.insert(formOf(t.name));

            runTest(t);
        }

        printf("%-16s %8ld %8ld %7.2f%%", name.substr(0, name.size() - 9).c_str(), c.cases, c.ok,
               c.cases ? 100.0 * c.ok / c.cases : 100.0);
        if (failedReads || failedWrites) printf("  (%ld read errors, %ld write errors)", failedReads, failedWrites);
        printf("\n");
        fflush(stdout);
        total += c.cases; totalOk += c.ok;
    }

    if (skipped) printf("\nSkipped %ld cases %s\n", skipped, skipNote.c_str());
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

    return (total ? 100.0 * totalOk / total : 100.0) >= opt.minPercent ? 0 : 1;
}

}
