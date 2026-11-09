// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <optional>
#include <vector>

#include "Shared.h"

#include "TestCPU.h"

namespace runner {

using namespace moira;

extern class TestCPU *moiracpu;
extern CPUModel cpuModel;
extern FPUModel fpuModel;
extern u16 opcode;

//
// Preparing a test
//

const std::vector<CPUModel> &allModels();
const char *modelName(CPUModel model);
std::optional<CPUModel> parseModel(const char *name);

const std::vector<FPUModel> &allFpuModels();
const char *fpuModelName(FPUModel model);
std::optional<FPUModel> parseFpuModel(const char *name);

// Returns true if the FPU can be installed in a system with the given CPU
bool isCompatible(CPUModel cpu, FPUModel fpu);

void selectModel(CPUModel model);
void selectFpu(FPUModel fpu);
const char *selectedModel();

// Returns the name of the reference CPU (determined by the compatibility mode)
const char *reference();

void setupMoira();
void setupTestEnvironment(Setup &s, long round);
void setupFpuEnvironment(Setup &s);
void setupTestInstruction(Setup &s, u32 pc, u16 opcode);

void resetMoira(Setup &s);


//
// Performing a test
//

void run(unsigned seed, long rounds, const std::vector<CPUModel> &cpus, const std::vector<FPUModel> &fpus);

void runCPU(long round);
void runMMU(long round);
void runMMUTranslate(long round);
void runFPU(long round);
void runDasmCPU(long round);
void runDasmMMU(long round);
void runDasmFPU(long round);
void passed(clock_t moira, clock_t ref);

void runSingleTest(Setup &s, u16 op);
void runSingleTest(Setup &s, u16 op, u16 ext);

void runDasmSingleTest(Setup &s, u16 op);
void runDasmSingleTest(Setup &s, u16 op, u16 ext);

void runReference(Setup &s, Result &r);
void runMoira(Setup &s, Result &r);
void disassembleReference(Setup &s, Result &r);
void disassembleMoira(Setup &s, Result &r);

bool skip(const Setup &s);                                           // Skips a test case based on its setup
bool skipCompare(const Setup &s, const Result &rRef, const Result &rMoira); // Skips the comparison (includes skip)
bool skipAccessCheck(u16 op);

extern bool misalignedRead;
bool isFpuOpcode(u16 op);
bool isMmuOpcode(u16 op);
bool isIllegalOpcode(u16 op);


//
// Analysing a test result
//

void recordMoiraRegisters(Result &r);

void compare(Setup &s, Result &rRef, Result &rMoira);
bool compareDasm(Result &rRef, Result &rMoira);
bool compareD(Result &rRef, Result &rMoira);
bool compareA(Result &rRef, Result &rMoira);
bool comparePC(Result &rRef, Result &rMoira);
bool compareSR(Result &rRef, Result &rMoira);
bool compareUSP(Result &rRef, Result &rMoira);
bool compareISP(Result &rRef, Result &rMoira);
bool compareMSP(Result &rRef, Result &rMoira);
bool compareVBR(Result &rRef, Result &rMoira);
bool compareSFC(Result &rRef, Result &rMoira);
bool compareDFC(Result &rRef, Result &rMoira);
bool compareCAxR(Result &rRef, Result &rMoira);
bool compareCycles(Result &rRef, Result &rMoira);
bool compareFPR(Result &rRef, Result &rMoira);
bool compareFPCR(Result &rRef, Result &rMoira);
bool compareFPSR(Result &rRef, Result &rMoira);
bool compareFPIAR(Result &rRef, Result &rMoira);
bool compareMMU(Result &rRef, Result &rMoira);
bool compareMemory();
void printMemoryDiff();

void dumpDasm(Setup &s, Result &rRef, Result &rMoira);
void bugReport();

}
