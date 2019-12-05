// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

using namespace Moira;

/*
#define __ ,
#define bind(id, name) { \
assert(exec[id] == &CPU::execIllegal); \
assert(dasm[id] == &CPU::dasmIllegal); \
assert(sync[id] == &CPU::syncIllegal); \
exec[id] = &CPU::exec##name; \
dasm[id] = &CPU::dasm##name; \
sync[id] = &CPU::sync##name; \
}
*/

#define __ ,
#define bind(id, name) { \
assert(exec[id] == &CPU::execIllegal); \
assert(dasm[id] == &CPU::dasmIllegal); \
exec[id] = &CPU::exec##name; \
dasm[id] = &CPU::dasm##name; \
}


static u16
parse(const char *s, u16 sum = 0)
{
    return
    *s == ' ' ? parse(s + 1, sum) :
    *s == '-' ? parse(s + 1, sum << 1) :
    *s == '0' ? parse(s + 1, sum << 1) :
    *s == '1' ? parse(s + 1, (sum << 1) + 1) : sum;
}

void
CPU::init()
{
    // Start with clean tables
    for (int i = 0; i < 0x10000; i++) {
        exec[i] = &CPU::execIllegal;
        dasm[i] = &CPU::dasmIllegal;
        sync[i] = &CPU::syncIllegal;
    }

    // Register unimplemented instructions
    registerUnimplemented();

    // Register the instruction set
    registerADD();
    registerAND();
    registerASL();
    registerASR();
    registerCLR();
    registerDBcc();
    registerEXT();
    registerLEA();
    registerLSL();
    registerLSR();
    registerNOP();
    registerROL();
    registerROR();
    registerROXL();
    registerROXR();
    registerSUB();
}

void
CPU::registerUnimplemented()
{
    /* Unimplemented instructions are identified by the following bit patterns:
     *
     *    1010 ---- ---- ---- : (Line A instructions)
     *    1111 ---- ---- ---- : (Line F instructions)
     *
     * Both instructions types are handles similarly. They only differ in the
     * associated vector number.
     */
    for (int i = 0; i < 0x1000; i++) {

        exec[0b1010 << 12 | i] = &CPU::execLineA;
        dasm[0b1010 << 12 | i] = &CPU::dasmLineA;
        sync[0b1010 << 12 | i] = &CPU::syncLineA;

        exec[0b1111 << 12 | i] = &CPU::execLineF;
        dasm[0b1111 << 12 | i] = &CPU::dasmLineF;
        sync[0b1111 << 12 | i] = &CPU::syncLineF;
    }
}

template<Instr I> void
CPU::registerShift(const char *patternReg,
                   const char *patternImm,
                   const char *patternEa)
{
    u16 opcode;

    // ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR
    //
    // Modes: (1)   Dx,Dy       8,16,32
    //        (2)   #<data>,Dy  8,16,32
    //        (3)   <ea>          16
    //               ------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               ------------------------------------------------
    //                X       X   X   X   X   X   X?? X??         X?!

    // (1)
    for (int dx = 0; dx < 8; dx++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse(patternReg) | dx << 9 | dy;
            bind(opcode | 0 << 6, Shift<I __ 0 __ Byte>);
            bind(opcode | 1 << 6, Shift<I __ 0 __ Word>);
            bind(opcode | 2 << 6, Shift<I __ 0 __ Long>);
        }
    }

    // (2)
    for (int data = 0; data < 8; data++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse(patternImm) | data << 9 | dy;
            bind(opcode | 0 << 6, Shift<I __ 11 __ Byte>);
            bind(opcode | 1 << 6, Shift<I __ 11 __ Word>);
            bind(opcode | 2 << 6, Shift<I __ 11 __ Long>);
        }
    }

    // (3)
    opcode = parse(patternEa);
    for (int ax = 0; ax < 8; ax++) {
        bind(opcode | 2 << 3 | ax, Shift<I __ 2 __ Word>);
        bind(opcode | 3 << 3 | ax, Shift<I __ 3 __ Word>);
        bind(opcode | 4 << 3 | ax, Shift<I __ 4 __ Word>);
        bind(opcode | 5 << 3 | ax, Shift<I __ 5 __ Word>);
        bind(opcode | 6 << 3 | ax, Shift<I __ 6 __ Word>);
    }
    bind(opcode | 7 << 3 | 0, Shift<I __ 7 __ Word>);
    bind(opcode | 7 << 3 | 1, Shift<I __ 8 __ Word>);
}

