// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "DasmRunner.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <string>

namespace runner {

using namespace moira;

const char *DasmRunner::description() const
{
    return "Compares the disassembler output of Moira with reference disassemblers.\n"
           "Every opcode is disassembled in several syntaxes and the texts are compared.\n";
}

void DasmRunner::printRanges() const
{
    printf("     Dasm range : all opcodes\n");
}

void DasmRunner::setupReferences()
{
    if (dasmMusashi) setupMusashi(cpuModel);
    if (dasmUAE) setupUAE(cpuModel, fpuModel);
}

void DasmRunner::selectFpuReferences()
{
    if (dasmUAE) setupUAE(cpuModel, fpuModel);
}

void DasmRunner::runStages(long round, const std::vector<FPUModel> &fpus)
{
    runDasmCPU(round);
    runDasmMMU(round);

    for (FPUModel fpu : fpus) {

        if (isCompatible(cpuModel, fpu)) {
            
            selectFpu(fpu);
            runDasmFPU(round);
        }
    }
}


void DasmRunner::runDasmCPU(long round)
{
    static const int CPU_SETUPS = 16;

    Setup setup;
    resetClocks();

    printf("%11s CPU : DASM ", selectedModel()); fflush(stdout);

    for (int i = 0; i < CPU_SETUPS; i++) {

        printf("."); fflush(stdout);

        setupTestEnvironment(setup, round * CPU_SETUPS + i);

        for (int op = 0x0000; op <= 0xFFFF; op++) {

            if constexpr (SKIP_ILLEGAL) { if (isIllegalOpcode(u16(op))) continue; }

            // Skip instructions covered by runDasmFPU
            if (isFpuOpcode(u16(op))) continue;

            runDasmSingleTest(setup, u16(op));
        }
    }
    passedDasm();
}


void DasmRunner::runDasmMMU(long round)
{
    if (!moiracpu->hasMMU()) return;

    static const int MMU_SETUPS = 4;
    Setup setup;
    resetClocks();

    printf("%11s MMU : DASM ", selectedModel()); fflush(stdout);

    for (int i = 0; i < MMU_SETUPS; i++) {

        setupTestEnvironment(setup, round * MMU_SETUPS + i);

        for (int op = 0xF000; op <= 0xF03F; op++) {

            if ((op % 8) == 0) { printf("."); fflush(stdout); }

            for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

                runDasmSingleTest(setup, u16(op), u16(ext));
            }
        }
    }
    passedDasm();
}


void DasmRunner::runDasmFPU(long round)
{
    static const int FPU_SETUPS = 4;
    Setup setup;
    resetClocks();

    if (fpuModel == FPUModel::BUILTIN)
        printf("%11s FPU : DASM ", selectedModel());
    else
        printf("%5s+%5s FPU : DASM ", selectedModel(), fpuModelName(fpuModel));
    fflush(stdout);

    for (int i = 0; i < FPU_SETUPS; i++) {

        setupTestEnvironment(setup, round * FPU_SETUPS + i);

        for (int op = 0xF200; op <= 0xF23F; op++) {

            if ((op % 8) == 0) { printf("."); fflush(stdout); }

            for (int ext = 0x0000; ext <= 0xFFFF; ext++) {

                runDasmSingleTest(setup, u16(op), u16(ext));
            }
        }
    }
    passedDasm();
}


void DasmRunner::passedDasm()
{
    // Moira's time covers all syntax flavors; each reference is timed separately
    printf(" PASSED  (Moira: %.2fs  Ref:", double(moiraclk[1]) / double(CLOCKS_PER_SEC));
    if (dasmBinutils) printf(" %.2fs", double(dasmclk[0]) / double(CLOCKS_PER_SEC));
    if (dasmMusashi) printf(" %.2fs", double(dasmclk[1]) / double(CLOCKS_PER_SEC));
    if (dasmUAE) printf(" %.2fs", double(dasmclk[2]) / double(CLOCKS_PER_SEC));
    printf(")\n");
}


void DasmRunner::runDasmSingleTest(Setup &s, u16 op, u16 ext)
{
    s.ext1 = ext;
    runDasmSingleTest(s, op);
}


