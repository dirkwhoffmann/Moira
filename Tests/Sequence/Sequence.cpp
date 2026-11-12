// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/* Differential sequence test. All other tools execute a single instruction from
 * a prepared state. This tool runs whole programs and lets events happen between
 * the instructions, which exercises the interplay of
 *
 *   - interrupts (all levels, masking, level 7, acknowledge),
 *   - the trace bit (T1), also in combination with exceptions and interrupts,
 *   - STOP and the wake-up from it,
 *   - writes to the status register (MOVE, ANDI, ORI, EORI to SR) including the
 *     switch to user mode, privilege violations, USP accesses,
 *   - TRAP, TRAPV, CHK, DIVU/DIVS by zero, illegal and line A/F opcodes,
 *   - RTE with arbitrary frames.
 *
 * A program is a random mix of such instructions. Between the instructions, the
 * IPL pins change according to a random schedule. Moira and a reference core (UAE or MAME) run
 * the program in lock step. After each step (an instruction plus the exceptions
 * that are due after it) the registers, the status register, the stack pointers,
 * the stopped state and all memory writes are compared.
 *
 * Interrupts are recognized at the end of an instruction (the IPL pins are polled
 * by the instruction). Cycle timing is not compared.
 */

#include "Moira.h"
#include "Sequence.h"
#include "Common.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace moira;
using common::portableRand;

using namespace sequence;

namespace {

//
// Moira
//

class SeqMoira : public Moira {

public:

    std::vector<u8> mem = std::vector<u8>(MEM_MASK + 1);
    mutable std::vector<Write> writes;
    bool interrupted = false;

    SeqMoira() { setModel(CPUModel::M68000); }

    u8 read8(u32 a) const override { return mem[a & MEM_MASK]; }
    u16 read16(u32 a) const override { return u16(mem[a & MEM_MASK] << 8 | mem[(a + 1) & MEM_MASK]); }
    u32 read32(u32 a) const override { return u32(read16(a)) << 16 | read16(a + 2); }
    void put8(u32 a, u8 v) const { writes.push_back({ a & MEM_MASK, v }); const_cast<SeqMoira *>(this)->mem[a & MEM_MASK] = v; }
    void write8(u32 a, u8 v) const override { put8(a, v); }
    void write16(u32 a, u16 v) const override { put8(a, u8(v >> 8)); put8(a + 1, u8(v)); }
    void write32(u32 a, u32 v) const override { write16(a, u16(v >> 16)); write16(a + 2, u16(v)); }

    // The device drops the IPL pins when the CPU acknowledges the interrupt (before the vector is fetched)
    bool ack = false;
    void willInterrupt(u8 level) override { interrupted = true; if (ack) setIPL(0); }

    bool pending() const
    {
        if (isHalted()) return false;
        if ((flags & State::TRACE_EXC) || ((flags & State::CHECK_IRQ) && (reg.ipl > reg.sr.ipl || (reg.ipl == 7 && !nmiTaken)))) return true;

        // A stopped CPU polls the IPL pins and wakes up as soon as the interrupt is accepted
        return stopped() && (ipl > reg.sr.ipl || (ipl == 7 && !nmiTaken));
    }

    bool stopped() const { return flags & State::STOPPED; }

