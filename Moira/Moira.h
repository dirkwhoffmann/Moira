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

namespace moira {

//
// Configuration
//

#define MOIRA_EMULATE_ADDRESS_ERROR

struct Registers {

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
    u32 pc;               // Program counter
};

struct StatusRegister {

    bool t;               // Trace flag
    bool s;               // Supervisor flag
    bool x;               // Extend flag
    bool n;               // Negative flag
    bool z;               // Zero flag
    bool v;               // Overflow flag
    bool c;               // Carry flag

    u8 ipl;               // Interrupt Priority Level
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
    void (Moira::*exec[65536])(u16);

    // Jump table storing all disassebler handlers
    void (Moira::*dasm[65536])(StrWriter&, u32, u16);

    // Jump table storing all time information handlers
    int (Moira::*sync[65536])(u16, int);

    //
    // Disassembler
    //
    bool hex = true;
    Align tab{10};
    
    char str[256];

public:
    
    Moira();

    void power();
    void reset();
    void process(u16 reg_ird);

    template<Size S = Long> u32 readD(int n) { return CLIP<S>(reg.d[n]); }
    template<Size S = Long> u32 readA(int n) { return CLIP<S>(reg.a[n]); }
    template<Size S = Long> u32 readR(int n) { return CLIP<S>(reg.r[n]); }
    template<Size S = Long> void writeD(int n, u32 v) { reg.d[n] = SUBS<S>(reg.d[n], v); }
    template<Size S = Long> void writeA(int n, u32 v) { reg.a[n] = SUBS<S>(reg.a[n], v); }
    template<Size S = Long> void writeR(int n, u32 v) { reg.r[n] = SUBS<S>(reg.r[n], v); }
    template<Size S = Long> void incD(int n, i32 v) { writeD<S>(n, readD<S>(n) + v); }
    template<Size S = Long> void incA(int n, i32 v) { writeA<S>(n, readA<S>(n) + v); }
    template<Size S = Long> void incR(int n, i32 v) { writeR<S>(n, readR<S>(n) + v); }

    u32 getPC() { return reg.pc; }
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
    // Working with the Status Register
    //

public:

    // Gets or sets the Status Register
    u16 getSR();
    void setSR(u16 value);

    // Gets or sets the Condition Code Register (CCR, the lower byte of SR)
    u8 getCCR();
    void setCCR(u8 value);

    // Gets or sets the Supervisor Stack Pointer (SSP)
    u32 getSSP() { return reg.ssp; }
    void setSSP(u32 value) { reg.ssp = value; }


    // Enters or leaves supervisior mode
    void setSupervisorMode(bool enable);


    //
    // Managing the instruction stream
    //

private:

    void prefetch();
    void fullPrefetch();
    void readExtensionWord();
    void jumpToVector(u8 nr);
    
    //
    // Running the disassembler
    //

public:
    
    void disassemble(u32 addr, char *str, bool hex = true);
};

} 
#endif
