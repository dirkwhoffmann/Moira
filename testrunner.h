// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "Sandbox.h"
#include "TestCPU.h"

// Musashi
extern "C" {
#include "m68k.h"
#include "m68kcpu.h"
#include "m68kops.h"
}

// Set to true for verbose output
#define VERBOSE false

// Set to true to verify all memory write operations
#define CHECK_MEM_WRITES true

// Number of instructions that are executed in a row
#define RUNS 1

extern class TestCPU *moiracpu;
extern Sandbox sandbox;
extern uint8_t musashiMem[0x10000];
extern uint8_t moiraMem[0x10000];
extern u32 musashiFC;

inline u8 get8(u8 *p, u32 addr) {
    return p[addr & 0xFFFF]; }
inline u16 get16(u8 *p, u32 addr) {
    return (u16)(get8(p, addr) << 8 | get8(p, addr + 1)); }
inline void set8(u8 *p, u32 addr, u8 val) {
    p[addr & 0xFFFF] = val; }
inline void set16(u8 *p, u32 addr, u16 val) {
    set8(p, addr, val >> 8); set8(p, addr + 1, val & 0xFF); }

// A test setup
struct Setup {

    uint8_t  ccr;
    bool     supervisor;
    uint32_t pc;
    uint16_t ext1;
    uint16_t ext2;
    uint32_t d[8];
    uint32_t a[8];
    uint8_t  mem[0x10000];
};

// A test result
struct Result {

    uint16_t dasmResult;
    uint32_t oldpc;
    uint16_t opcode;
    uint32_t pc;
    uint32_t usp;
    uint32_t ssp;
    uint32_t d[8];
    uint32_t a[8];
    uint16_t sr;
    uint32_t fc;
    char dasm[64];
    int dasmCnt;
    int cycles;
};

// Location of the tested instruction in memory
const uint32_t pc = 0x1000;

// Creates a smart random number
uint32 smartRandom();


//
// Preparing a test
//

void setupMusashi();
void setupMoira();

void createTestCase(Setup &s);
void setupInstruction(Setup &s, uint32_t pc, uint16_t opcode);

void resetMusashi(Setup &s);
void resetMoira(Setup &s);

//
// Performing a test
//

void run();

void runSingleTest(Setup &s);

clock_t runMusashi(int i, Setup &s, Result &r);
clock_t runMoira(int i, Setup &s, Result &r);

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
bool compareSP(Result &r1, Result &r2);
bool compareIRD(Result &r1, Result &r2);
bool compareIRC(Result &r1, Result &r2);
bool compareCycles(Result &r1, Result &r2);

void bugReport();

#endif