template<Instr I> void
CPU::registerLogic(const char *patternXXReg, const char *patternRegXX)
{
    u16 opcode;

    // ADD
    //
    // Modes: (1)   <ea>,Dy
    //               ------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               ------------------------------------------------
    //                X       X   X   X   X   X   X   X   X   X   X
    //
    //        (2)   Dx,<ea>
    //               ------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               ------------------------------------------------
    //                        X   X   X   X   X   X   X

    // (1)
    for (int dy = 0; dy < 8; dy++) {

        opcode = parse(patternXXReg) | dy << 9;
        for (int reg = 0; reg < 8; reg++) {

            bind(opcode | 0 << 6 | 0 << 3 | reg, AndXXRg<I __ 0 __ Byte>);
            bind(opcode | 0 << 6 | 1 << 3 | reg, AndXXRg<I __ 1 __ Byte>);
            bind(opcode | 0 << 6 | 2 << 3 | reg, AndXXRg<I __ 2 __ Byte>);
            bind(opcode | 0 << 6 | 3 << 3 | reg, AndXXRg<I __ 3 __ Byte>);
            bind(opcode | 0 << 6 | 4 << 3 | reg, AndXXRg<I __ 4 __ Byte>);
            bind(opcode | 0 << 6 | 5 << 3 | reg, AndXXRg<I __ 5 __ Byte>);
            bind(opcode | 0 << 6 | 6 << 3 | reg, AndXXRg<I __ 6 __ Byte>);

            bind(opcode | 1 << 6 | 0 << 3 | reg, AndXXRg<I __ 0 __ Word>);
            bind(opcode | 1 << 6 | 1 << 3 | reg, AndXXRg<I __ 1 __ Word>);
            bind(opcode | 1 << 6 | 2 << 3 | reg, AndXXRg<I __ 2 __ Word>);
            bind(opcode | 1 << 6 | 3 << 3 | reg, AndXXRg<I __ 3 __ Word>);
            bind(opcode | 1 << 6 | 4 << 3 | reg, AndXXRg<I __ 4 __ Word>);
            bind(opcode | 1 << 6 | 5 << 3 | reg, AndXXRg<I __ 5 __ Word>);
            bind(opcode | 1 << 6 | 6 << 3 | reg, AndXXRg<I __ 6 __ Word>);

            bind(opcode | 2 << 6 | 0 << 3 | reg, AndXXRg<I __ 0 __ Long>);
            bind(opcode | 2 << 6 | 1 << 3 | reg, AndXXRg<I __ 1 __ Long>);
            bind(opcode | 2 << 6 | 2 << 3 | reg, AndXXRg<I __ 2 __ Long>);
            bind(opcode | 2 << 6 | 3 << 3 | reg, AndXXRg<I __ 3 __ Long>);
            bind(opcode | 2 << 6 | 4 << 3 | reg, AndXXRg<I __ 4 __ Long>);
            bind(opcode | 2 << 6 | 5 << 3 | reg, AndXXRg<I __ 5 __ Long>);
            bind(opcode | 2 << 6 | 6 << 3 | reg, AndXXRg<I __ 6 __ Long>);
        }
        bind(opcode | 0 << 6 | 7 << 3 | 0, AndXXRg<I __ 7 __ Byte>);
        bind(opcode | 0 << 6 | 7 << 3 | 1, AndXXRg<I __ 8 __ Byte>);
        bind(opcode | 0 << 6 | 7 << 3 | 2, AndXXRg<I __ 9 __ Byte>);
        bind(opcode | 0 << 6 | 7 << 3 | 3, AndXXRg<I __ 10 __ Byte>);
        bind(opcode | 0 << 6 | 7 << 3 | 4, AndXXRg<I __ 11 __ Byte>);

        bind(opcode | 1 << 6 | 7 << 3 | 0, AndXXRg<I __ 7 __ Word>);
        bind(opcode | 1 << 6 | 7 << 3 | 1, AndXXRg<I __ 8 __ Word>);
        bind(opcode | 1 << 6 | 7 << 3 | 2, AndXXRg<I __ 9 __ Word>);
        bind(opcode | 1 << 6 | 7 << 3 | 3, AndXXRg<I __ 10 __ Word>);
        bind(opcode | 1 << 6 | 7 << 3 | 4, AndXXRg<I __ 11 __ Word>);

        bind(opcode | 2 << 6 | 7 << 3 | 0, AndXXRg<I __ 7 __ Long>);
        bind(opcode | 2 << 6 | 7 << 3 | 1, AndXXRg<I __ 8 __ Long>);
        bind(opcode | 2 << 6 | 7 << 3 | 2, AndXXRg<I __ 9 __ Long>);
        bind(opcode | 2 << 6 | 7 << 3 | 3, AndXXRg<I __ 10 __ Long>);
        bind(opcode | 2 << 6 | 7 << 3 | 4, AndXXRg<I __ 11 __ Long>);
    }

    // (2)
     for (int dy = 0; dy < 8; dy++) {

         opcode = parse(patternRegXX) | dy << 9;
         for (int reg = 0; reg < 8; reg++) {

             bind(opcode | 0 << 6 | 2 << 3 | reg, AndRgXX<I __ 2 __ Byte>);
             bind(opcode | 0 << 6 | 3 << 3 | reg, AndRgXX<I __ 3 __ Byte>);
             bind(opcode | 0 << 6 | 4 << 3 | reg, AndRgXX<I __ 4 __ Byte>);
             bind(opcode | 0 << 6 | 5 << 3 | reg, AndRgXX<I __ 5 __ Byte>);
             bind(opcode | 0 << 6 | 6 << 3 | reg, AndRgXX<I __ 6 __ Byte>);

             bind(opcode | 1 << 6 | 2 << 3 | reg, AndRgXX<I __ 2 __ Word>);
             bind(opcode | 1 << 6 | 3 << 3 | reg, AndRgXX<I __ 3 __ Word>);
             bind(opcode | 1 << 6 | 4 << 3 | reg, AndRgXX<I __ 4 __ Word>);
             bind(opcode | 1 << 6 | 5 << 3 | reg, AndRgXX<I __ 5 __ Word>);
             bind(opcode | 1 << 6 | 6 << 3 | reg, AndRgXX<I __ 6 __ Word>);

             bind(opcode | 2 << 6 | 2 << 3 | reg, AndRgXX<I __ 2 __ Long>);
             bind(opcode | 2 << 6 | 3 << 3 | reg, AndRgXX<I __ 3 __ Long>);
             bind(opcode | 2 << 6 | 4 << 3 | reg, AndRgXX<I __ 4 __ Long>);
             bind(opcode | 2 << 6 | 5 << 3 | reg, AndRgXX<I __ 5 __ Long>);
             bind(opcode | 2 << 6 | 6 << 3 | reg, AndRgXX<I __ 6 __ Long>);
         }
         bind(opcode | 0 << 6 | 7 << 3 | 0, AndRgXX<I __ 7 __ Byte>);
         bind(opcode | 0 << 6 | 7 << 3 | 1, AndRgXX<I __ 8 __ Byte>);

         bind(opcode | 1 << 6 | 7 << 3 | 0, AndRgXX<I __ 7 __ Word>);
         bind(opcode | 1 << 6 | 7 << 3 | 1, AndRgXX<I __ 8 __ Word>);

         bind(opcode | 2 << 6 | 7 << 3 | 0, AndRgXX<I __ 7 __ Long>);
         bind(opcode | 2 << 6 | 7 << 3 | 1, AndRgXX<I __ 8 __ Long>);
     }
}

