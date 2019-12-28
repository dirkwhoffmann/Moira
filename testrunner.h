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


//
// Preparing a test
//

void setupTestCase(Setup &s, uint32_t pc, uint16_t opcode);

// Setting up memory
void setupMemory(uint32_t addr, uint16_t val1, uint16_t val2, uint16_t val3);
void setMem16(uint32_t addr, uint16_t word);

// Setting up the CPU (called on start up)
void setupMusashi();
void setupMoira();

// Resetting the CPU (called at the beginning of each test)
void resetMusashi(Setup &s);
void resetMoira(Setup &s);

// Setting up Moira


//
// Performing a test
//

// Main entry point to run all tests
void dasmTest();
void run();

//
// Analysing a test result
//

void dumpSetup(Setup &s);
void dumpResult(Result &r);
void compare(Result &r1, Result &r2);


// Check for certain instructions
bool isMul(uint16_t opcode);
bool isDiv(uint16_t opcode);
bool isMulOrDiv(uint16_t opcode);
bool isNbcd(uint16_t opcode);
bool isAbcd(uint16_t opcode);
bool isSbcd(uint16_t opcode);
bool isBcd(uint16_t opcode);



#endif
