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


// Setting up memory
void setupMemory(uint32_t addr, uint16_t val1, uint16_t val2, uint16_t val3);
void setMem16(uint32_t addr, uint16_t word);

// Setting up Musashi
void setupMusashi();
void resetMusashi();

// Setting up Moira


// Main entry point for running the test suite
void dasmTest();
void execTest();

namespace moira { class Moira; }
extern class moira::Moira *moiracpu;
extern class Tester_68k *tester;
extern uint8_t mem[0x10000];


// Check for certain instructions
bool isMul(uint16_t opcode);
bool isDiv(uint16_t opcode);
bool isMulOrDiv(uint16_t opcode);
bool isNbcd(uint16_t opcode);
bool isAbcd(uint16_t opcode);
bool isSbcd(uint16_t opcode);
bool isBcd(uint16_t opcode);



#endif