template<Instr I> void
CPU::registerAddSub(const char *patternXXReg, const char *patternRegXX)
{
    u16 opcode;

    // ADD
    //
    // Modes: (1)   <ea>,Dy
    //               ------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               ------------------------------------------------
    //                X   X   X   X   X   X   X   X   X   X   X   X
    //
    //        (2)   Dx,<ea>
    //               ------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               ------------------------------------------------
    //                        X   X   X   X   X   X   X

    // (1)
    for (int dy = 0; dy < 8; dy++) {

        opcode = parse(patternXXReg) | dy << 9;
        for (int reg = 0; reg < 8; reg++) {

            bind(opcode | 0 << 6 | 0 << 3 | reg, AddXXRg<I __ 0 __ Byte>);
            // Byte size is not supported for addressing mode 1
            bind(opcode | 0 << 6 | 2 << 3 | reg, AddXXRg<I __ 2 __ Byte>);
            bind(opcode | 0 << 6 | 3 << 3 | reg, AddXXRg<I __ 3 __ Byte>);
            bind(opcode | 0 << 6 | 4 << 3 | reg, AddXXRg<I __ 4 __ Byte>);
            bind(opcode | 0 << 6 | 5 << 3 | reg, AddXXRg<I __ 5 __ Byte>);
            bind(opcode | 0 << 6 | 6 << 3 | reg, AddXXRg<I __ 6 __ Byte>);

            bind(opcode | 1 << 6 | 0 << 3 | reg, AddXXRg<I __ 0 __ Word>);
            bind(opcode | 1 << 6 | 1 << 3 | reg, AddXXRg<I __ 1 __ Word>);
            bind(opcode | 1 << 6 | 2 << 3 | reg, AddXXRg<I __ 2 __ Word>);
            bind(opcode | 1 << 6 | 3 << 3 | reg, AddXXRg<I __ 3 __ Word>);
            bind(opcode | 1 << 6 | 4 << 3 | reg, AddXXRg<I __ 4 __ Word>);
            bind(opcode | 1 << 6 | 5 << 3 | reg, AddXXRg<I __ 5 __ Word>);
            bind(opcode | 1 << 6 | 6 << 3 | reg, AddXXRg<I __ 6 __ Word>);

            bind(opcode | 2 << 6 | 0 << 3 | reg, AddXXRg<I __ 0 __ Long>);
            bind(opcode | 2 << 6 | 1 << 3 | reg, AddXXRg<I __ 1 __ Long>);
            bind(opcode | 2 << 6 | 2 << 3 | reg, AddXXRg<I __ 2 __ Long>);
            bind(opcode | 2 << 6 | 3 << 3 | reg, AddXXRg<I __ 3 __ Long>);
            bind(opcode | 2 << 6 | 4 << 3 | reg, AddXXRg<I __ 4 __ Long>);
            bind(opcode | 2 << 6 | 5 << 3 | reg, AddXXRg<I __ 5 __ Long>);
            bind(opcode | 2 << 6 | 6 << 3 | reg, AddXXRg<I __ 6 __ Long>);
        }
        bind(opcode | 0 << 6 | 7 << 3 | 0, AddXXRg<I __ 7 __ Byte>);
        bind(opcode | 0 << 6 | 7 << 3 | 1, AddXXRg<I __ 8 __ Byte>);
        bind(opcode | 0 << 6 | 7 << 3 | 2, AddXXRg<I __ 9 __ Byte>);
        bind(opcode | 0 << 6 | 7 << 3 | 3, AddXXRg<I __ 10 __ Byte>);
        bind(opcode | 0 << 6 | 7 << 3 | 4, AddXXRg<I __ 11 __ Byte>);

        bind(opcode | 1 << 6 | 7 << 3 | 0, AddXXRg<I __ 7 __ Word>);
        bind(opcode | 1 << 6 | 7 << 3 | 1, AddXXRg<I __ 8 __ Word>);
        bind(opcode | 1 << 6 | 7 << 3 | 2, AddXXRg<I __ 9 __ Word>);
        bind(opcode | 1 << 6 | 7 << 3 | 3, AddXXRg<I __ 10 __ Word>);
        bind(opcode | 1 << 6 | 7 << 3 | 4, AddXXRg<I __ 11 __ Word>);

        bind(opcode | 2 << 6 | 7 << 3 | 0, AddXXRg<I __ 7 __ Long>);
        bind(opcode | 2 << 6 | 7 << 3 | 1, AddXXRg<I __ 8 __ Long>);
        bind(opcode | 2 << 6 | 7 << 3 | 2, AddXXRg<I __ 9 __ Long>);
        bind(opcode | 2 << 6 | 7 << 3 | 3, AddXXRg<I __ 10 __ Long>);
        bind(opcode | 2 << 6 | 7 << 3 | 4, AddXXRg<I __ 11 __ Long>);
    }

    // (2)
     for (int dy = 0; dy < 8; dy++) {

         opcode = parse(patternRegXX) | dy << 9;
         for (int reg = 0; reg < 8; reg++) {

             bind(opcode | 0 << 6 | 2 << 3 | reg, AddRgXX<I __ 2 __ Byte>);
             bind(opcode | 0 << 6 | 3 << 3 | reg, AddRgXX<I __ 3 __ Byte>);
             bind(opcode | 0 << 6 | 4 << 3 | reg, AddRgXX<I __ 4 __ Byte>);
             bind(opcode | 0 << 6 | 5 << 3 | reg, AddRgXX<I __ 5 __ Byte>);
             bind(opcode | 0 << 6 | 6 << 3 | reg, AddRgXX<I __ 6 __ Byte>);

             bind(opcode | 1 << 6 | 2 << 3 | reg, AddRgXX<I __ 2 __ Word>);
             bind(opcode | 1 << 6 | 3 << 3 | reg, AddRgXX<I __ 3 __ Word>);
             bind(opcode | 1 << 6 | 4 << 3 | reg, AddRgXX<I __ 4 __ Word>);
             bind(opcode | 1 << 6 | 5 << 3 | reg, AddRgXX<I __ 5 __ Word>);
             bind(opcode | 1 << 6 | 6 << 3 | reg, AddRgXX<I __ 6 __ Word>);

             bind(opcode | 2 << 6 | 2 << 3 | reg, AddRgXX<I __ 2 __ Long>);
             bind(opcode | 2 << 6 | 3 << 3 | reg, AddRgXX<I __ 3 __ Long>);
             bind(opcode | 2 << 6 | 4 << 3 | reg, AddRgXX<I __ 4 __ Long>);
             bind(opcode | 2 << 6 | 5 << 3 | reg, AddRgXX<I __ 5 __ Long>);
             bind(opcode | 2 << 6 | 6 << 3 | reg, AddRgXX<I __ 6 __ Long>);
         }
         bind(opcode | 0 << 6 | 7 << 3 | 0, AddRgXX<I __ 7 __ Byte>);
         bind(opcode | 0 << 6 | 7 << 3 | 1, AddRgXX<I __ 8 __ Byte>);

         bind(opcode | 1 << 6 | 7 << 3 | 0, AddRgXX<I __ 7 __ Word>);
         bind(opcode | 1 << 6 | 7 << 3 | 1, AddRgXX<I __ 8 __ Word>);

         bind(opcode | 2 << 6 | 7 << 3 | 0, AddRgXX<I __ 7 __ Long>);
         bind(opcode | 2 << 6 | 7 << 3 | 1, AddRgXX<I __ 8 __ Long>);
     }
}

