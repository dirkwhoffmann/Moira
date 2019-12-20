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
#include "Sandbox.h"
#include "assert.h"

namespace moira {

//
// Configuration
//

#define MOIRA_EMULATE_ADDRESS_ERROR 1

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

    // Sandbox (for memory debugging)
    Sandbox sandbox;

    // The delegation object (connected memory)
    MoiraDelegate *memory = NULL;

private:

    // The clock
    i64 clock;
    
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
    void (Moira::*dasm[65536])(StrWriter&, u32&, u16);


    //
    // Disassembler
    //

    bool hex = true;
    Align tab{8};
    char str[256];

    
    //
    // Constructing and initializing
    //

public:

    Moira();
    void createJumpTables();


    //
    // Running the device
    //

public:

    void power();
    void reset();
    void process(u16 reg_ird);
    void process() { process(ird); }


    //
    // Accessing registers
    //

public:

    template<Size S = Long> u32 readD(int n) { return CLIP<S>(reg.d[n]); }
    template<Size S = Long> u32 readA(int n) { return CLIP<S>(reg.a[n]); }
    template<Size S = Long> u32 readR(int n) { return CLIP<S>(reg.r[n]); }
    template<Size S = Long> void writeD(int n, u32 v) { reg.d[n] = SUBS<S>(reg.d[n], v); }
    template<Size S = Long> void writeA(int n, u32 v) { reg.a[n] = SUBS<S>(reg.a[n], v); }
    template<Size S = Long> void writeR(int n, u32 v) { reg.r[n] = SUBS<S>(reg.r[n], v); }
    template<Size S = Long> void incD(int n, i32 v) { writeD<S>(n, readD<S>(n) + v); }
    template<Size S = Long> void incA(int n, i32 v) { writeA<S>(n, readA<S>(n) + v); }
    template<Size S = Long> void incR(int n, i32 v) { writeR<S>(n, readR<S>(n) + v); }

    i64 getClock() { return clock; }
    u32 getPC() { return reg.pc; }
    u32 getIRC() { return irc; }
    u32 getIRD() { return ird; }
    void setIRD(u32 value) { ird = value; }

    //
    // Accessing memory
    //

    // Checks an address for an address error
    template<Size S> bool addressError(u32 addr);
    template<Mode M, Size S> bool addressErrorDeprecated(u32 addr);

    /* Reads or writes a value from or to memory
     * Returns false if an address error has occurred.
     */
    bool read8(u32 addr, u8 &value);
    bool read16(u32 addr, u16 &value);
    bool read32(u32 addr, u32 &value);
    bool write8(u32 addr, u8 value);
    bool write16(u32 addr, u16 value);
    bool write32(u32 addr, u32 value);

    u32 readOnReset(u32 addr);

    // Reads a value from memory (DEPRECATED)
    template<Size S> u32 read(u32 addr);

    // Writes a value to memory (DEPRECATED)
    template<Size S> void write(u32 addr, u32 value);

    // Pushes a long word onto the stack
    void push(u32 value);

    // Computes an effective address
    template<Mode M, Size S, u8 flags = 0> u32 computeEA(u32 n);

    // Emulates the address register modification for modes (An)+, (An)-
    template<Mode M, Size S> void postIncPreDec(int n);

    // Reads an operand
    template<Mode M, Size S> bool readOperand(int n, u32 &ea, u32 &result);

    // Writes an operand
    template<Mode M, Size S> bool writeOperand(int n, u32 value);

    // Writes an operand to an already computed effective address
    template<Mode M, Size S> void writeOperand(int n, u32 ea, u32 value);

    // Reads an immediate value
    template<Size S> u32 readImm();


    //
    // Emulate timing
    //

    void sync(int cycles);


    //
    // Processing arithmetical and logical operations
    //

    template <Instr I, Size S> u32  shift(int cnt, u64 data);
    template <Instr I, Size S> u32  arith(u32 op1, u32 op2);
    template <Instr I, Size S> u32  logic(u32 op1);
    template <Instr I, Size S> u32  logic(u32 op1, u32 op2);
    template <Instr I>         u32  bitop(u32 op, u8 bit);
    template <Size S>          void cmp(u32 op1, u32 op2);
    template <Instr I>         bool bcond();


private:

    // Instruction handlers
    #include "MoiraExec.h"
    #include "MoiraDasm.h"


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
    void dummyRead(u32 pc);
    void dummyRead() { dummyRead(reg.pc); }
    void jumpToVector(u8 nr);

    //
    // Running the disassembler
    //

public:

    int disassemble(u32 addr, char *str, bool hex = true);
};

}
#endif

