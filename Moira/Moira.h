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

    uint8_t ipl;  // Interrupt Priority Level
};

class Moira {

public:

    // The emulated CPU type
    // MoiraCpu type;

    // The delegation object (connected memory)
    MoiraDelegate *memory = NULL;

private:

    // The data and address registers
    Registers reg;

    // The program counter
    uint32_t pc;

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
    uint16_t irc;
    uint16_t ird;

    // Flags
    StatusRegister sr;


    // Jump table storing all instruction handlers
    void (Moira::*exec[65536])(uint16_t);

    // Jump table storing all disassebler handlers
    void (Moira::*dasm[65536])(StrWriter&, uint16_t, uint16_t, uint16_t);

    // Jump table storing all time information handlers
    int (Moira::*sync[65536])(uint16_t, int);

    // Disassembler
    bool hex = true;
    char str[256];

public:
    
    Moira();

    void power();
    void reset();
    void process(uint16_t reg_ird);

    uint32_t getD(unsigned n) { assert(n < 8); return reg.d[n].read<Long>(); }
    void setD(unsigned n, uint32_t v) { assert(n < 8); reg.d[n].write<Long>(v); }
    uint32_t getA(unsigned n) { assert(n < 8); return reg.a[n].read<Long>(); }
    void setA(unsigned n, uint32_t v) { assert(n < 8); reg.a[n].write<Long>(v); }

    uint32_t getPC() { return pc; }
    uint32_t getIRC() { return irc; }
    uint32_t getIRD() { return ird; }

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
    uint16_t getSR();
    void setSR(uint16_t value);

    // Gets or sets the Condition Code Register (CCR, the lower byte of SR)
    uint8_t getCCR();
    void setCCR(uint8_t value);


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
    
    void disassemble(uint32_t addr, char *str, bool hex = true);
};

#endif