template<Instr I> void
CPU::registerClr(const char *pattern)
{
     // CLR
     //
     // Modes:       <ea>
     //               ------------------------------------------------
     //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
     //               ------------------------------------------------
     //                X       X   X   X   X   X   X   X

     u32 opcode = parse(pattern);

     for (int reg = 0; reg < 8; reg++) {

         bind(opcode | 0 << 6 | 0 << 3 | reg, Clr<I __ 0 __ Byte>);
         bind(opcode | 0 << 6 | 2 << 3 | reg, Clr<I __ 2 __ Byte>);
         bind(opcode | 0 << 6 | 3 << 3 | reg, Clr<I __ 3 __ Byte>);
         bind(opcode | 0 << 6 | 4 << 3 | reg, Clr<I __ 4 __ Byte>);
         bind(opcode | 0 << 6 | 5 << 3 | reg, Clr<I __ 5 __ Byte>);
         bind(opcode | 0 << 6 | 6 << 3 | reg, Clr<I __ 6 __ Byte>);

         bind(opcode | 1 << 6 | 0 << 3 | reg, Clr<I __ 0 __ Word>);
         bind(opcode | 1 << 6 | 2 << 3 | reg, Clr<I __ 2 __ Word>);
         bind(opcode | 1 << 6 | 3 << 3 | reg, Clr<I __ 3 __ Word>);
         bind(opcode | 1 << 6 | 4 << 3 | reg, Clr<I __ 4 __ Word>);
         bind(opcode | 1 << 6 | 5 << 3 | reg, Clr<I __ 5 __ Word>);
         bind(opcode | 1 << 6 | 6 << 3 | reg, Clr<I __ 6 __ Word>);

         bind(opcode | 2 << 6 | 0 << 3 | reg, Clr<I __ 0 __ Long>);
         bind(opcode | 2 << 6 | 2 << 3 | reg, Clr<I __ 2 __ Long>);
         bind(opcode | 2 << 6 | 3 << 3 | reg, Clr<I __ 3 __ Long>);
         bind(opcode | 2 << 6 | 4 << 3 | reg, Clr<I __ 4 __ Long>);
         bind(opcode | 2 << 6 | 5 << 3 | reg, Clr<I __ 5 __ Long>);
         bind(opcode | 2 << 6 | 6 << 3 | reg, Clr<I __ 6 __ Long>);
     }
    bind(opcode | 0 << 6 | 7 << 3 | 0, Clr<I __ 7 __ Byte>);
    bind(opcode | 0 << 6 | 7 << 3 | 1, Clr<I __ 8 __ Byte>);
    bind(opcode | 1 << 6 | 7 << 3 | 0, Clr<I __ 7 __ Word>);
    bind(opcode | 1 << 6 | 7 << 3 | 1, Clr<I __ 8 __ Word>);
    bind(opcode | 2 << 6 | 7 << 3 | 0, Clr<I __ 7 __ Long>);
    bind(opcode | 2 << 6 | 7 << 3 | 1, Clr<I __ 8 __ Long>);
}

