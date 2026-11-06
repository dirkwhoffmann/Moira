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
extern Model cpuModel;
extern u16 opcode;

//
// Preparing a test
//

const std::vector<Model> &allModels();
const char *modelName(Model model);
std::optional<Model> parseModel(const char *name);

void selectModel(Model model);
const char *selectedModel();

// Returns the name of the reference CPU (determined by the compatibility mode)
const char *reference();

void setupMoira();
void setupTestEnvironment(Setup &s, long round);
void setupFpuEnvironment(Setup &s);
void setupTestInstruction(Setup &s, u32 pc, u16 opcode);

void resetMoira(Setup &s, bool fpu = false);


//
// Performing a test
//

void run(unsigned seed, long rounds, const std::vector<Model> &models);

void runCPU(long round);
void runMMU(long round);
void runFPU(long round);
void passed();

void runSingleTest(Setup &s, u16 op);
void runSingleTest(Setup &s, u16 op, u16 ext);

void runReference(Setup &s, Result &r); 
void runMoira(Setup &s, Result &r);

bool skip(u16 op);
bool skipAccessCheck(u16 op);

bool isFpuOpcode(u16 op);
bool isIllegalOpcode(u16 op);


//
// Analysing a test result
//

void recordMoiraRegisters(Result &r);
void recordRefFpu(Result &r);
void recordMoiraFpu(Result &r);

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
bool compareMemory();

void bugReport();

}
