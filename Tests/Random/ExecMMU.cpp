// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "ExecRunner.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <string>

namespace runner {

using namespace moira;


//
// MMU address translation
//

namespace {

struct TranslationSetup {

    u32 tc;
    u64 crp;
    u64 srp;
    u32 tt0;
    u32 tt1;
    int levels;
    int bits[4];
};

// Fills the memory with descriptor-like data
void fillWithDescriptors()
{
    for (int i = 0; i < 0x10000; i += 4) {

        u32 w;
        int kind = portableRand() % 8;

        if (kind < 4) {
            // Short descriptor
            u32 r0 = u32(portableRand()), r1 = portableRand(), r2 = portableRand();
            w = (r0 & 0xFFFFFFF0) | (r1 % 4) | (r2 & 0xC);
            if (portableRand() % 6 == 0) w &= ~3u;
        } else if (kind < 6) {
            // Upper half of a long descriptor (limit, flags, DT)
            u32 limit = (portableRand() % 3 == 0) ? 0x7FFF : u32(portableRand() % 70);
            u32 r0 = portableRand(), r1 = portableRand(), r2 = portableRand(), r3 = portableRand();
            w = (r0 % 6 == 0 ? 0x80000000 : 0) | limit << 16 | (r1 & 0x0100) | (r2 & 0x1C) | (r3 % 4);
            if (portableRand() % 6 == 0) w &= ~3u;
        } else {
            // Lower half of a long descriptor (address)
            u32 r0 = portableRand();
            u32 hi = portableRand() % 3 == 0 ? u32(portableRand()) << 16 : 0;
            w = (r0 & 0xFFF0) | hi;
        }
        refMem[i] = moiraMem[i] = u8(w >> 24);
        refMem[i + 1] = moiraMem[i + 1] = u8(w >> 16);
        refMem[i + 2] = moiraMem[i + 2] = u8(w >> 8);
        refMem[i + 3] = moiraMem[i + 3] = u8(w);
    }
}


TranslationSetup randomTranslationSetup()
{
    TranslationSetup t;

    // Page size, initial shift, and table index bits (sum must be 32)
    int ps = 8 + portableRand() % 8;
    int is = portableRand() % 3 == 0 ? portableRand() % 6 : 0;
    int rest = 32 - ps - is;

    do {
        t.levels = 2 + portableRand() % 3;
        int left = rest;
        for (int i = 0; i < 4; i++) t.bits[i] = 0;
        for (int i = 0; i < t.levels; i++) {
            int minLeft = t.levels - i - 1;
            int maxBits = std::min(6, left - minLeft);
            int b = (i == t.levels - 1) ? left : 1 + portableRand() % maxBits;
            t.bits[i] = b;
            left -= b;
        }
        if (left != 0) t.levels = 0;
    } while (t.levels == 0 || t.bits[t.levels - 1] > 6 || t.bits[t.levels - 1] < 1);

    u32 tcE = portableRand() % 2 ? 0x02000000 : 0;
    u32 tcF = portableRand() % 4 == 0 ? 0x01000000 : 0;
    t.tc = 0x80000000 | tcE | tcF | ps << 20 | is << 16
         | t.bits[0] << 12 | t.bits[1] << 8 | t.bits[2] << 4 | t.bits[3];

    auto rootPointer = [&]() {
        u64 limit = portableRand() % 3 == 0 ? 0x7FFF : portableRand() % 70;
        u64 lu = portableRand() % 6 == 0;
        u64 dt = 1 + portableRand() % 3;
        return lu << 63 | limit << 48 | dt << 32 | u64(portableRand() & 0xFFF0);
    };
    auto transparentTranslation = [&]() {
        if (portableRand() % 2) return u32(0);
        u32 mask = (0xFF00 >> (portableRand() % 9)) & 0xFF;
        u32 b = portableRand() & 0xFF;
        u32 f10 = portableRand() % 2, f9 = portableRand() % 2, f8 = portableRand() % 2;
        u32 m1 = portableRand() % 8, m0 = portableRand() % 8;
        return b << 24 | mask << 16 | 0x8000 | f10 << 10 | f9 << 9 | f8 << 8 | m1 << 4 | m0;
    };
    t.tt0 = transparentTranslation();
    t.tt1 = transparentTranslation();
    t.crp = rootPointer();
    t.srp = rootPointer();
    return t;
}


// Fills the memory with 68040 table descriptors
void fillWith040Descriptors()
{
    for (int i = 0; i < 0x10000; i += 4) {

        // Descriptor type: invalid, page, indirect, or table / page
        int r = portableRand() % 100;
        u32 type = r < 15 ? 0 : r < 55 ? 1 : r < 70 ? 2 : 3;

        u32 w0 = u32(portableRand()), w1 = u32(portableRand());
        u32 w = (w0 & 0xFF00) | (w1 & 0xFC) | type;
        if (portableRand() % 3 == 0) w &= ~0x7Cu;               // Clear W, U, M, CM, S
        if (portableRand() % 4 == 0) w |= u32(portableRand() & 3) << 8; // U0, U1
        if (portableRand() % 4 == 0) w |= 0x400;                // G
        refMem[i] = moiraMem[i] = u8(w >> 24);
        refMem[i + 1] = moiraMem[i + 1] = u8(w >> 16);
        refMem[i + 2] = moiraMem[i + 2] = u8(w >> 8);
        refMem[i + 3] = moiraMem[i + 3] = u8(w);
    }
}


u32 randomTT040()
{
    if (portableRand() % 2) return 0;
    u32 mask = (0xFF00 >> (portableRand() % 9)) & 0xFF;
    u32 b = portableRand() & 0xFF;
    u32 f14 = portableRand() % 2, f13 = portableRand() % 2;
    u32 f2 = portableRand() % 4 == 0, f5 = portableRand() % 4, f8 = portableRand() % 4;
    return b << 24 | mask << 16 | 0x8000 | f14 << 14 | f13 << 13 | f2 << 2 | f5 << 5 | f8 << 8;
}


}


void ExecRunner::runMMU(long round)
{
    if (!moiracpu->hasMMU()) return;

    // Let UAE execute MMU instructions and compare the MMU registers
    mmuCompare = ref().comparesMMU(cpuModel);
    if (mmuCompare) enableMMUUAE(true);
 
    static const int MMU_SETUPS = 4;
    Setup setup;
    resetClocks();

    printf("%11s MMU : CORE ", selectedModel()); fflush(stdout);
    common::Progress progress;

    for (int i = 0; i < MMU_SETUPS; i++) {

        setupTestEnvironment(setup, round * MMU_SETUPS + i);

        // Start with a valid (but disabled) translation setup
        TranslationSetup t = randomTranslationSetup();
        setup.tc = t.tc & 0x7FFFFFFF;
        setup.crp = t.crp;
        setup.srp = t.srp;
        setup.tt0 = t.tt0;
        setup.tt1 = t.tt1;

        for (int op = 0xF000; op <= 0xF03F; op++) {

            progress.update(100.0 * (i * 64 + (op - 0xF000)) / (MMU_SETUPS * 64));

            for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

                runSingleTest(setup, u16(op), u16(ext));
            }
        }
    }
    progress.finish();
    if (mmuCompare) { enableMMUUAE(false); mmuCompare = false; }
    passed(moiraclk[0], refclk[0]);
}