void
CPU::registerADD()
{
    registerAddSub<ADD>("1101 ---0 ---- ----",  // <ea>,Dy
                        "1101 ---1 ---- ----"); // Dx,<ea>
}

void
CPU::registerAND()
{
    registerLogic<AND>("1100 ---0 ---- ----",  // <ea>,Dy
                       "1100 ---1 ---- ----"); // Dx,<ea>
}

void
CPU::registerASL()
{
    registerShift<ASL>("1110 ---1 --10 0---",  // Dx,Dy
                       "1110 ---1 --00 0---",  // ##,Dy
                       "1110 0001 11-- ----"); // <ea>
}

void
CPU::registerASR()
{
    registerShift<ASR>("1110 ---0 --10 0---",  // Dx,Dy
                       "1110 ---0 --00 0---",  // ##,Dy
                       "1110 0000 11-- ----"); // <ea>
}

void
CPU::registerCLR()
{
    registerClr<CLR>("0100 0010 ---- ----");   // <ea>
}

void
CPU::registerDBcc()
{
    u32 opcode = parse("0101 ---- 1100 1---");

    for (int reg = 0; reg < 8; reg++) {

        bind(opcode | 0x0 << 8 | reg, Dbcc<CT>);
        bind(opcode | 0x1 << 8 | reg, Dbcc<CF>);
        bind(opcode | 0x2 << 8 | reg, Dbcc<HI>);
        bind(opcode | 0x3 << 8 | reg, Dbcc<LS>);
        bind(opcode | 0x4 << 8 | reg, Dbcc<CC>);
        bind(opcode | 0x5 << 8 | reg, Dbcc<CS>);
        bind(opcode | 0x6 << 8 | reg, Dbcc<NE>);
        bind(opcode | 0x7 << 8 | reg, Dbcc<EQ>);
        bind(opcode | 0x8 << 8 | reg, Dbcc<VC>);
        bind(opcode | 0x9 << 8 | reg, Dbcc<VS>);
        bind(opcode | 0xA << 8 | reg, Dbcc<PL>);
        bind(opcode | 0xB << 8 | reg, Dbcc<MI>);
        bind(opcode | 0xC << 8 | reg, Dbcc<GE>);
        bind(opcode | 0xD << 8 | reg, Dbcc<LT>);
        bind(opcode | 0xE << 8 | reg, Dbcc<GT>);
        bind(opcode | 0xF << 8 | reg, Dbcc<LE>);
    }
}

