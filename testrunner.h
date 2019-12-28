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

// A test setup
struct Setup {

    uint32_t pc;
    uint16_t opcode;
    uint16_t sr;
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

// Checks for certain instruction types
bool isMul(uint16_t opcode);
bool isDiv(uint16_t opcode);
bool isMulOrDiv(uint16_t opcode);
bool isNbcd(uint16_t opcode);
bool isAbcd(uint16_t opcode);
bool isSbcd(uint16_t opcode);
bool isBcd(uint16_t opcode);

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
void runSingleTest(Setup &s);

//
// Analysing a test result
//

void dumpSetup(Setup &s);
void dumpResult(Result &r);
void compare(Setup &s, Result &r1, Result &r2);
void compare(int c1, int c2, char *s1, char *s2);
void bugReport();

#endif
