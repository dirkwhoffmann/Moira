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

#include "MoiraUtils.h"
#include "StringWriter.h"
#include "MoiraDelegate.h"
#include "assert.h"

namespace Moira {

struct Registers {

    union {
        struct {
            Reg d[8];      // D0, D1 ... D7
            Reg a[8];      // A0, A1 ... A7
        };
        struct {
            Reg _pad[14];
            Reg usp;       // User Stack Pointer (equals a[7])
            Reg ssp;       // Supervisor Stack Pointer
        };
        Reg r[17];
    };
};

struct StatusRegister {

    bool s;       // Supervisor flag
    bool m;       // Master / Interrupt state
    bool x;       // Extend flag
    bool n;       // Negative flag
    bool z;       // Zero flag
    bool v;       // Overflow flag
    bool c;       // Carry flag

    u8 ipl;       // Interrupt Priority Level
};

class CPU {

public:

    // The emulated CPU type
    // MoiraCpu type;

    // The delegation object (connected memory)
    MoiraDelegate *memory = NULL;

private:

    // The data and address registers
    Registers reg;

    // The program counter
    u32 pc;

    /* The prefetch queue
     * http://pasti.fxatari.com/68kdocs/68kPrefetch.html
     *
     * Three registers are involved in prefetching:
     *
     *     IRC : Holds the most recent word prefetched from memory
     *     IR  : Holds the instruction currently being decoded
     *     IRD : Holds the instruction currently being executed
     *
     * For emulation, only IRC and IRD are needed.
     */
    u16 irc;
    u16 ird;

    // Flags
    StatusRegister sr;


    // Jump table storing all instruction handlers
    void (CPU::*exec[65536])(u16);

    // Jump table storing all disassebler handlers
    void (CPU::*dasm[65536])(StrWriter&, u16, u16, u16);

    // Jump table storing all time information handlers
    int (CPU::*sync[65536])(u16, int);

    // Disassembler
    bool hex = true;
    char str[256];

public:
    
    CPU();

    void power();
    void reset();
    void process(u16 reg_ird);

    u32 getD(unsigned n) { assert(n < 8); return reg.d[n].read<Long>(); }
    void setD(unsigned n, u32 v) { assert(n < 8); reg.d[n].write<Long>(v); }
    u32 getA(unsigned n) { assert(n < 8); return reg.a[n].read<Long>(); }
    void setA(unsigned n, u32 v) { assert(n < 8); reg.a[n].write<Long>(v); }

    u32 getPC() { return pc; }
    u32 getIRC() { return irc; }
    u32 getIRD() { return ird; }

private:

    // Initialization
    #include "MoiraInit.h"

    // Instruction logic
    #include "MoiraLogic.h"

    // Instruction handlers
    #include "MoiraExec.h"
    #include "MoiraDasm.h"
    #include "MoiraSync.h"


    //
    // Working with the instruction register
    //

public:

    // Gets or sets the Status Register
    u16 getSR();
    void setSR(u16 value);

    // Gets or sets the Condition Code Register (CCR, the lower byte of SR)
    u8 getCCR();
    void setCCR(u8 value);


    //
    // Managing the instruction stream
    //

private:

    void prefetch();
    void readExtensionWord();

    
    //
    // Running the disassembler
    //

public:
    
    void disassemble(u32 addr, char *str, bool hex = true);
};

} 
#endif