void DasmRunner::runDasmSingleTest(Setup &s, u16 op)
{
    Result refResult, moiraResult;

    // Prepare the test case with the selected instruction
    setupTestInstruction(s, pc, op);

    // Run both disassemblers
    disassembleReference(s, refResult);
    disassembleMoira(s, moiraResult);
    moiraclk[1] += moiraResult.elapsed[1];

    // Compare
    if (!compareDasm(refResult, moiraResult)) {

        printf("\nDISASSEMBLER MISMATCH FOUND\n");
        dumpDasm(s, refResult, moiraResult);
        bugReport();
    }
}









//
// Comparing disassembler output with UAE
//

// Returns true if the two disassembler outputs match. Instructions that either
// side regards as illegal are not compared, because the two disassemblers
// differ in how they report them.
bool dasmEqualUAE(const char *uae, const char *moira)
{
    auto starts = [](const char *s, const char *prefix) { return strncasecmp(s, prefix, strlen(prefix)) == 0; };

    if (starts(uae, "illegal") || starts(uae, "[") || starts(uae, "f-line") || starts(uae, "f?")) return true;
    if (starts(uae, "movec")) return strcasecmp(uae, moira) == 0;   // UAE prints some control registers in upper case
    if (strstr(uae, "(68020+)") || strstr(uae, "(68040+)")) return true;   // Not available on the selected model
    if (starts(uae, "fmovecr") || strstr(uae, "#+") || strstr(uae, "#-") || (starts(uae, "fmove.p") && strstr(uae, ",#$")) || strstr(uae, "/#$")) {
        return true;   // Floating-point constants are printed in a different notation
    }   // Addressing mode not available on the selected model
    if (starts(moira, "dc.w") || isdigit((unsigned char)moira[0])) return true;

    return strcmp(uae, moira) == 0;
}


static bool noUAEDasmForCoprocessor(u16 opcode)
{
    switch (TestRunner::get().cpuModel) {
        case CPUModel::M68EC020: case CPUModel::M68EC030: case CPUModel::M68EC040: case CPUModel::M68LC040:
            return opcode >= 0xF000;
        default:
            return false;
    }
}


bool DasmRunner::compareDasm(Result &rRef, Result &rMoira)
{
    auto I = moiracpu->getInstrInfo(rRef.opcode).I;

    bool skipMusashi = !dasmMusashi || rRef.opcode >= 0xF000;
    bool skipBinutils = !dasmBinutils;

    switch (cpuModel) {

        case CPUModel::M68010:

            skipBinutils |= I == Instr::CMPI;
            break;
            
        case CPUModel::M68EC020:
        case CPUModel::M68EC030:
        case CPUModel::M68EC040:
        case CPUModel::M68LC040:

            skipBinutils |= rRef.opcode >= 0xF000;
            break;

        default:
            break;
    }

    // Binutils decodes FPU instructions only for the 68040 (ENABLE_M68881 is off)
    skipBinutils |= (rRef.opcode & 0xFE00) == 0xF200 && moiracpu->has6888x();

    // Compare with the Musashi disassembler
    if (!skipMusashi) {

        if (rRef.dasmCntMusashi != rMoira.dasmCntMusashi) return false;
        if (strcmp(rRef.dasmMusashi, rMoira.dasmMusashi) != 0) return false;
    }

    // Compare with UAE's disassembler (UAE decodes coprocessor opcodes regardless of the model)
    if (dasmUAE && !noUAEDasmForCoprocessor(rRef.opcode) && !dasmEqualUAE(rRef.dasmUAE, rMoira.dasmMoiraUAE)) return false;

    // Moira's own syntax has no reference. Make sure that it produces some output. (The
    // length may differ from the GNU flavor, because the latter mimics Binutils' view on
    // instructions that are not available on the selected CPU.)
    if (!rMoira.dasmMoiraSyn[0] || rMoira.dasmCntMoiraSyn <= 0) return false;

    // Compare with binutils
    if (!skipBinutils) {

        if (I != Instr::FTRAPcc) {
            if (rRef.dasmCntBinutils != rMoira.dasmCntBinutils) return false;
        }
        if (strcmp(rRef.dasmBinutils, rMoira.dasmBinutils) != 0) return false;
    }

    return true;
}

} // namespace runner
