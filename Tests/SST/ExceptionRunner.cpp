// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "SSTRunner.h"
#include <algorithm>
#include <cstring>

namespace sstrunner {

ExceptionRunner::ExceptionRunner(const CoreFactory &factory) : SSTRunner(factory)
{
    skipNote = "that fault while a group 1/2 exception is processed (UAE and MAME disagree on these frames)";
}

const char *ExceptionRunner::description() const
{
    return "Compares the exception processing of Moira (address errors, bus errors, traps,\n"
           "etc.) with a reference CPU and checks registers, stack frames and bus cycles.\n"
           "The 68020 is compared without timing. Bus errors are injected into the data\n"
           "accesses of the instruction (one per operand access, no instruction fetches).\n"
           "Only the SR, PC, format word and fault address of its frames are comparable.\n";
}

// Address errors that are part of the test itself are covered by the plain runner
bool ExceptionRunner::accepts(const sst::Test &t) const
{
    return !expectsAddressError(t);
}

// The 68020 decodes extension words with bit 8 set as full format (memory indirect addressing). The
// 68000 test data contains such words by chance and the cores disagree on invalid combinations.
static bool hasFullFormatExtension(const sst::State &t)
{

    u16 ird = t.prefetch[0];
    auto indexed = [](u16 m, u16 r) { return m == 6 || (m == 7 && r == 3); };
    auto word = [&](u32 addr) -> u16 {
        u16 w = 0;
        for (auto &[a, v] : t.ram) { if (a == addr) w |= u16(v) << 8; if (a == addr + 1) w |= v; }
        return w;
    };

    // Position of the extension word: behind IRC, or behind the immediate data or register mask
    int skip = 0;
    switch (ird >> 12) {
        case 0x0: skip = (ird & 0x0100) ? 0 : (ird & 0x0F00) == 0x0800 ? 1 : ((ird >> 6) & 3) == 2 ? 2 : 1; break;
        case 0x4: if ((ird & 0xFB80) == 0x4880) skip = 1; break;
    }
    bool hasEA = true;
    switch (ird >> 12) {
        case 0x6: case 0x7: case 0xA: case 0xF: hasEA = false; break;
        case 0xE: hasEA = ((ird >> 6) & 3) == 3; break;
        case 0x4: hasEA = (ird & 0xFF80) != 0x4E00; break;
    }
    auto extAt = [&](int words) { return words == 0 ? t.prefetch[1] : word(t.pc + 2 * (words - 1)); };
    bool full = false;
    if (hasEA && indexed((ird >> 3) & 7, ird & 7)) full = extAt(skip) & 0x100;

    // MOVE: the destination extension word follows the one of the source
    if ((ird & 0xC000) == 0 && (ird & 0x3000) && ((ird >> 6) & 7) == 6) {
        int srcWords = 0;
        u16 m = (ird >> 3) & 7, r = ird & 7;
        if (m == 5 || (m == 7 && (r == 0 || r == 2))) srcWords = 1;
        else if (m == 6 || (m == 7 && r == 3)) srcWords = 1;
        else if (m == 7 && r == 1) srcWords = 2;
        else if (m == 7 && r == 4) srcWords = (ird >> 12) == 2 ? 2 : 1;
        if (extAt(srcWords) & 0x100) full = true;
    }
    return full;
}

void ExceptionRunner::runTest(const sst::Test &t)
{
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
                runCase(t, v.state, -1, 0, none, "halt", false, false, 0, v.name);
                continue;
            }
            if (model >= CPUModel::M68EC020 && hasFullFormatExtension(v.state)) continue;
            auto log = ref.cpu.log();

            // Did the instruction end in an address error? (vector 3 is read)
            auto rs = ref.cpu.state();
            bool ae = std::any_of(log.begin(), log.end(), [](auto &c) { return !c.write && c.fc == 5 && c.addr == 12; });

            // The 68020 reports vector 3 in the format word of the frame. It only detects faulty instruction fetches.
            if (model >= CPUModel::M68EC020) {
                u32 oldSsp = (v.state.sr & 0x2000) ? v.state.ssp : 0;
                ae = rs.ssp != oldSsp && (ref.mem.peek16(rs.ssp + 6) & 0xFFF) == 0x00C;
            }
            if (!ae) continue;

            // Classify the failing access by the function code and the R/W bit of the stacked frame
            u16 code = ref.mem.peek16(rs.ssp);
            const char *kd = ((code & 7) == 2 || (code & 7) == 6) ? "ae-fetch" : (code & 0x10) ? "ae-read" : "ae-write";
            if (model >= CPUModel::M68EC020) kd = "ae-fetch";
            bool isFetch = !strcmp(kd, "ae-fetch");
            mame::BusCycle none { 0, false, 0, 0, 0, 0 };
            runCase(t, v.state, -1, 0, none, kd, isFetch, false, ref.cpu.state().sr, v.name);
        }
    }

    if (model >= CPUModel::M68EC020 && hasFullFormatExtension(t.initial)) { skipped++; skipNote = "that use a 68020 full format extension word (the cores disagree on invalid ones)"; return; }

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
            runCase(t, st, -1, 0, none, "halt", false, true, 0, pass ? " [SSP^1]" : "");
            continue;
        }
        int accesses = int(ref.cpu.log().size());
        auto plain = ref.cpu.log();
        u16 plainSR = ref.cpu.state().sr;

        // Find the exception vector fetch of instructions that raise an exception themselves
        int vectorFetch = -1;
        for (int k = 0; k < accesses && vectorFetch < 0; k++) if (!plain[k].write && plain[k].addr < 0x100 && plain[k].fc == 5) vectorFetch = k;

        // The 68020 reference does not model the stacking of exceptions. Only the accesses of the instruction are failed.
        int limit = accesses;
        if (model >= CPUModel::M68EC020 && vectorFetch >= 0) {
            limit = vectorFetch;
            u32 frame = ref.cpu.state().ssp & ADDR_MASK;
            for (int k = 0; k < accesses; k++) {
                if (plain[k].write && plain[k].fc == 5 && plain[k].addr >= frame && plain[k].addr < frame + 92) { limit = std::min(limit, k); break; }
            }
        }

        for (int k = 0; k < limit; k++) {

            // UAE and MAME disagree on the frames of bus errors that hit the stacking, the vector fetch, or the first fetches of the handler of a group 1 or 2 exception
            if (model < CPUModel::M68EC020 && ref.core->disagreesOnStacking() && vectorFetch >= 3 && k >= vectorFetch - 3) { skipped++; continue; }

            const char *kd = kind(plain[k]);
            bool isFetch = !strcmp(kd, "fetch");
            runCase(t, st, k, accesses, plain[k], kd, isFetch, true, plainSR, pass ? " [SSP^1]" : "");
        }
    }
}

}