    // One step: an instruction and the exceptions that are due after it
    void step()
    {
        writes.clear();
        interrupted = false;
        execute();
        for (int i = 0; i < 8 && pending(); i++) execute();
    }
};

//
// Programs
//

u32 rnd(u32 n) { return n ? u32(portableRand()) % n : 0; }

u16 randomSR()
{
    u16 sr = u16(rnd(32));                          // CCR
    sr |= u16(rnd(8) << 8);                         // Interrupt mask
    if (rnd(100) < 75) sr |= 0x2000;                // Supervisor mode
    if (rnd(100) < 30) sr |= 0x8000;                // Trace bit
    return sr;
}

bool noChk = false;
bool userStop = false;      // Allow STOP to clear the supervisor bit?

Program generate()
{
    Program p;
    auto &w = p.words;
    auto emit = [&](u16 a) { w.push_back(a); };

    for (auto &d : p.d) d = u32(portableRand()) << 1 ^ u32(portableRand());
    for (int i = 0; i < 4; i++) p.a[i] = DATA + 0x20 * i;
    for (int i = 4; i < 7; i++) p.a[i] = u32(portableRand());
    p.sr = (rnd(100) < 85 ? 0x2000 : 0) | (rnd(100) < 25 ? 0x8000 : 0) | (rnd(8) << 8) | rnd(32);
    p.ack = rnd(100) < 70;

    int n = 20 + int(rnd(40));
    for (int i = 0; i < n; i++) {

        u16 x = u16(rnd(8)), y = u16(rnd(8)), q = u16(rnd(8));
        u16 imm = randomSR();

        switch (rnd(40)) {

            case 0: case 1: emit(0x7000 | x << 9 | rnd(256)); break;                   // moveq
            case 2: emit(0x5040 | q << 9 | x); break;                                  // addq.w
            case 3: emit(0x5140 | q << 9 | x); break;                                  // subq.w
            case 4: emit(0xD040 | x << 9 | y); break;                                  // add.w
            case 5: emit(0x9080 | x << 9 | y); break;                                  // sub.l
            case 6: emit(0xC040 | x << 9 | y); break;                                  // and.w
            case 7: emit(0x8080 | x << 9 | y); break;                                  // or.l
            case 8: emit(0xB140 | x << 9 | y); break;                                  // eor.w
            case 9: emit(0xB040 | x << 9 | y); break;                                  // cmp.w
            case 10: emit(0x4A40 | x); break;                                          // tst.w
            case 11: emit(0x4440 | x); break;                                          // neg.w
            case 12: emit(0x4840 | x); break;                                          // swap
            case 13: emit(0xE148 | q << 9 | x); break;                                 // lsl.w
            case 14: case 15: emit(0x2080 | (y & 3) << 9 | x); break;                  // move.l dx,(ay)
            case 16: emit(0x4E71); break;                                              // nop
            case 17: emit(0x40C0 | x); break;                                          // move sr,dx
            case 18: emit(0x44C0 | x); break;                                          // move dx,ccr
            case 19: case 20: emit(0x46FC); emit(imm); break;                          // move #imm,sr
            case 21: emit(0x007C); emit(imm & 0xA71F); break;                          // ori #imm,sr
            case 22: emit(0x027C); emit(imm | 0x00FF); break;                          // andi #imm,sr
            case 23: emit(0x0A7C); emit(imm & 0x871F); break;                          // eori #imm,sr
            case 24: emit(0x46C0 | x); break;                                          // move dx,sr
            case 25: emit(0x4E60 | (y & 3)); emit(0x4E71); break;                      // move ay,usp
            case 26: emit(0x4E68 | (y & 3)); emit(0x4E71); break;                      // move usp,ay
            case 27: emit(0x4E72); emit((imm & 0xA71F) | (userStop ? 0 : 0x2000)); break;                          // stop #imm
            case 28: emit(0x4E40 | rnd(16)); break;                                    // trap #n
            case 29: emit(0x4E76); break;                                              // trapv
            case 30: if (noChk) emit(0x4E71); else emit(0x4180 | x << 9 | y); break;                                 // chk.w dy,dx
            case 31: emit(0x80C0 | x << 9 | y); break;                                 // divu.w dy,dx
            case 32: emit(0x81C0 | x << 9 | y); break;                                 // divs.w dy,dx
            case 33: emit(0x4AFC); emit(0x4E71); break;                                // illegal
            case 34: emit(0xA000 | rnd(0x1000)); emit(0x4E71); break;                  // line A
            case 35: emit(0xF000 | rnd(0x1000)); emit(0x4E71); break;                  // line F
            case 36: emit(0x4E70); emit(0x4E71); break;                                // reset
            case 37: case 38: case 39:                                                 // rte with a frame
            {
                emit(0x487A); emit(10);                                                 // pea next(pc)
                emit(0x3F3C); emit(imm);                                                // move.w #imm,-(a7)
                emit(0x4E73); emit(0x4E71);                                             // rte
                break;
            }
        }
    }
    emit(0x60FE);                                                                       // bra.s *

    // Schedule IPL changes
    int steps = int(w.size());
    for (int e = 0, ne = 2 + int(rnd(6)); e < ne; e++) {
        int level = rnd(100) < 10 ? 7 : int(rnd(7));
        p.events.push_back({ int(rnd(unsigned(steps) + 20)), level });
    }
    std::sort(p.events.begin(), p.events.end());
    return p;
}

//
// Runner
//

void put16(std::vector<u8> &m, u32 a, u16 v) { m[a] = u8(v >> 8); m[a + 1] = u8(v); }
void put32(std::vector<u8> &m, u32 a, u32 v) { put16(m, a, u16(v >> 16)); put16(m, a + 2, u16(v)); }

void installSystem(std::vector<u8> &m)
{
    std::fill(m.begin(), m.end(), 0);

    for (int v = 2; v < 256; v++) put32(m, 4 * v, H_RTE);
    for (int v : { 2, 3 }) put32(m, 4 * v, H_LOOP);
    for (int v : { 4, 8, 10, 11 }) put32(m, 4 * v, H_SKIP);

    // addq.l #4,2(a7) / rte
    put16(m, H_SKIP, 0x58AF); put16(m, H_SKIP + 2, 2); put16(m, H_SKIP + 4, 0x4E73);
    put16(m, H_RTE, 0x4E73);
    put16(m, H_LOOP, 0x60FE);

    // UAE reads the vector number of an interrupt from the bus (autovector). MAME uses a callback.
    for (int level = 0; level < 8; level++) m[(0xFFFFF1 | level << 1) & MEM_MASK] = u8(24 + level);
}

struct Diff { std::string what; };

} // namespace

