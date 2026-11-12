// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "SSTRunner.h"
#include <algorithm>
#include <map>

namespace sstrunner {

RetryRunner::RetryRunner(const CoreFactory &factory) : SSTRunner(factory)
{
}

const char *RetryRunner::description() const
{
    return "Bus error recovery: Lets a bus error hit every data access of an instruction, returns\n"
           "from the handler with RTE (the cause of the fault is gone) and checks that the\n"
           "instruction completes with the same registers, status register and memory as an\n"
           "undisturbed run. This is the page fault scenario of an MMU. Moira restarts the\n"
           "instruction of every 68020 and above.\n"
           "The reference CPU is not used.\n";
}

namespace {

const char *SKIP_NOTE = "that cannot be compared (they raise an exception, touch the exception frame or the vector, or fault in the middle of a long write)";

struct Result {

    u32 d[8], a[7], usp, isp, pc;
    u16 sr;
    std::map<u32, u8> mem;
};

}

void RetryRunner::runTest(const sst::Test &t)
{
    auto prepare = [&]() {
        moira.load(t.initial);
        
        moira.wideBus = true;

        // The handler of the bus error is a single RTE
        moira.mem.poke(HANDLER, 0x4E);
        moira.mem.poke(HANDLER + 1, 0x73);
    };
    auto result = [&]() {
        Result r;
        for (int i = 0; i < 8; i++) r.d[i] = moira.getD(i);
        for (int i = 0; i < 7; i++) r.a[i] = moira.getA(i);
        r.usp = moira.getUSP();
        r.isp = moira.getISP();
        r.pc = moira.getPC0();
        r.sr = u16(moira.getSR());
        for (u32 a : moira.mem.touched) r.mem[a] = moira.mem.peek(a);
        return r;
    };

    // Undisturbed run
    prepare();
    moira.logging = true;
    moira.execute();
    moira.logging = false;
    if (moira.exceptions) { skipped++; skipNote = SKIP_NOTE; return; }
    int accesses = moira.fault.count;

    // The bus error frame overwrites operands and instruction words that live in the memory below the supervisor stack pointer
    {
        u32 top = t.initial.ssp & ADDR_MASK;
        if (HANDLER + 2 > top - 0x60 && HANDLER < top) { skipped++; skipNote = SKIP_NOTE; return; }
        for (auto &a : moira.log) {
            bool vector = a.addr < 0x10;        // The bus error vector
            if (vector || (a.addr + 2 > top - 0x60 && a.addr < top + 8)) { skipped++; skipNote = SKIP_NOTE; return; }
        }
    }
    Result clean = result();

    for (int k = 0; k < accesses; k++) {

        prepare();
        u32 ssp0 = moira.getISP();
        moira.fault.index = k;
        moira.logging = true;
        moira.execute();
        moira.logging = false;

        // Did the access fail? The handler has been entered if the PC is there.
        if (moira.getPC0() != HANDLER) continue;
        bool write = moira.lastFaultWrite;
        u32 ssp1 = moira.getISP();

        // A fault in the second half of a long write leaves the first half in memory. Instructions that read
        // the destination first (NOT, ADD, ...) cannot be restarted then. An MMU does not fault in the middle
        // of an aligned operand, so these cases are of no interest.
        size_t fi = moira.faultLogIndex;
        if (write && fi > 0 && fi < moira.log.size() && moira.log[fi - 1].write && moira.log[fi - 1].addr + 2 == moira.log[fi].addr) { skipped++; skipNote = SKIP_NOTE; continue; }

        // Return from the handler and let the instruction finish
        moira.fault.index = -1;
        bool dbg = getenv("RETRY_DEBUG") && opt.verbose && shown < opt.verbose;
        if (dbg) {
            printf("      %s fault #%d: start pc %06x sp %06x, frame:", t.name.c_str(), k, t.initial.pc - 4, moira.getSP());
            for (int i = 0; i < 12; i++) printf(" %04x", moira.mem.peek16(moira.getSP() + 2 * i));
            printf("\n");
        }
        moira.execute();
        if (dbg) printf("      after RTE: pc0 %06x sp %06x\n", moira.getPC0(), moira.getSP());
        moira.execute();
        if (dbg) printf("      after retry: pc0 %06x sp %06x (clean pc0 %06x)\n", moira.getPC0(), moira.getSP(), clean.pc);
        Result got = result();

        // The frame is garbage on the stack. Its position is known.
        std::string diff;
        auto add = [&](const std::string &s) { diff += (diff.empty() ? "" : " ") + s; };
        for (int i = 0; i < 8; i++) if (got.d[i] != clean.d[i]) add("D" + std::to_string(i));
        for (int i = 0; i < 7; i++) if (got.a[i] != clean.a[i]) add("A" + std::to_string(i));
        if (got.usp != clean.usp) add("USP");
        if (got.isp != clean.isp) add("ISP");
        if (got.pc != clean.pc) add("PC");
        if ((got.sr ^ clean.sr) & srMask) add("SR");
        bool memDiff = false;
        std::map<u32, u8> all = got.mem;
        for (auto &[a, v] : clean.mem) all.emplace(a, 0);
        for (auto &[a, v] : all) {
            if (a >= ((ssp1 - 4) & ADDR_MASK) && a < (ssp0 & ADDR_MASK)) continue;
            u8 g = got.mem.count(a) ? got.mem[a] : 0, e = clean.mem.count(a) ? clean.mem[a] : 0;
            if (g != e && !(got.mem.count(a) == 0 || clean.mem.count(a) == 0)) { memDiff = true; if (getenv("RETRY_DEBUG")) printf("      mem %06x: clean %02x got %02x (ssp0 %06x ssp1 %06x)\n", a, e, g, ssp0, ssp1); }
        }
        if (memDiff) add("mem");

        const char *kd = write ? "write" : "read";
        if (tsv) fprintf(tsv, "%s\t%d\t%d\t%s\t%s\t%s\n", t.name.c_str(), k, accesses, kd, diff.empty() ? "ok" : "FAIL", diff.c_str());
        c.cases++;
        byKind[kd].cases++;
        keyCount[formOf(t.name) + " #" + std::to_string(k) + " " + kd]++;
        if (diff.empty()) { c.ok++; byKind[kd].ok++; continue; }

        (write ? failedWrites : failedReads)++;
        signatures[diff]++;
        byKind[kd].signatures[diff]++;

        if (shown < opt.verbose) {
            shown++;
            printf("    %s  fault #%d (%s): %s\n", t.name.c_str(), k, kd, diff.c_str());
        }
    }
}

}
