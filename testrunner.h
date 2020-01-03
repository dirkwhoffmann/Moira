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
#include <stdint.h>
#include <string.h>
#include <time.h>

// Portable 68000
#include "tester.h"
#include "exception.h"

// Musashi
extern "C" {
#include "m68k.h"
#include "m68kcpu.h"
#include "m68kops.h"
}

// Set to true to compare Moira against Musashi
#define MUSASHI true

// Set to true to compare the disassemblers, too
#define TEST_DASM true

// Set to true for verbose output
#define VERBOSE false

// Forward declarations
namespace moira { class Moira; }
extern class moira::Moira *moiracpu;
extern class Tester_68k *tester;
extern uint8_t mem[0x10000];
extern uint8_t musashiMem[0x10000];
extern uint8_t moiraMem[0x10000];
extern Sandbox sandbox;

inline u8 get8(u8 *p, u32 addr) {
    return mem[addr & 0xFFFF]; }
inline u16 get16(u8 *p, u32 addr) {
    return get8(p, addr) << 8 | get8(p, addr + 1); }
inline void set8(u8 *p, u32 addr, u8 val) {
    mem[addr & 0xFFFF] = val; }
inline void set16(u8 *p, u32 addr, u16 val) {
    set8(p, addr, val); set8(p, addr+1, val & 0xFF); }

inline u32 memHi(u32 addr) { return mem[addr & 0xFFFF]; }
inline u32 memLo(u32 addr) { return mem[(addr + 1) & 0xFFFF]; }
inline u32 memWord(u32 addr) { return memHi(addr) << 8 | memLo(addr); }

// A test setup
struct Setup {

    uint8_t  ccr;
    bool     supervisor;
    uint32_t pc;
    uint16_t opcode;
    uint32_t d[8];
    uint32_t a[8];
    uint8_t  mem[0x10000];
};

// A test result
struct Result {

    uint32_t pc;
    uint32_t usp;
    uint32_t ssp;
    uint32_t d[8];
    uint32_t a[8];
    uint16_t sr;
    char dasm[128];
    int dasmCnt;
    int cycles;
};

// Create smart random numbers
uint32 smartRandom();

// Location of the tested instruction in memory
const uint32_t pc = 0x1000;

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

void runDasmTest(Setup &s);
void runSingleTest(Setup &s);

clock_t runMusashi(Setup &s, Result &r);
clock_t runMoira(Setup &s, Result &r);

//
// Analysing a test result
//

void recordMusashiResult(Result &r);
void recordMoiraResult(Result &r);

void compare(Setup &s, Result &r1, Result &r2);
bool compareD(Setup &s, Result &r1, Result &r2);
bool compareA(Setup &s, Result &r1, Result &r2);
bool comparePC(Setup &s, Result &r1, Result &r2);
bool compareSR(Setup &s, Result &r1, Result &r2);
bool compareIRD(Setup &s, Result &r1, Result &r2);
bool compareIRC(Setup &s, Result &r1, Result &r2);
bool compareCycles(Setup &s, Result &r1, Result &r2);
void compare(int c1, int c2, char *s1, char *s2);

void bugReport();

#endif