void
CPU::registerEXT()
{
    u32 opcode = parse("0100 1000 --00 0---");

    for (int reg = 0; reg < 8; reg++) {
        bind(opcode | 2 << 6 | reg, Ext<Word>);
        bind(opcode | 3 << 6 | reg, Ext<Long>);
    }
}

void
CPU::registerLSL()
{
    registerShift<LSL>("1110 ---1 --10 1---",  // Dx,Dy
                       "1110 ---1 --00 1---",  // ##,Dy
                       "1110 0011 11-- ----"); // <ea>
}

void
CPU::registerLSR()
{
    registerShift<LSR>("1110 ---0 --10 1---",  // Dx,Dy
                       "1110 ---0 --00 1---",  // ##,Dy
                       "1110 0010 11-- ----"); // <ea>
}

void
CPU::registerNOP()
{
    bind(parse("0100 1110 0111 0001"), Nop);
}

void
CPU::registerROL()
{
    registerShift<ROL>("1110 ---1 --11 1---",  // Dx,Dy
                       "1110 ---1 --01 1---",  // ##,Dy
                       "1110 0111 11-- ----"); // <ea>
}

void
CPU::registerROR()
{
    registerShift<ROR>("1110 ---0 --11 1---",  // Dx,Dy
                       "1110 ---0 --01 1---",  // ##,Dy
                       "1110 0110 11-- ----"); // <ea>
}

