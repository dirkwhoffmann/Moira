// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef MOIRA_H
#define MOIRA_H

#include "MoiraConfig.h"
#include "MoiraTypes.h"
#include "StrWriter.h"

namespace moira {

struct Registers {

    u32 pc;               // Program counter

    union {
        struct {
            u32 d[8];     // D0, D1 ... D7
            u32 a[8];     // A0, A1 ... A7
        };
        struct {
            u32 r[16];    // D0, D1 ... D7, A0, A1 ... A7
        };
        struct {
            u32 _pad[15];
            u32 sp;       // Visible stack pointer (overlays a[7])
        };
    };

    u32 usp;              // User Stack Pointer
    u32 ssp;              // Supervisor Stack Pointer

    u8 ipl;               //  Interrupt Priority Level (polled from IPL lines)
};

struct StatusRegister {

    bool t;               // Trace flag
    bool s;               // Supervisor flag
    bool x;               // Extend flag
    bool n;               // Negative flag
    bool z;               // Zero flag
    bool v;               // Overflow flag
    bool c;               // Carry flag

    u8 ipl;               // Interrupt Priority Level (trigger threshold)
};

struct PrefetchQueue {    // http://pasti.fxatari.com/68kdocs/68kPrefetch.html

    u16 irc;              // The most recent word prefetched from memory
    u16 ird;              // The instruction currently being executed
};

class Moira {

    //
    // Configuration
    //

    // Emulated CPU model (68000 is the only supported model yet)
    CPUModel model = M68000;

    // Interrupt mode of this CPU
    IrqMode irqMode = IRQ_AUTO;

    // Number format used by the disassembler (hex or decimal)
    bool hex = true;

    // Text formatting style used by the disassembler (upper case or lower case)
    bool upper = false;

    // Tab spacing used by the disassembler
    Align tab{8};


    //
    // Internal state
    //

    // Number of elapsed cycles since powerup
    i64 clock;

    // The data and address registers
    Registers reg;

    // The status register
    StatusRegister sr;

    // The prefetch queue
    PrefetchQueue queue;

    // Jump table holding the instruction handlers
    void (Moira::*exec[65536])(u16);

    // Jump table holding the disassebler handlers
    void (Moira::*dasm[65536])(StrWriter&, u32&, u16);

    
    //
    // Constructing and configuring
    //

public:

    Moira();
    void createJumpTables();

    void configDasm(bool h, bool u) { hex = h; upper = u; }


    //
    // Running the device
    //

public:

    void reset();

    // Executes the next instruction
    void execute();

    // Disassembles a single instruction
    // Returns the instruction size in bytes
    int disassemble(u32 addr, char *str);


    //
    // Interfacing with the outside world
    //

private:

    u8 read8(u32 addr);
    u16 read16(u32 addr);
    u16 read16OnReset(u32 addr);
    u16 read16Dasm(u32 addr);
    u8 readIPL();
    void write8  (u32 addr, u8  val);
    void write16 (u32 addr, u16 val);
    int readIrqUserVector(u8 level) { return 0; }

    //
    // Accessing the clock
    //

public:

    i64 getClock() { return clock; }
    void setClock(i64 val) { clock = val; }

private:

    // Advances the clock (called before each memory access)
    void sync(int cycles);


    //
    // Accessing registers
    //

private:

    template<Size S = Long> u32 readD(int n);
    template<Size S = Long> u32 readA(int n);
    template<Size S = Long> u32 readR(int n);
    template<Size S = Long> void writeD(int n, u32 v);
    template<Size S = Long> void writeA(int n, u32 v);
    template<Size S = Long> void writeR(int n, u32 v);

public:

    u32 getD(int n) { return readD(n); }
    void setD(int n, u32 v) { writeD(n,v); }

    u32 getA(int n) { return readA(n); }
    void setA(int n, u32 v) { writeA(n,v); }

    u32 getPC() { return reg.pc; }
    void setPC(u32 val) { reg.pc = val; }

    u16 getIRC() { return queue.irc; }
    void setIRC(u16 val) { queue.irc = val; }

    u16 getIRD() { return queue.ird; }
    void setIRD(u16 val) { queue.ird = val; }

    u8 getCCR();
    void setCCR(u8 val);

    u16 getSR();
    void setSR(u16 val);

    void setSupervisorMode(bool enable);

    u32 getSSP() { return reg.ssp; }
    void setSSP(u32 val) { reg.ssp = val; }

    u32 getUSP() { return reg.usp; }
    void setUSP(u32 val) { reg.usp = val; }


    //
    // Handling interrupts
    //

    int getIrqVector(int level);


    // Analyzing instructions
    //

public:

    bool isIllegalInstr(u16 op) { return exec[op] == &Moira::execIllegal; }
    bool isLineAInstr(u16 op)   { return exec[op] == &Moira::execLineA; }
    bool isLineFInstr(u16 op)   { return exec[op] == &Moira::execLineF; }

    

private:

    #include "MoiraALU.h"
    #include "MoiraDataflow.h"
    #include "MoiraExec.h"
    #include "MoiraDasm.h"
};

}
#endif

