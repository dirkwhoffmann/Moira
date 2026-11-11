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


// Decides whether the results of a test case are excluded from the comparison
bool ExecRunner::skipCompare(const Setup &s, const Result &rRef, const Result &rMoira)
{
    // Cases that can be recognized by looking at the setup
    if (skip(s)) return true;

    // Cases that can only be recognized by looking at the results
    // Cases that the reference excludes
    if (ref().skipResult(s, rRef, rMoira)) return true;

    // MMU opcodes: Compare only instructions that execute on both sides. Exception handling of
    // invalid extension words differs (UAE raises a privilege violation for some of them).
    if (isMmuOpcode(s.opcode)) {
        auto completed = [](const Result &r) { return r.pc > 0x1000 && r.pc <= 0x1010; };
        if (!completed(rRef) || !completed(rMoira)) return true;

        // UAE fetches the extension words of indexed addressing modes incorrectly
        if ((s.opcode >> 3 & 7) == 6) return true;

        // UAE reads misaligned long words incorrectly
        if (misalignedRead) return true;

        // UAE applies the (An)+ and -(An) side effect to PFLUSH and PLOAD
        if ((s.opcode >> 3 & 7) == 3 || (s.opcode >> 3 & 7) == 4) return true;

        // PFLUSH without effective address: UAE still skips the extension words of the EA field
        if ((s.ext1 >> 13) == 1 && ((s.ext1 >> 8 & 31) == 4 || (s.ext1 >> 8 & 31) == 0x10) && (s.opcode & 0x3F) > 0x17) return true;

        // Table walks with translation disabled are meaningless (covered by the XLAT stage)
        u16 ext = s.ext1, mode = ext >> 8 & 31;
        if (!(s.tc & 0x80000000) && ((ext >> 13) == 4 || ((ext >> 13) == 1 && (mode == 0 || mode == 2)))) return true;

        // PMOVE to TC with an invalid configuration raises the MMU configuration exception, which Moira doesn't emulate
        auto invalidTC = [](u32 tc) {
            if (!(tc & 0x80000000)) return false;
            int ps = tc >> 20 & 15, is = tc >> 16 & 15, sum = ps + is;
            for (int i = 12; i >= 0; i -= 4) { int b = tc >> i & 15; if (!b) break; sum += b; }
            return ps < 8 || sum != 32;
        };
        if (invalidTC(rRef.tc)) return true;
    }

    return false;
}


