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

    // Emulated CPU model (only 68000 at the moment)
    CPUModel model = M68000;

    // Number format used by the disassembler (hex or decimal)
    bool hex = true;

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

    // Executes the next instruction
    void execute();

    // Disassembles a single instruction
    // Returns the instruction size in bytes
    int disassemble(u32 addr, char *str);


    //
    // Accessing the clock
    //

public:

    i64 getClock() { return clock; }
    void setClock(i64 value) { clock = value; }

private:

    // Advances the clock (called before each memory access)
    void sync(int cycles);


    //
    // Accessing registers
    //

public:

    u32 getPC() { return reg.pc; }

    template<Size S = Long> u32 readD(int n);
    template<Size S = Long> u32 readA(int n);
    template<Size S = Long> u32 readR(int n);
    template<Size S = Long> void writeD(int n, u32 v);
    template<Size S = Long> void writeA(int n, u32 v);
    template<Size S = Long> void writeR(int n, u32 v);

    u32 getIRC() { return queue.irc; }
    u32 getIRD() { return queue.ird; }
    void setIRD(u32 value) { queue.ird = value; }

    
    //
    // Accessing memory
    //

private:

    /* When the CPU accesses memory, a cascade of functions is executed.
     * Based on the level of abstraction, we distinguish level 1, level 2, and
     * level 3 functions. Level 1 functions are the most abstract ones and
     * called inside the execution handler of an instruction. The level 3
     * functions perform the actual memory access.
     */


    // Level 2

    /* Reads a value from memory / Writes a value into memory
     *
     *      last: Indicates if this bus cycle is the last one. In that case,
     *            the CPU polls the interrupt level.
     *     error: If provided, the functions checks for an address error and
     *            triggers an address error exception if applicable.
     */
    template<Size S, bool last = false> u32 readM(u32 addr);
    template<Size S, bool last = false> u32 readM(u32 addr, bool &error);

    template<Size S, bool last = false> void writeM(u32 addr, u32 val);
    template<Size S, bool last = false> void writeM(u32 addr, u32 val, bool &error);

    template<Size S, bool last = false> void writeMrev(u32 addr, u32 val);
    template<Size S, bool last = false> void writeMrev(u32 addr, u32 val, bool &error);


    // Level 3

    // Read a byte or a word from memory
    u8 read8(u32 addr);
    u16 read16(u32 addr);

    // Special read16 variant used inside the reset routine
    u16 read16Reset(u32 addr);

    // Special read16 variant used by the disassembler
    u16 read16Dasm(u32 addr);

    // Returns the current value of the IPL pins
    u8 readIPL();


    /* Checks for an address error
     * An address error occurs if the CPU tries to access a word or a long word
     * that is located at an odd address. If an address error is encountered,
     * the function calls execAddressError to initiate exception processing.
     */
    template<Size S, int delay = 0> bool addressError(u32 addr);



    u16 readOnReset(u32 addr);

    // Pushes a value onto the stack
    template<Size S, bool last = false> void push(u32 value);

    // Computes an effective address
    template<Mode M, Size S, bool skip = false> u32 computeEA(u32 n);

    // Emulates the address register modification for modes (An)+, (An)-
    template<Mode M, Size S> void updateAn(int n);

    // Reads an immediate value
    template<Size S> u32 readI();



    void write8  (u32 addr, u8  val);
    void write16 (u32 addr, u16 val);


    void dummyRead(u32 pc);
    void dummyRead() { dummyRead(reg.pc); }


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

    template<bool last = false> void prefetch();
    template<bool last = false> void fullPrefetch();
    template<bool skip = false> void readExtensionWord();
    void jumpToVector(u8 nr);

    //
    // Handling interrupts
    //

private:

    /* Polls the IPL pins
     * Polling takes place during the last bus cycle of an instruction.
     */
    void pollIrq();


    //
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

