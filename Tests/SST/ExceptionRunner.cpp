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
           "etc.) with a reference 68000 and checks registers, stack frames and bus cycles.\n";
}

// Address errors that are part of the test itself are covered by the plain runner
bool ExceptionRunner::accepts(const sst::Test &t) const
{
    return !expectsAddressError(t);
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
            runCase(t, v.state, -1, 0, none, kd, isFetch, false, ref.cpu.state().sr, v.name);
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
            runCase(t, st, -1, 0, none, "halt", false, true, 0, pass ? " [SSP^1]" : "");
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
            if (ref.core->disagreesOnStacking() && vectorFetch >= 3 && k >= vectorFetch - 3) { skipped++; continue; }

            const char *kd = kind(plain[k]);
            bool isFetch = !strcmp(kd, "fetch");
            runCase(t, st, k, accesses, plain[k], kd, isFetch, true, plainSR, pass ? " [SSP^1]" : "");
        }
    }
}

}