void ExecRunner::compare(Setup &s, Result &rRef, Result &rMoira)
{
    bool error = false;

    if (doExec(s.opcode)) {

        if (!skipCompare(s, rRef, rMoira)) {

            if (!comparePC(rRef, rMoira)) {
                printf("\nPROGRAM COUNTER MISMATCH FOUND");
                error = true;
            }
            if (!compareCycles(rRef, rMoira)) {
                printf("\nCLOCK MISMATCH FOUND");
                error = true;
            }
            if (!compareSR(rRef, rMoira)) {
                printf("\nSTATUS REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareUSP(rRef, rMoira)) {
                printf("\nUSP MISMATCH FOUND");
                error = true;
            }
            if (!compareISP(rRef, rMoira)) {
                printf("\nISP MISMATCH FOUND");
                error = true;
            }
            if (!compareMSP(rRef, rMoira)) {
                printf("\nMSP MISMATCH FOUND");
                error = true;
            }
            if (!compareD(rRef, rMoira)) {
                printf("\nDATA REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareA(rRef, rMoira)) {
                printf("\nADDRESS REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareVBR(rRef, rMoira)) {
                printf("\nVBR REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareSFC(rRef, rMoira)) {
                printf("\nSFC REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareDFC(rRef, rMoira)) {
                printf("\nDFC REGISTER MISMATCH FOUND");
                error = true;
            }
            if (!compareCAxR(rRef, rMoira)) {
                printf("\nCACHE REGISTER MISMATCH FOUND");
                error = true;
            }
            if (isFpuOpcode(s.opcode)) {

                if (!compareFPR(rRef, rMoira)) {
                    printf("\nFLOATING-POINT REGISTER MISMATCH FOUND");
                    error = true;
                }
                if (!compareFPCR(rRef, rMoira)) {
                    printf("\nFPCR MISMATCH FOUND");
                    error = true;
                }
                if (!compareFPSR(rRef, rMoira)) {
                    printf("\nFPSR MISMATCH FOUND");
                    error = true;
                }
                if (!compareFPIAR(rRef, rMoira)) {
                    printf("\nFPIAR MISMATCH FOUND");
                    error = true;
                }
                if (!compareMemory()) {
                    printf("\nMEMORY MISMATCH FOUND");
                    printMemoryDiff();
                    error = true;
                }
            }
            if (mmuCompare && !compareMMU(rRef, rMoira)) {
                printf("\nMMU REGISTER MISMATCH FOUND");
                error = true;
            }
            if (sandbox.getErrors()) {
                printf("\nSANDBOX ERRORS REPORTED");
                if (lastSetup) {
                    printf("\nMemory at $0 (setup / reference / Moira):\n");
                    for (int q = 0; q < 16; q += 4)
                        printf("  %02x %02x %02x %02x  /  %02x %02x %02x %02x  /  %02x %02x %02x %02x\n",
                               lastSetup->mem[q], lastSetup->mem[q+1], lastSetup->mem[q+2], lastSetup->mem[q+3],
                               refMem[q], refMem[q+1], refMem[q+2], refMem[q+3],
                               moiraMem[q], moiraMem[q+1], moiraMem[q+2], moiraMem[q+3]);
                }
                error = true;
            }
        }
    }

    if (error) {

        printf("\n");

        // Disassemble the instruction again to get proper debug output
        memcpy(refMem, s.mem, sizeof(refMem));
        memcpy(moiraMem, s.mem, sizeof(moiraMem));
        disassembleReference(s, rRef);
        disassembleMoira(s, rMoira);
        dumpDasm(s, rRef, rMoira);

        printf("Setup:   ");
        dumpSetup(s);

        printf("%-7s: ", ref().name());
        dumpResult(rRef);

        printf("Moira:   ");
        dumpResult(rMoira);

        if (isMmuOpcode(s.opcode)) {
            printf("CRP:   %016llx / %016llx / %016llx (setup / %s / Moira)\n",
                   (unsigned long long)s.crp, (unsigned long long)rRef.crp, (unsigned long long)rMoira.crp, ref().name());
            printf("SRP:   %016llx / %016llx / %016llx\n",
                   (unsigned long long)s.srp, (unsigned long long)rRef.srp, (unsigned long long)rMoira.srp);
            printf("TC:    %08x / %08x / %08x\n", s.tc, rRef.tc, rMoira.tc);
            printf("TT0:   %08x / %08x / %08x\n", s.tt0, rRef.tt0, rMoira.tt0);
            printf("TT1:   %08x / %08x / %08x\n", s.tt1, rRef.tt1, rMoira.tt1);
            printf("MMUSR: %04x / %04x / %04x\n\n", s.mmusr & 0xFFFF, rRef.mmusr & 0xFFFF, rMoira.mmusr & 0xFFFF);
        }

        if (isFpuOpcode(s.opcode)) {
            for (int i = 0; i < 8; i++) {
                printf("FP%d: setup %s  %s %s  Moira %s\n", i,
                       s.fpr[i].str().c_str(), ref().name(), rRef.fpr[i].str().c_str(), rMoira.fpr[i].str().c_str());
            }
            printf("FPCR: %08x / %08x / %08x (setup / %s / Moira)\n", s.fpcr, rRef.fpcr, rMoira.fpcr, ref().name());
            printf("FPSR: %08x / %08x / %08x\n", s.fpsr, rRef.fpsr, rMoira.fpsr);
            printf("FPIAR: %08x / %08x / %08x\n\n", s.fpiar, rRef.fpiar, rMoira.fpiar);
        }

        bugReport();
    }
}


bool ExecRunner::compareD(Result &rRef, Result &rMoira)
{
    assert(rRef.opcode == rMoira.opcode);
    Instr instr = moiracpu->getInstrInfo(rRef.opcode).I;
    if (instr == Instr::DIVU || instr == Instr::DIVS)
    {
        // The reference CPUs differ in some corner cases
        return true;
    }

    for (int i = 0; i < 8; i++) if (rRef.d[i] != rMoira.d[i]) return false;
    return true;
}


bool ExecRunner::compareA(Result &rRef, Result &rMoira)
{
    for (int i = 0; i < 8; i++) if (rRef.a[i] != rMoira.a[i]) return false;
    return true;
}


bool ExecRunner::comparePC(Result &rRef, Result &rMoira)
{
    return ref().equalPC(rRef, rMoira);
}


bool ExecRunner::compareSR(Result &rRef, Result &rMoira)
{
    assert(rRef.opcode == rMoira.opcode);
    Instr instr = moiracpu->getInstrInfo(rRef.opcode).I;
    if (instr == Instr::DIVU || instr == Instr::DIVS) {

        // The reference CPU differs (and is likely wrong). Ignore it
        return true;
    }

    // Compare all bits that are defined
    u32 mask = ref().srMask(instr);
    return (rRef.sr & mask) == (rMoira.sr & mask);
}


bool ExecRunner::compareUSP(Result &rRef, Result &rMoira)
{
    return rRef.usp == rMoira.usp;
}


bool ExecRunner::compareISP(Result &rRef, Result &rMoira)
{
    return rRef.isp == rMoira.isp;
}


bool ExecRunner::compareMSP(Result &rRef, Result &rMoira)
{
    return rRef.msp == rMoira.msp;
}


bool ExecRunner::compareVBR(Result &rRef, Result &rMoira)
{
    return rRef.vbr == rMoira.vbr;
}


bool ExecRunner::compareSFC(Result &rRef, Result &rMoira)
{
    return rRef.sfc == rMoira.sfc;
}


bool ExecRunner::compareDFC(Result &rRef, Result &rMoira)
{
    return rRef.dfc == rMoira.dfc;
}


bool ExecRunner::compareCAxR(Result &rRef, Result &rMoira)
{
    return rRef.cacr == rMoira.cacr && rRef.caar == rMoira.caar;
}


bool ExecRunner::compareFPR(Result &rRef, Result &rMoira) { return ref().equalFPR(rRef, rMoira); }
bool ExecRunner::compareFPCR(Result &rRef, Result &rMoira) { return ref().equalFPCR(rRef, rMoira); }
bool ExecRunner::compareFPSR(Result &rRef, Result &rMoira) { return ref().equalFPSR(rRef, rMoira); }
bool ExecRunner::compareFPIAR(Result &rRef, Result &rMoira) { return ref().equalFPIAR(rRef, rMoira); }
bool ExecRunner::compareMMU(Result &rRef, Result &rMoira)
{
    return rRef.crp == rMoira.crp && rRef.srp == rMoira.srp &&
           rRef.tc == rMoira.tc && rRef.tt0 == rMoira.tt0 && rRef.tt1 == rMoira.tt1 &&
           (rRef.mmusr & 0xFFFF) == (rMoira.mmusr & 0xFFFF);
}

bool ExecRunner::compareMemory() { return memcmp(refMem, moiraMem, sizeof(refMem)) == 0; }

// Prints the first differing bytes of the two memory images
void ExecRunner::printMemoryDiff()
{
    int shown = 0;
    for (size_t i = 0; i < sizeof(refMem) && shown < 16; i++) {
        if (refMem[i] != moiraMem[i]) {
            printf("\n  Memory[%05zx]: %s %02x  Moira %02x", i, ref().name(), refMem[i], moiraMem[i]);
            shown++;
        }
    }
}


bool ExecRunner::compareCycles(Result &rRef, Result &rMoira)
{
    assert(rRef.opcode == rMoira.opcode);
    auto I = moiracpu->getInstrInfo(rRef.opcode).I;
    auto S = moiracpu->getInstrInfo(rRef.opcode).S;
    auto M = moiracpu->getInstrInfo(rRef.opcode).M;

    // Exclude some instructions
    if (I == Instr::TAS) return true;

    // FPU timing is not emulated yet
    if (isFpuOpcode(rRef.opcode)) return true;

    // Some references are only reliable for certain models
    if (ref().ignoreCycles(cpuModel)) return true;

    /* Timing comparison is off for every model beyond the 68000 and 68010
     * while the instruction cache is emulated. Those models (68020 and up)
     * all execute on Moira's C68020 core (see createJumpTable() in
     * MoiraInit_cpp.h), which routes instruction fetches through the cache.
     * Musashi does not model the 68020's instruction cache, so its cycle
     * counts assume every fetch goes straight to the bus. Moira's
     * cache-aware fetch timing (tuned against WinUAE/UAE, not Musashi)
     * legitimately diverges from that flat model.
     */
    if constexpr (MOIRA_EMULATE_ICACHE) {
        if (cpuModel != CPUModel::M68000 && cpuModel != CPUModel::M68010) return true;
    }

    // WHY DO WE IGNORE THESE?
    if (cpuModel == CPUModel::M68010) {

        if (I == Instr::CLR && S == Byte && M == Mode::AL) return true;
        if (I == Instr::CLR && S == Word && M == Mode::AL) return true;
        if (I == Instr::TRAPV) return true;
    }

    return rRef.cycles == rMoira.cycles;
}

} // namespace runner
