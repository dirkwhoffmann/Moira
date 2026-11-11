// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

// State shared by all test runner instances.
//
// The reference cores (Musashi, UAE, Binutils) are plain C/C++ libraries with
// global state. Everything those libraries need to call back into lives here and
// exists exactly once.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "MoiraTypes.h"
#include "FPU/MFloat80.h"
#include "FPU/FPU.h"
#include "Sandbox.h"
#include "Common.h"
#include "config.h"

extern "C" {
#include "m68k.h"
#include "m68kcpu.h"
#include "m68kops.h"
}

// Binutils
#include "m68k-dis.h"
struct meminfo { unsigned char bytes[0x10000]; unsigned len; };
extern meminfo mi;
extern disassemble_info di;

// Size of the buffer that receives the disassembly of the binutils disassembler
static const size_t BINUTILS_BUFFER_SIZE = 128;

// Initializes the reference libraries for a CPU model (implemented in the glue layer)
void setupBinutils(moira::CPUModel model);
void setupMusashi(moira::CPUModel model);
void setupUAE(moira::CPUModel model, moira::FPUModel fpu);
void setupMAME(moira::CPUModel model);

using common::Randomizer;
using common::portableRand;
using common::portableSrand;

extern Sandbox sandbox;
extern Randomizer randomizer;
extern u8 moiraMem[0x10000];    // Memory of the tested Moira core
extern u8 refMem[0x10000];      // Memory of the reference CPU (Musashi or UAE)
extern u32 musashiFC;           // Function code of the last Musashi bus access

inline u8 get8(u8 *p, u32 addr) {
    return p[addr & 0xFFFF];
}
inline u16 get16(u8 *p, u32 addr) {
    return (u16)(get8(p, addr) << 8 | get8(p, addr + 1));
}
inline u32 get32(u8 *p, u32 addr) {
    return (u32)(get16(p, addr) << 16 | get16(p, addr + 2));
}
inline void set8(u8 *p, u32 addr, u8 val) {
    p[addr & 0xFFFF] = val;
}
inline void set16(u8 *p, u32 addr, u16 val) {
    set8(p, addr, val >> 8); set8(p, addr + 1, val & 0xFF);
}
inline void set32(u8 *p, u32 addr, u32 val) {
    set16(p, addr, val >> 16); set16(p, addr + 2, val & 0xFFFF);
}

//
// Test data
//

namespace runner {

using moira::MFloat80;

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
    MFloat80 fpr[8];
    u32     fpcr;
    u32     fpsr;
    u32     fpiar;
    u64     crp;
    u64     srp;
    u32     tc;
    u32     tt0;
    u32     tt1;
    u32     mmusr;
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
    MFloat80 fpr[8];
    u32     fpcr;
    u32     fpsr;
    u32     fpiar;
    u64     crp;
    u64     srp;
    u32     tc;
    u32     tt0;
    u32     tt1;
    u32     mmusr;
    char    dasmMoira[128];
    int     dasmCntMoira;
    char    dasmMoiraSyn[128];      // Moira's output in its native syntax
    int     dasmCntMoiraSyn;
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

}


//
// Reference CPUs (implemented in the glue layer)
//

// Binutils
void disassembleBinutils(runner::Result &r);

// Musashi
void resetMusashi(const runner::Setup &s);
void disassembleMusashi(const runner::Setup &s, runner::Result &r);
void executeMusashi(runner::Result &r);

// MAME
void resetMAME(const runner::Setup &s);
void executeMAME(runner::Result &r);
bool addressErrorMAME();

// UAE
void disassembleUAE(const runner::Setup &s, runner::Result &r);
void resetUAE(const runner::Setup &s);
void executeUAE(runner::Result &r);
int lastExceptionUAE();
void setMMUUAE(u64 crp, u64 srp, u32 tc, u32 tt0, u32 tt1);
void enableMMUUAE(bool enable);
void flushATCUAE();
bool ptestUAE(u32 addr, u8 fc, bool write, u32 level, u32 *mmusr, u32 *descr);
bool translateUAE(u32 addr, bool super, bool data, bool write, u32 *physical);
void setMMU040UAE(u32 urp, u32 srp, u32 tc, u32 itt0, u32 itt1, u32 dtt0, u32 dtt1);
void endMMU040UAE();
void flushATC040UAE();
bool translate040UAE(u32 addr, bool super, bool data, bool write, u32 *physical);
u32 ptest040UAE(u32 addr, u8 dfc, bool write);