void ExecRunner::runMMUTranslate(long round)
{
    static const int TABLE_SETUPS = 256;
    static const int ADDRESSES = 512;

    long successes = 0, faults = 0;
    resetClocks();
    printf("%11s MMU : XLAT ", selectedModel()); fflush(stdout);
    common::Progress progress;

    for (int i = 0; i < TABLE_SETUPS; i++) {

        progress.update(100.0 * i / TABLE_SETUPS);

        fillWithDescriptors();
        auto t = randomTranslationSetup();

        // Configure both MMUs
        setMMUUAE(t.crp, t.srp, t.tc, t.tt0, t.tt1);
        moiracpu->mmu.mmu030.crp = t.crp;
        moiracpu->mmu.mmu030.srp = t.srp;
        moiracpu->mmu.mmu030.tc = t.tc;
        moiracpu->mmu.mmu030.tt0 = t.tt0;
        moiracpu->mmu.mmu030.tt1 = t.tt1;

        u32 lastAddr = 0; u8 lastFC = 1;

        for (int j = 0; j < ADDRESSES; j++) {

            // Flush the ATCs regularly. This keeps the number of cached pages below
            // the ATC size, so that the (pseudo-LRU) replacement strategy doesn't matter.
            if (j % 16 == 0) {
                flushATCUAE();
                for (auto &e : moiracpu->mmu.mmu030.atc) e.valid = false;
            }

            // Pick an address that is likely to be covered by the tables
            u32 addr = u32(portableRand()) << (portableRand() % 17);
            if (portableRand() % 4 == 0) addr &= 0xFFFF;
            bool super = portableRand() % 2, data = portableRand() % 2, write = data && portableRand() % 2;
            u8 fc = (super ? 4 : 0) | (data ? 1 : 2);

            // UAE's ATC lookup is undefined for write accesses if the root pointer
            // terminates the walk, because the M bit has nowhere to live.
            u64 rp = (super && (t.tc & 0x02000000)) ? t.srp : t.crp;
            if (write && ((rp >> 32) & 3) == 1) write = false;   // (also with FCL: FC lookup is skipped)

            // Run PTEST on both MMUs (no side effects)
            {
                u32 level = portableRand() % 6;
                u32 paddr = addr, pfc = fc;
                if (level == 0) {

                    // UAE compares the unmasked address with the ATC entries. Use the
                    // page of an earlier access to get ATC hits.
                    paddr = (portableRand() % 4 ? lastAddr : addr) & ~((1u << (t.tc >> 20 & 0xF)) - 1);
                    if (portableRand() % 4) pfc = lastFC;
                }
                bool pwrite = portableRand() % 2;
                u32 statusRef = 0, descrRef = 0, descrMoira = 0;
                ptestUAE(paddr, pfc, pwrite, level, &statusRef, &descrRef);
                moiracpu->setSupervisorMode(super);
                sandbox.prepare(false, true);
                u32 statusMoira = moiracpu->mmu.mmu030.test(paddr, pfc, pwrite, level, descrMoira);

                // The descriptor address is only returned for levels > 0
                bool equal = statusRef == statusMoira && (level == 0 || descrRef == descrMoira) && !memcmp(refMem, moiraMem, sizeof(refMem));
                if (!equal) {

                    printf("\n\nPTEST MISMATCH FOUND\n\n");
                    printf("Setup %d.%d: TC: %08x  CRP: %016llx  SRP: %016llx  TT0: %08x  TT1: %08x\n", i, j, t.tc, t.crp, t.srp, t.tt0, t.tt1);
                    printf("PTEST%c: addr %08x  FC %d  level %d\n", pwrite ? 'W' : 'R', paddr, pfc, level);
                    printf("UAE  : MMUSR %04x  descriptor %08x\n", statusRef, descrRef);
                    printf("Moira: MMUSR %04x  descriptor %08x\n", statusMoira, descrMoira);
                    exit(1);
                }
            }

            // Translate with both MMUs
            u32 physRef = 0, physMoira = 0;
            bool okRef = translateUAE(addr, super, data, write, &physRef);
            moiracpu->setSupervisorMode(super);
            sandbox.prepare(false, true);     // Don't record the descriptor updates
            auto translated = moiracpu->mmu.translate(addr, fc, write);
            bool okMoira = translated.has_value();
            if (okMoira) physMoira = *translated;

            (okRef ? successes : faults)++;
            lastAddr = addr; lastFC = fc;
            bool equal = okRef == okMoira && (!okRef || physRef == physMoira) && !memcmp(refMem, moiraMem, sizeof(refMem));
            if (equal) continue;

            printf("\n\nTRANSLATION MISMATCH FOUND\n\n");
            printf("Setup %d.%d: TC: %08x  CRP: %016llx  SRP: %016llx  TT0: %08x  TT1: %08x\n", i, j, t.tc, t.crp, t.srp, t.tt0, t.tt1);
            printf("Access: addr %08x  FC %d  %s\n", addr, fc, write ? "write" : "read");
            printf("UAE  : %s %08x\n", okRef ? "ok   " : "fault", physRef);
            printf("Moira: %s %08x\n", okMoira ? "ok   " : "fault", physMoira);
            for (int k = 0; k < 0x10000; k++) {
                if (refMem[k] != moiraMem[k]) {
                    printf("Memory differs at %04x: UAE %02x Moira %02x\n", k, refMem[k], moiraMem[k]);
                    break;
                }
            }
            exit(1);
        }
    }
    progress.finish();
    passed(0, 0);
    printf("%11s           %ld translations, %ld faults\n", "", successes, faults);
}