int sequence::run(int argc, char **argv, Reference &ref)
{
    long seed = common::defaultSeed(), programs = 2000, maxSteps = 120, verbose = 0;
    std::vector<std::string> cpus;
    bool noAck = false;
    long only = -1;
    double minPercent = 0;

    common::Args args(argc, argv);
    args.integer("--seed", seed, "Seed of the random number generator (default: derived from the date)");
    args.integer("--programs", programs, "Number of programs per CPU model");
    args.integer("--steps", maxSteps, "Maximum number of steps per program");
    args.list("--cpu", cpus, "CPU model (68000, 68010; default: all models the reference supports)", "<model>");
    args.integer("-v", verbose, "Print the first <n> failures with the program");
    args.number("--min", minPercent, "Exit with an error if less than <x> percent of the programs match");
    args.integer("--program", only, "Run only the program with this number (and print it with -v)");
    args.flag("--userstop", userStop, "Let STOP clear the supervisor bit (the CPU then stays stopped in user mode until an interrupt wakes it)");
    args.flag("--nochk", noChk, "Do not generate CHK");
    args.flag("--noack", noAck, "Never drop the IPL pins when an interrupt is taken");
    if (!args.parse()) return 1;

    if (cpus.empty()) for (const char *n : { "68000", "68010" }) if (ref.init(n)) cpus.push_back(n);

    std::string lower = ref.name();
    for (auto &ch : lower) ch = char(tolower(ch));
    std::string title = std::string("Sequence test: Moira vs. ") + ref.name() + " (interrupts, trace, STOP, RTE)";
    common::printBanner(title.c_str());
    common::printSetting("seed", "%ld", seed);
    common::printSetting("programs", "%ld", programs);
    printf("\n");

    long total = 0, failed = 0, addressErrors = 0, unreliable = 0;
    long printed = 0;
    std::map<std::string, long> histogram;

    for (auto &name : cpus) {

        CPUModel model;
        if (name == "68000") model = CPUModel::M68000;
        else if (name == "68010") model = CPUModel::M68010;
        else { printf("Unsupported model %s\n", name.c_str()); return 1; }
        if (!ref.init(name)) { printf("Unsupported model %s\n", name.c_str()); return 1; }

        common::portableSrand(unsigned(seed));
        long modelFailed = 0;
        common::Progress progress;
        printf("%11s : ", name.c_str()); fflush(stdout);

        SeqMoira moira;
        moira.setModel(model);
        moira.setSR(0x2700);   // Required by setISP

        for (long pn = 0; pn < programs; pn++) {

            progress.update(100.0 * double(pn) / double(programs));
            Program prog = generate();
            if (only >= 0 && pn != only) continue;

            // Build the memory image
            installSystem(moira.mem);
            for (size_t i = 0; i < prog.words.size(); i++) put16(moira.mem, PROG + 2 * u32(i), prog.words[i]);

            // Initialize Moira
            moira.reset();
            moira.setSR(prog.sr & 0xA71F | 0x2000);
            moira.setISP(SSP);
            moira.setUSP(USP);
            moira.setSR(u16(prog.sr));
            for (int i = 0; i < 8; i++) moira.setD(i, prog.d[i]);
            for (int i = 0; i < 7; i++) moira.setA(i, prog.a[i]);
            moira.setPC(PROG); moira.setPC0(PROG);
            moira.setIRD(moira.read16(PROG)); moira.setIRC(moira.read16(PROG + 2));
            moira.setIPL(0);
            moira.ack = prog.ack && !noAck;
            moira.setClock(0);

            // Initialize the reference core
            ref.load(moira.mem, prog);

            std::string fail;
            std::string trace;
            size_t ev = 0;
            int pin = 0;

            for (long step = 0; step < maxSteps && fail.empty(); step++) {

                // Apply IPL events
                while (ev < prog.events.size() && prog.events[ev].first <= step) {
                    pin = prog.events[ev++].second;
                    moira.setIPL(u8(pin)); ref.setIPL(pin);
                }

                u32 pcBefore = moira.getPC0();
                u16 srBefore = moira.getSR();
                std::string text = ref.disassemble(pcBefore);
                if (text.empty()) { char buf[128]; moira.disassemble(buf, pcBefore); text = buf; }
                moira.step();
                ref.step();
                if (ref.unreliable()) { unreliable++; break; }

                // Acknowledge the interrupt
                bool refIrq = ref.interrupted();
                if (prog.ack && !noAck && (moira.interrupted || refIrq)) {
                    pin = 0; moira.setIPL(0); ref.setIPL(0);
                }

                // Compare
                RefState u = ref.state();
                std::string diff;
                auto cmp = [&](const char *field, u32 a, u32 b) {
                    if (a != b) { char buf[64]; snprintf(buf, sizeof buf, "%s moira=%08X %s=%08X ", field, a, lower.c_str(), b); diff += buf; }
                };
                for (int i = 0; i < 8; i++) { char f[4] = { 'D', char('0' + i), 0 }; cmp(f, moira.getD(i), u.d[i]); }
                for (int i = 0; i < 7; i++) { char f[4] = { 'A', char('0' + i), 0 }; cmp(f, moira.getA(i), u.a[i]); }
                cmp("SR", moira.getSR(), u.sr);
                cmp("PC", moira.getPC0(), u.pc);
                cmp("SP", moira.getSP(), u.sp);
                cmp("USP", moira.getUSP(), u.usp);
                cmp("ISP", moira.getISP(), u.isp);
                cmp("STOP", moira.stopped(), ref.stopped());
                cmp("IRQ", moira.interrupted, refIrq);

                // Address errors are covered by the exception tests. The frames of the 68010 differ between UAE and the
                // real hardware, so the program ends here and the frame is not compared.
                bool addressError = ref.addressError();

                std::vector<Write> a = moira.writes, b = ref.writes();
                std::sort(a.begin(), a.end()); std::sort(b.begin(), b.end());
                if (a != b && !addressError) {
                    diff += "writes ";
                    for (auto &w : a) { char buf[32]; snprintf(buf, sizeof buf, "%05X:%02X ", w.addr, w.value); diff += buf; }
                    diff += "| ";
                    for (auto &w : b) { char buf[32]; snprintf(buf, sizeof buf, "%05X:%02X ", w.addr, w.value); diff += buf; }
                }

                char line[256];
                snprintf(line, sizeof line, "  step %3ld  ipl=%d  pc=%05X sr=%04X  %-28s -> moira pc=%05X sr=%04X sp=%05X | %s pc=%05X sr=%04X sp=%05X%s\n",
                         step, pin, pcBefore, srBefore, text.c_str(), moira.getPC0(), moira.getSR(), moira.getSP(), lower.c_str(), u.pc, u.sr, u.sp,
                         moira.interrupted ? "  [moira irq]" : "");
                trace += line;

                if (!diff.empty()) {
                    fail = diff;

                    // Classify: the instruction and the differing fields
                    std::string key;
                    std::string t = text;
                    key = t.substr(0, t.find(' ')) + " : ";
                    for (const char *f : { "D0 ", "D1 ", "D2 ", "D3 ", "D4 ", "D5 ", "D6 ", "D7 ", "A0 ", "A1 ", "A2 ", "A3 ", "A4 ", "A5 ", "A6 ", "SR ", "PC ", "SP ", "USP ", "ISP ", "STOP ", "IRQ ", "writes " })
                        if (diff.find(f) != std::string::npos) key += std::string(f);
                    histogram[key]++;
                }

                if (addressError) { addressErrors++; break; }

                // Finish if the CPU does not move anymore
                if (fail.empty() && moira.stopped() && ref.stopped() && ev >= prog.events.size()) break;
            }

            total++;
            if (!fail.empty()) {
                failed++; modelFailed++;
                if (printed < verbose) {
                    printed++;
                    printf("\nFAIL (%s, program %ld, seed %ld, ack %d, start SR %04X)\n%s  => %s\n", name.c_str(), pn, seed, prog.ack, prog.sr,
                           trace.c_str(), fail.c_str());
                }
            }
        }
        progress.finish();
        printf(" %ld of %ld programs differ\n", modelFailed, programs);
    }

    if (addressErrors) printf("\n%ld programs ended with an address error (not compared)\n", addressErrors);
    if (unreliable) printf("%ld programs ended because the reference core ran into a known defect (not compared)\n", unreliable);
    printf("\n%ld of %ld programs match (%.2f%%)\n", total - failed, total, total ? 100.0 * double(total - failed) / double(total) : 100.0);

    if (!histogram.empty()) {
        std::vector<std::pair<long, std::string>> sorted;
        for (auto &h : histogram) sorted.push_back({ h.second, h.first });
        std::sort(sorted.rbegin(), sorted.rend());
        printf("\nMost frequent differences (instruction : fields):\n");
        for (size_t i = 0; i < sorted.size() && i < 25; i++) printf("%10ld  %s\n", sorted[i].first, sorted[i].second.c_str());
    }

    double percent = total ? 100.0 * double(total - failed) / double(total) : 100.0;
    return percent < minPercent || (minPercent == 0 && failed) ? 1 : 0;
}
