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

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace runner {

using namespace moira;

extern class TestCPU *moiracpu;
extern Model cpuModel;
extern u16 opcode;

// A test setup
struct Setup {

    long    round;
    u8      ccr;
    bool    supervisor;
    u32     pc;
    u16     opcode;
    u16     ext1;
    u16     ext2;
    u16     ext3;
    u32     d[8];
    u32     a[8];
    u16     vbr;
    u16     sfc;
    u16     dfc;
    u32     cacr;
    u32     caar;
    Float80 fpr[8];
    u32     fpcr;
    u32     fpsr;
    u32     fpiar;
    u8      mem[0x10000];
};

// A test result
struct Result {

    u16     dasmResult;
    u32     oldpc;
    u16     opcode;
    u32     pc;
    u32     usp;
    u32     isp;
    u32     msp;
    u32     d[8];
    u32     a[8];
    u16     sr;
    u32     fc;
    u32     vbr;
    u32     sfc;
    u32     dfc;
    u32     cacr;
    u32     caar;
    Float80 fpr[8];
    u32     fpcr;
    u32     fpsr;
    u32     fpiar;
    char    dasmMoira[128];
    int     dasmCntMoira;
    char    dasmBinutils[BINUTILS_BUFFER_SIZE];
    int     dasmCntBinutils;
    char    dasmMusashi[128];
    int     dasmCntMusashi;
    char    dasmUAE[128];
    int     dasmCntUAE;
    char    dasmMoiraUAE[128];      // Moira's output used for the comparison with UAE
    int     dasmCntMoiraUAE;
    int     cycles;
    clock_t elapsed[2];
};

// Location of the tested instruction in memory
const u32 pc = 0x1000;


//
// Preparing a test
//

const std::vector<Model> &allModels();
const char *modelName(Model model);
std::optional<Model> parseModel(const char *name);

void selectModel(Model model);
const char *selectedModel();

void setupMoira();

void setupTestEnvironment(Setup &s, long round);
void setupFpuEnvironment(Setup &s);
void setupTestInstruction(Setup &s, u32 pc, u16 opcode);

void resetMusashi(Setup &s);        // Resets the Musashi core (the UAE core is set up in runUAE)
void resetMoira(Setup &s);
void resetMusashiFpu(Setup &s);
void resetMoiraFpu(Setup &s);


//
// Performing a test
//

void run(unsigned seed, const std::vector<Model> &models);

void runCPU(long round);
void runMMU(long round);
void runFPU(long round);
void passed();

void runSingleTest(Setup &s, u16 op);
void runSingleTest(Setup &s, u16 op, u16 ext);

void runBinutils(Setup &s, Result &r);
void runReference(Setup &s, Result &r);   // Runs the reference CPU (Musashi or UAE)
void runMoira(Setup &s, Result &r);

bool skip(u16 op);
bool skipAccessCheck(u16 op);


//
// Analysing a test result
//

void recordMusashiRegisters(Result &r);
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

bool isFpuOpcode(u16 op);

void bugReport();

} // namespace runner