void ExecRunner::runMMUTranslate040(long round)
{
    static const int TABLE_SETUPS = 256;
    static const int ADDRESSES = 512;

    long successes = 0, faults = 0;
    resetClocks();
    printf("%11s MMU : XLAT ", selectedModel()); fflush(stdout);
    common::Progress progress;

    auto &mmu = moiracpu->mmu.mmu040;

    for (int i = 0; i < TABLE_SETUPS; i++) {

        progress.update(100.0 * i / TABLE_SETUPS);

        fillWith040Descriptors();

        u32 urp = u32(portableRand()) & 0xFE00, srp = u32(portableRand()) & 0xFE00;
        u32 tc = 0x8000 | (portableRand() % 2 ? 0x4000 : 0);
        u32 itt0 = randomTT040(), itt1 = randomTT040(), dtt0 = randomTT040(), dtt1 = randomTT040();

        // Configure both MMUs
        setMMU040UAE(urp, srp, tc, itt0, itt1, dtt0, dtt1);
        mmu.urp = urp; mmu.srp = srp;
        mmu.setITT0(itt0); mmu.setITT1(itt1); mmu.setDTT0(dtt0); mmu.setDTT1(dtt1);
        mmu.setTC(tc);

        for (int j = 0; j < ADDRESSES; j++) {

            // Flush the ATCs regularly (UAE's ATC is 4-way set associative)
            if (j % 8 == 0) { flushATC040UAE(); mmu.atcFlush(); }

            u32 addr = u32(portableRand()) << (portableRand() % 17);
            if (portableRand() % 4 == 0) addr &= 0xFFFF;
            bool super = portableRand() % 2, data = portableRand() % 2, write = data && portableRand() % 2;
            u8 fc = (super ? 4 : 0) | (data ? 1 : 2);

            // PTEST
            if (portableRand() % 4 == 0) {

                bool pwrite = portableRand() % 2;
                u32 statusRef = ptest040UAE(addr, fc, pwrite);
                moiracpu->setSupervisorMode(super);
                sandbox.prepare(false, true);
                u32 statusMoira = mmu.test(addr, fc, pwrite);

                if (statusRef != statusMoira || memcmp(refMem, moiraMem, sizeof(refMem))) {

                    printf("\n\nPTEST MISMATCH FOUND\n\n");
                    printf("Setup %d.%d: TC: %04x  URP: %08x  SRP: %08x\n", i, j, tc, urp, srp);
                    printf("             ITT0: %08x  ITT1: %08x  DTT0: %08x  DTT1: %08x\n", itt0, itt1, dtt0, dtt1);
                    printf("PTEST%c: addr %08x  FC %d\n", pwrite ? 'W' : 'R', addr, fc);
                    printf("UAE  : MMUSR %08x\n", statusRef);
                    printf("Moira: MMUSR %08x\n", statusMoira);
                    endMMU040UAE();
                    exit(1);
                }
                continue;
            }

            // Translate with both MMUs
            u32 physRef = 0, physMoira = 0;
            bool okRef = translate040UAE(addr, super, data, write, &physRef);
            moiracpu->setSupervisorMode(super);
            sandbox.prepare(false, true);     // Don't record the descriptor updates
            auto translated = mmu.translate(addr, fc, write);
            bool okMoira = translated.has_value();
            if (okMoira) physMoira = *translated;

            (okRef ? successes : faults)++;
            bool equal = okRef == okMoira && (!okRef || physRef == physMoira) && !memcmp(refMem, moiraMem, sizeof(refMem));
            if (equal) continue;

            printf("\n\nTRANSLATION MISMATCH FOUND\n\n");
            printf("Setup %d.%d: TC: %04x  URP: %08x  SRP: %08x\n", i, j, tc, urp, srp);
            printf("             ITT0: %08x  ITT1: %08x  DTT0: %08x  DTT1: %08x\n", itt0, itt1, dtt0, dtt1);
            printf("Access: addr %08x  FC %d  %s\n", addr, fc, write ? "write" : "read");
            printf("UAE  : %s %08x\n", okRef ? "ok   " : "fault", physRef);
            printf("Moira: %s %08x\n", okMoira ? "ok   " : "fault", physMoira);
            for (int k = 0; k < 0x10000; k++) {
                if (refMem[k] != moiraMem[k]) {
                    printf("Memory differs at %04x: UAE %02x Moira %02x\n", k, refMem[k], moiraMem[k]);
                    break;
                }
            }
            endMMU040UAE();
            exit(1);
        }
    }
    endMMU040UAE();
    mmu.setTC(0);
    progress.finish();
    passed(0, 0);
    printf("%11s           %ld translations, %ld faults\n", "", successes, faults);
}

} // namespace runner
