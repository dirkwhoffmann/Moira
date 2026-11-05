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

#include "RunnerTypes.h"

// The runner is compiled once. It uses Moira's value types (Instr, Model, ...)
// from the default namespace and talks to the tested core through a MoiraAdapter.
#include "MoiraTypes.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace runner {

using namespace moira;

extern MoiraAdapter *moiracpu;
extern Model cpuModel;
extern u16 opcode;


//
// Preparing a test
//

void selectModel(Model model);
const char *selectedModel();

void setupBinutils();
void setupMusashi();

void setupTestEnvironment(Setup &s, long round);
void setupFpuEnvironment(Setup &s);
void setupTestInstruction(Setup &s, u32 pc, u16 opcode);

void resetMusashi(Setup &s);        // Resets the Musashi core (the UAE core is set up in runUAE)
void resetMusashiFpu(Setup &s);


//
// Performing a test
//

void run(MoiraAdapter &adapter);
void run(MoiraAdapter &adapter, unsigned seed);

void runCPU(long round);
void runMMU(long round);
void runFPU(long round);
void passed();

void runSingleTest(Setup &s, u16 op);
void runSingleTest(Setup &s, u16 op, u16 ext);

void runBinutils(Setup &s, Result &r);
void runReference(Setup &s, Result &r);   // Runs the reference CPU (Musashi or UAE)

bool skip(u16 op);
bool skipAccessCheck(u16 op);


//
// Analysing a test result
//

void recordMusashiRegisters(Result &r);
void recordRefFpu(Result &r);

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

bool isFpuOpcode(u16 op);

void bugReport();

} // namespace runner
