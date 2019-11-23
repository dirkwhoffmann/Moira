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

#include <stdint.h>
#include "MoiraUtils.h"
#include "MoiraDelegate.h"
#include "assert.h"

struct Reg {

    uint32_t value;

    template<Size S> uint32_t read() { return CLIP<S>(value); }
    template<Size S> void write(uint32_t v) { value = CLIP<S>(v); }

    Reg & operator=(const uint32_t & rhs) { value = rhs; return *this; }
};

struct Registers {

    // Data and address registers
    union {
        struct {
            Reg d[8]; // D0, D1 ... D7
            Reg a[8]; // A0, A1 ... A7
        };
        Reg r[16];
    };

    
    // template<Size S> uint32_t readRn(unsigned n) { return CLIP<S>(r[n]); }
    // template<Size S> uint32_t readDn(unsigned n) { return CLIP<S>(d[n]); }
    // template<Size S> uint32_t readAn(unsigned n) { return CLIP<S>(a[n]); }
};

struct Flags {

    bool s;       // Supervisor flag
    bool m;       // Master / Interrupt state
    bool x;       // Extend flag
    bool n;       // Negative flag
    bool z;       // Zero flag
    bool v;       // Overflow flag
    bool c;       // Carry flag
};

class Moira {

public:

    // The emulated CPU type
    // MoiraCpu type;

    // The CPU delegate
    MoiraDelegate *delegate = NULL;

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
    Flags flags;

    // Jump table storing all instruction handlers
    void (Moira::*exec[65536])(uint16_t);

    // Jump table storing all disassebler handlers
    void (Moira::*dasm[65536])(uint16_t, char *, bool);

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
    // Managing the instruction stream
    //

    void prefetch();
    void readExtensionWord();

    
    //
    // Running the disassembler
    //

    void disassemble(uint16_t addr, char *str);
};

#endif
