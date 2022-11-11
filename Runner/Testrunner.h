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

#include "Sandbox.h"
#include "Randomizer.h"
#include "TestCPU.h"

extern "C" {

// Musashi
#include "m68k.h"
#include "m68kcpu.h"
#include "m68kops.h"
}

#include "config.h"

using namespace moira;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

extern class TestCPU *moiracpu;
extern Sandbox sandbox;
extern Randomizer randomizer;
extern u8 musashiMem[0x10000];
extern u8 moiraMem[0x10000];
extern u32 musashiFC;
extern moira::Model cpuModel;
extern u16 opcode;

// Binutils
struct meminfo { unsigned char bytes[0x10000]; unsigned len; };
extern meminfo mi;
extern disassemble_info di;

inline u8 get8(u8 *p, u32 addr) {
    return p[addr & 0xFFFF];
}
inline u16 get16(u8 *p, u32 addr) {
    return (u16)(get8(p, addr) << 8 | get8(p, addr + 1));
}
inline void set8(u8 *p, u32 addr, u8 val) {
    p[addr & 0xFFFF] = val;
}
inline void set16(u8 *p, u32 addr, u16 val) {
    set8(p, addr, val >> 8); set8(p, addr + 1, val & 0xFF);
}

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
    char    dasmMoira[128];
    int     dasmCntMoira;
    char    dasmBinutils[128];
    int     dasmCntBinutils;
    char    dasmMusashi[128];
    int     dasmCntMusashi;
    int     cycles;
    clock_t elapsed[2];
};

// Location of the tested instruction in memory
const u32 pc = 0x1000;


//
// Preparing a test
//

void selectModel(moira::Model model);
const char *selectedModel();

void setupBinutils();
void setupMusashi();
void setupMoira();

void setupTestEnvironment(Setup &s, long round);
void setupTestInstruction(Setup &s, u32 pc, u16 opcode);

void resetMusashi(Setup &s);
void resetMoira(Setup &s);


//
// Performing a test
//

void run();

void runCPU(long round);
void runMMU(long round);
void runFPU(long round);
void passed();

void runSingleTest(Setup &s, u16 op);
void runSingleTest(Setup &s, u16 op, u16 ext);

void runBinutils(Setup &s, Result &r);
void runMusashi(Setup &s, Result &r);
void runMoira(Setup &s, Result &r);

bool skip(u16 op);


//
// Analysing a test result
//

void recordMusashiRegisters(Result &r);
void recordMoiraRegisters(Result &r);

void compare(Setup &s, Result &r1, Result &r2);
bool compareDasm(Result &r1, Result &r2);
bool compareD(Result &r1, Result &r2);
bool compareA(Result &r1, Result &r2);
bool comparePC(Result &r1, Result &r2);
bool compareSR(Result &r1, Result &r2);
bool compareUSP(Result &r1, Result &r2);
bool compareISP(Result &r1, Result &r2);
bool compareMSP(Result &r1, Result &r2);
bool compareVBR(Result &r1, Result &r2);
bool compareSFC(Result &r1, Result &r2);
bool compareDFC(Result &r1, Result &r2);
bool compareCAxR(Result &r1, Result &r2);
bool compareCycles(Result &r1, Result &r2);

void bugReport();