void
CPU::registerROXL()
{
    registerShift<ROXL>("1110 ---1 --11 0---",  // Dx,Dy
                        "1110 ---1 --01 0---",  // ##,Dy
                        "1110 0101 11-- ----"); // <ea>
}

void
CPU::registerROXR()
{
    registerShift<ROXR>("1110 ---0 --11 0---",  // Dx,Dy
                        "1110 ---0 --01 0---",  // ##,Dy
                        "1110 0100 11-- ----"); // <ea>
}

void
CPU::registerSUB()
{
    registerAddSub<SUB>("1001 ---0 ---- ----",  // <ea>,Dy
                        "1001 ---1 ---- ----"); // Dx,<ea>
}

void
CPU::registerLEA()
{
    u16 opcode;

    // LEA
    //
    // Modes:       LEA <ea>,An
    //
    //               ------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               ------------------------------------------------
    //                        X           X   X   X   X   X   X

    for (int an = 0; an < 8; an++) {

        opcode = parse("0100 ---1 11-- ----") | an << 9;
        for (int reg = 0; reg < 8; reg++) {
            bind(opcode | 2 << 3 | reg, Lea<2>);
            bind(opcode | 5 << 3 | reg, Lea<5>);
            bind(opcode | 6 << 3 | reg, Lea<6>);
        }
        bind(opcode | 7 << 3 | 0, Lea<7>);
        bind(opcode | 7 << 3 | 1, Lea<8>);
        bind(opcode | 7 << 3 | 2, Lea<9>);
        bind(opcode | 7 << 3 | 3, Lea<10>);
    }
}

#undef __
