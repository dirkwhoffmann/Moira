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
#define register(id, name) { \
assert(exec[id] == &CPU::execIllegal); \
assert(dasm[id] == &CPU::dasmIllegal); \
assert(sync[id] == &CPU::syncIllegal); \
exec[id] = &CPU::exec##name; \
dasm[id] = &CPU::dasm##name; \
sync[id] = &CPU::sync##name; \
}
*/


// Adds a single entry to the instruction jump table
#define __ ,
#define register(id, name) { \
if (exec[id] != &CPU::execIllegal) printf("id = %x\n", id); \
assert(exec[id] == &CPU::execIllegal); \
assert(dasm[id] == &CPU::dasmIllegal); \
exec[id] = &CPU::exec##name; \
dasm[id] = &CPU::dasm##name; }

// Registers an instruction in one of the standard instruction formats:
//
//     Variants:  ____ ____ ____ _XXX
//                ____ XXX_ ____ _XXX
//                ____ XXX_ SS__ _XXX
//                ____ ____ __MM MXXX
//                ____ ___S __MM MXXX
//                ____ XXX_ __MM MXXX
//                ____ XXXS __MM MXXX
//                ____ XXX_ SSMM MXXX
//
//       Legend:  XXX : Operand parameters    (0 .. 7)
//                  S : Size information      (0 = Word, 1 = Long)
//                 SS : Size information      (0 = Byte, 1 = Word, 2 = Long)

#define _____________XXX(op,I,M,S,f) { \
for (int j = 0; j < 8; j++) register((op) | j, f<I __ M __ S>); }

#define ____XXX______XXX(op,I,M,S,f) { \
for (int i = 0; i < 8; i++) _____________XXX((op) | i << 9, I, M, S, f); }

#define ____XXX_SS___XXX(op,I,M,s,f) { \
if ((s) & 0b100) ____XXX______XXX((op) | 2 << 6, I, M, Long, f); \
if ((s) & 0b010) ____XXX______XXX((op) | 1 << 6, I, M, Word, f); \
if ((s) & 0b001) ____XXX______XXX((op) | 0 << 6, I, M, Byte, f); }

#define __________MMMXXX(op,I,m,S,f) { \
for (int j = 0; j < 8; j++) { \
if ((m) & 0b100000000000) register((op) | 0 << 3 | j, f<I __  0 __ S>); \
if ((m) & 0b010000000000) register((op) | 1 << 3 | j, f<I __  1 __ S>); \
if ((m) & 0b001000000000) register((op) | 2 << 3 | j, f<I __  2 __ S>); \
if ((m) & 0b000100000000) register((op) | 3 << 3 | j, f<I __  3 __ S>); \
if ((m) & 0b000010000000) register((op) | 4 << 3 | j, f<I __  4 __ S>); \
if ((m) & 0b000001000000) register((op) | 5 << 3 | j, f<I __  5 __ S>); \
if ((m) & 0b000000100000) register((op) | 6 << 3 | j, f<I __  6 __ S>); \
} \
if ((m) & 0b000000010000) register((op) | 7 << 3 | 0, f<I __  7 __ S>); \
if ((m) & 0b000000001000) register((op) | 7 << 3 | 1, f<I __  8 __ S>); \
if ((m) & 0b000000000100) register((op) | 7 << 3 | 2, f<I __  9 __ S>); \
if ((m) & 0b000000000010) register((op) | 7 << 3 | 3, f<I __ 10 __ S>); \
if ((m) & 0b000000000001) register((op) | 7 << 3 | 4, f<I __ 11 __ S>); }

#define _______S__MMMXXX(op,I,m,s,f) { \
if ((s) & 0b100) __________MMMXXX((op) | 1 << 8, I, m, Long, f); \
if ((s) & 0b010) __________MMMXXX((op) | 0 << 8, I, m, Word, f); \
if ((s) & 0b001) assert(false); }

#define ________SSMMMXXX(op,I,m,s,f) { \
if ((s) & 0b100) __________MMMXXX((op) | 2 << 6, I, m, Long, f); \
if ((s) & 0b010) __________MMMXXX((op) | 1 << 6, I, m, Word, f); \
if ((s) & 0b001) __________MMMXXX((op) | 0 << 6, I, m, Byte, f); }

#define ____XXX___MMMXXX(op,I,m,S,f) { \
for (int i = 0; i < 8; i++) __________MMMXXX((op) | i << 9, I, m, S, f) }

#define ____XXXS__MMMXXX(op,I,m,s,f) { \
for (int i = 0; i < 8; i++) _______S__MMMXXX((op) | i << 9, I, m, s, f) }

#define ____XXX_SSMMMXXX(op,I,m,s,f) { \
for (int i = 0; i < 8; i++) ________SSMMMXXX((op) | i << 9, I, m, s, f) }

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
    registerAND();
    registerABCD();
    registerADD();
    registerADDA();
    registerASL();
    registerASR();
    registerBCHG();
    registerBCLR();
    registerBSET();
    registerBTST();
    registerCLR();
    registerCMP();
    registerCMPA();
    registerDBcc();
    registerDIVS();
    registerDIVU();
    registerEOR();
    registerEXT();
    registerLEA();
    registerLSL();
    registerLSR();
    registerMOVEA();
    registerMOVEQ();
    registerMULS();
    registerMULU();
    registerNBCD();
    registerNEG();
    registerNEGX();
    registerNOT();
    registerNOP();
    registerOR();
    registerROL();
    registerROR();
    registerROXL();
    registerROXR();
    registerSBCD();
    registerScc();
    registerSUB();
    registerSUBA();
    registerTAS();
    registerTST();
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
    u16 opcode1 = parse(patternReg);
    u16 opcode2 = parse(patternImm);
    u16 opcode3 = parse(patternEa);

    // ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR
    //
    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // Dx,Dy          X
    // #<data>,Dy                                                 X
    // <ea>                   X   X   X   X   X   X   X

    // (1)
    ____XXX_SS___XXX(opcode1, I, 0x0, Byte | Word | Long, ShiftRg);
    ____XXX_SS___XXX(opcode2, I, 0xB, Byte | Word | Long, ShiftIm);

    /*
    for (int dx = 0; dx < 8; dx++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse(patternReg) | dx << 9 | dy;
            register(opcode | 0 << 6, Shift<I __ 0 __ Byte>);
            register(opcode | 1 << 6, Shift<I __ 0 __ Word>);
            register(opcode | 2 << 6, Shift<I __ 0 __ Long>);
        }
    }
    */

    // (2)

    /*
    for (int data = 0; data < 8; data++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse(patternImm) | data << 9 | dy;
            register(opcode | 0 << 6, Shift<I __ 11 __ Byte>);
            register(opcode | 1 << 6, Shift<I __ 11 __ Word>);
            register(opcode | 2 << 6, Shift<I __ 11 __ Long>);
        }
    }
    */

    // (3)
    opcode = parse(patternEa);
    for (int ax = 0; ax < 8; ax++) {
        register(opcode | 2 << 3 | ax, Shift<I __ 2 __ Word>);
        register(opcode | 3 << 3 | ax, Shift<I __ 3 __ Word>);
        register(opcode | 4 << 3 | ax, Shift<I __ 4 __ Word>);
        register(opcode | 5 << 3 | ax, Shift<I __ 5 __ Word>);
        register(opcode | 6 << 3 | ax, Shift<I __ 6 __ Word>);
    }
    register(opcode | 7 << 3 | 0, Shift<I __ 7 __ Word>);
    register(opcode | 7 << 3 | 1, Shift<I __ 8 __ Word>);
}

template<Instr I> void
CPU::registerAbcdSbcd(const char *patternReg, const char *patternInd)
{
    u32 opcode;

    // ABCD, SBCD
    //
    // Modes: (1)   Dx,Dy
    //        (2)   -(Ay),-(Ax)

    // (1)
    opcode = parse(patternReg);
    for (int src = 0; src < 8; src++) {
        for (int dst = 0; dst < 8; dst++) {
            register(opcode | dst << 9 | src, Abcd<I __ 0 __ Byte>);
        }
    }

    // (2)
    opcode = parse(patternInd);
    for (int src = 0; src < 8; src++) {
        for (int dst = 0; dst < 8; dst++) {
            register(opcode | dst << 9 | src, Abcd<I __ 4 __ Byte>);
        }
    }
}

template<Instr I> void
CPU::registerAddSubEaRg(const char *pattern)
{
    // Mode :       <ea>,Dy
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X   X   X   X   X   X   X   X   X   X   X   X

    u16 opcode = parse(pattern);
    u16 opcodeB = opcode | 0 << 6;
    u16 opcodeW = opcode | 1 << 6;
    u16 opcodeL = opcode | 2 << 6;

    for (int dy = 0; dy < 8; dy++) {
        for (int reg = 0; reg < 8; reg++) {

            register(opcodeB | dy << 9 | 0 << 3 | reg, AddXXRg<I __ 0 __ Byte>);
            // Byte size is not supported for addressing mode 1
            register(opcodeB | dy << 9 | 2 << 3 | reg, AddXXRg<I __ 2 __ Byte>);
            register(opcodeB | dy << 9 | 3 << 3 | reg, AddXXRg<I __ 3 __ Byte>);
            register(opcodeB | dy << 9 | 4 << 3 | reg, AddXXRg<I __ 4 __ Byte>);
            register(opcodeB | dy << 9 | 5 << 3 | reg, AddXXRg<I __ 5 __ Byte>);
            register(opcodeB | dy << 9 | 6 << 3 | reg, AddXXRg<I __ 6 __ Byte>);

            register(opcodeW | dy << 9 | 0 << 3 | reg, AddXXRg<I __ 0 __ Word>);
            register(opcodeW | dy << 9 | 1 << 3 | reg, AddXXRg<I __ 1 __ Word>);
            register(opcodeW | dy << 9 | 2 << 3 | reg, AddXXRg<I __ 2 __ Word>);
            register(opcodeW | dy << 9 | 3 << 3 | reg, AddXXRg<I __ 3 __ Word>);
            register(opcodeW | dy << 9 | 4 << 3 | reg, AddXXRg<I __ 4 __ Word>);
            register(opcodeW | dy << 9 | 5 << 3 | reg, AddXXRg<I __ 5 __ Word>);
            register(opcodeW | dy << 9 | 6 << 3 | reg, AddXXRg<I __ 6 __ Word>);

            register(opcodeL | dy << 9 | 0 << 3 | reg, AddXXRg<I __ 0 __ Long>);
            register(opcodeL | dy << 9 | 1 << 3 | reg, AddXXRg<I __ 1 __ Long>);
            register(opcodeL | dy << 9 | 2 << 3 | reg, AddXXRg<I __ 2 __ Long>);
            register(opcodeL | dy << 9 | 3 << 3 | reg, AddXXRg<I __ 3 __ Long>);
            register(opcodeL | dy << 9 | 4 << 3 | reg, AddXXRg<I __ 4 __ Long>);
            register(opcodeL | dy << 9 | 5 << 3 | reg, AddXXRg<I __ 5 __ Long>);
            register(opcodeL | dy << 9 | 6 << 3 | reg, AddXXRg<I __ 6 __ Long>);
        }
        register(opcodeB | dy << 9 | 7 << 3 | 0, AddXXRg<I __  7 __ Byte>);
        register(opcodeB | dy << 9 | 7 << 3 | 1, AddXXRg<I __  8 __ Byte>);
        register(opcodeB | dy << 9 | 7 << 3 | 2, AddXXRg<I __  9 __ Byte>);
        register(opcodeB | dy << 9 | 7 << 3 | 3, AddXXRg<I __ 10 __ Byte>);
        register(opcodeB | dy << 9 | 7 << 3 | 4, AddXXRg<I __ 11 __ Byte>);

        register(opcodeW | dy << 9 | 7 << 3 | 0, AddXXRg<I __  7 __ Word>);
        register(opcodeW | dy << 9 | 7 << 3 | 1, AddXXRg<I __  8 __ Word>);
        register(opcodeW | dy << 9 | 7 << 3 | 2, AddXXRg<I __  9 __ Word>);
        register(opcodeW | dy << 9 | 7 << 3 | 3, AddXXRg<I __ 10 __ Word>);
        register(opcodeW | dy << 9 | 7 << 3 | 4, AddXXRg<I __ 11 __ Word>);

        register(opcodeL | dy << 9 | 7 << 3 | 0, AddXXRg<I __  7 __ Long>);
        register(opcodeL | dy << 9 | 7 << 3 | 1, AddXXRg<I __  8 __ Long>);
        register(opcodeL | dy << 9 | 7 << 3 | 2, AddXXRg<I __  9 __ Long>);
        register(opcodeL | dy << 9 | 7 << 3 | 3, AddXXRg<I __ 10 __ Long>);
        register(opcodeL | dy << 9 | 7 << 3 | 4, AddXXRg<I __ 11 __ Long>);
    }
}

template<Instr I> void
CPU::registerAddSubRgEa(const char *pattern)
{
    // Mode:        Dx,<ea>
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                        X   X   X   X   X   X   X

    u16 opcode = parse(pattern);
    u16 opcodeB = opcode | 0 << 6;
    u16 opcodeW = opcode | 1 << 6;
    u16 opcodeL = opcode | 2 << 6;

    for (int dy = 0; dy < 8; dy++) {
        for (int reg = 0; reg < 8; reg++) {

            register(opcodeB | dy << 9 | 2 << 3 | reg, AddRgXX<I __ 2 __ Byte>);
            register(opcodeB | dy << 9 | 3 << 3 | reg, AddRgXX<I __ 3 __ Byte>);
            register(opcodeB | dy << 9 | 4 << 3 | reg, AddRgXX<I __ 4 __ Byte>);
            register(opcodeB | dy << 9 | 5 << 3 | reg, AddRgXX<I __ 5 __ Byte>);
            register(opcodeB | dy << 9 | 6 << 3 | reg, AddRgXX<I __ 6 __ Byte>);

            register(opcodeW | dy << 9 | 2 << 3 | reg, AddRgXX<I __ 2 __ Word>);
            register(opcodeW | dy << 9 | 3 << 3 | reg, AddRgXX<I __ 3 __ Word>);
            register(opcodeW | dy << 9 | 4 << 3 | reg, AddRgXX<I __ 4 __ Word>);
            register(opcodeW | dy << 9 | 5 << 3 | reg, AddRgXX<I __ 5 __ Word>);
            register(opcodeW | dy << 9 | 6 << 3 | reg, AddRgXX<I __ 6 __ Word>);

            register(opcodeL | dy << 9 | 2 << 3 | reg, AddRgXX<I __ 2 __ Long>);
            register(opcodeL | dy << 9 | 3 << 3 | reg, AddRgXX<I __ 3 __ Long>);
            register(opcodeL | dy << 9 | 4 << 3 | reg, AddRgXX<I __ 4 __ Long>);
            register(opcodeL | dy << 9 | 5 << 3 | reg, AddRgXX<I __ 5 __ Long>);
            register(opcodeL | dy << 9 | 6 << 3 | reg, AddRgXX<I __ 6 __ Long>);
        }
        register(opcodeB | dy << 9 | 7 << 3 | 0, AddRgXX<I __ 7 __ Byte>);
        register(opcodeB | dy << 9 | 7 << 3 | 1, AddRgXX<I __ 8 __ Byte>);

        register(opcodeW | dy << 9 | 7 << 3 | 0, AddRgXX<I __ 7 __ Word>);
        register(opcodeW | dy << 9 | 7 << 3 | 1, AddRgXX<I __ 8 __ Word>);

        register(opcodeL | dy << 9 | 7 << 3 | 0, AddRgXX<I __ 7 __ Long>);
        register(opcodeL | dy << 9 | 7 << 3 | 1, AddRgXX<I __ 8 __ Long>);
    }
}

template<Instr I> void
CPU::registerAddSubA(const char *pattern)
{
    // ADDA, SUBA
    //
    // Modes:       <ea>,An
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X   X   X   X   X   X   X   X   X   X   X   X

    u16 opcode = parse(pattern);
    u16 opcodeW = opcode | 0 << 8;
    u16 opcodeL = opcode | 1 << 8;

    for (int dst = 0; dst < 8; dst++) {

        for (int src = 0; src < 8; src++) {

            register(opcodeW | dst << 9 | 0 << 3 | src, AddaDn<I __ 0 __ Word>);
            register(opcodeW | dst << 9 | 1 << 3 | src, AddaEa<I __ 1 __ Word>);
            register(opcodeW | dst << 9 | 2 << 3 | src, AddaEa<I __ 2 __ Word>);
            register(opcodeW | dst << 9 | 3 << 3 | src, AddaEa<I __ 3 __ Word>);
            register(opcodeW | dst << 9 | 4 << 3 | src, AddaEa<I __ 4 __ Word>);
            register(opcodeW | dst << 9 | 5 << 3 | src, AddaEa<I __ 5 __ Word>);
            register(opcodeW | dst << 9 | 6 << 3 | src, AddaEa<I __ 6 __ Word>);

            register(opcodeL | dst << 9 | 0 << 3 | src, AddaEa<I __ 0 __ Long>);
            register(opcodeL | dst << 9 | 1 << 3 | src, AddaEa<I __ 1 __ Long>);
            register(opcodeL | dst << 9 | 2 << 3 | src, AddaEa<I __ 2 __ Long>);
            register(opcodeL | dst << 9 | 3 << 3 | src, AddaEa<I __ 3 __ Long>);
            register(opcodeL | dst << 9 | 4 << 3 | src, AddaEa<I __ 4 __ Long>);
            register(opcodeL | dst << 9 | 5 << 3 | src, AddaEa<I __ 5 __ Long>);
            register(opcodeL | dst << 9 | 6 << 3 | src, AddaEa<I __ 6 __ Long>);

        }
        register(opcodeW | dst << 9 | 7 << 3 | 0, AddaEa<I __  7 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 1, AddaEa<I __  8 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 2, AddaEa<I __  9 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 3, AddaEa<I __ 10 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 4, AddaIm<I __ 11 __ Word>);

        register(opcodeL | dst << 9 | 7 << 3 | 0, AddaEa<I __  7 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 1, AddaEa<I __  8 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 2, AddaEa<I __  9 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 3, AddaEa<I __ 10 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 4, AddaIm<I __ 11 __ Long>);
    }
}

template<Instr I> void
CPU::registerMulDiv(const char *pattern)
{
    // DIVS, DIVU, MULS, MULU
    //
    // Modes: (1)   <ea>,Dy
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X   X   X   X

    u16 opcode = parse(pattern);

    ____XXX___MMMXXX(opcode, I, 0b101111111111, Long, MulDiv);

    /*
    for (int dst = 0; dst < 8; dst++) {
        for (int src = 0; src < 8; src++) {

            register(opcode | dst << 9 | 0 << 3 | src, MulDiv<I __ 0>);
            register(opcode | dst << 9 | 2 << 3 | src, MulDiv<I __ 2>);
            register(opcode | dst << 9 | 3 << 3 | src, MulDiv<I __ 3>);
            register(opcode | dst << 9 | 4 << 3 | src, MulDiv<I __ 4>);
            register(opcode | dst << 9 | 5 << 3 | src, MulDiv<I __ 5>);
            register(opcode | dst << 9 | 6 << 3 | src, MulDiv<I __ 6>);
        }
        register(opcode | dst << 9 | 7 << 3 | 0, MulDiv<I __  7>);
        register(opcode | dst << 9 | 7 << 3 | 1, MulDiv<I __  8>);
        register(opcode | dst << 9 | 7 << 3 | 2, MulDiv<I __  9>);
        register(opcode | dst << 9 | 7 << 3 | 3, MulDiv<I __ 10>);
        register(opcode | dst << 9 | 7 << 3 | 4, MulDiv<I __ 11>);
     }
     */
}

template<Instr I> void
CPU::registerNegNot(const char *pattern)
{
    // NEG, NEGX, NOT
    //
    // Modes:       <ea>
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X

    u16 opcode = parse(pattern);
    u16 opcodeB = opcode | 0 << 6;
    u16 opcodeW = opcode | 1 << 6;
    u16 opcodeL = opcode | 2 << 6;

    for (int dst = 0; dst < 8; dst++) {

        register(opcodeB | 0 << 3 | dst, NegNotDn<I __ 0 __ Byte>);
        register(opcodeB | 2 << 3 | dst, NegNotEa<I __ 2 __ Byte>);
        register(opcodeB | 3 << 3 | dst, NegNotEa<I __ 3 __ Byte>);
        register(opcodeB | 4 << 3 | dst, NegNotEa<I __ 4 __ Byte>);
        register(opcodeB | 5 << 3 | dst, NegNotEa<I __ 5 __ Byte>);
        register(opcodeB | 6 << 3 | dst, NegNotEa<I __ 6 __ Byte>);

        register(opcodeW | 0 << 3 | dst, NegNotDn<I __ 0 __ Word>);
        register(opcodeW | 2 << 3 | dst, NegNotEa<I __ 2 __ Word>);
        register(opcodeW | 3 << 3 | dst, NegNotEa<I __ 3 __ Word>);
        register(opcodeW | 4 << 3 | dst, NegNotEa<I __ 4 __ Word>);
        register(opcodeW | 5 << 3 | dst, NegNotEa<I __ 5 __ Word>);
        register(opcodeW | 6 << 3 | dst, NegNotEa<I __ 6 __ Word>);

        register(opcodeL | 0 << 3 | dst, NegNotDn<I __ 0 __ Long>);
        register(opcodeL | 2 << 3 | dst, NegNotEa<I __ 2 __ Long>);
        register(opcodeL | 3 << 3 | dst, NegNotEa<I __ 3 __ Long>);
        register(opcodeL | 4 << 3 | dst, NegNotEa<I __ 4 __ Long>);
        register(opcodeL | 5 << 3 | dst, NegNotEa<I __ 5 __ Long>);
        register(opcodeL | 6 << 3 | dst, NegNotEa<I __ 6 __ Long>);

    }
    register(opcodeB | 7 << 3 | 0, NegNotEa<I __  7 __ Byte>);
    register(opcodeB | 7 << 3 | 1, NegNotEa<I __  8 __ Byte>);

    register(opcodeW | 7 << 3 | 0, NegNotEa<I __  7 __ Word>);
    register(opcodeW | 7 << 3 | 1, NegNotEa<I __  8 __ Word>);

    register(opcodeL | 7 << 3 | 0, NegNotEa<I __  7 __ Long>);
    register(opcodeL | 7 << 3 | 1, NegNotEa<I __  8 __ Long>);
}

template<Instr I> void
CPU::registerClr(const char *pattern)
{
    u32 opcode = parse(pattern);

    // CLR
    //
    // Modes:       <ea>
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X

    for (int reg = 0; reg < 8; reg++) {

        register(opcode | 0 << 6 | 0 << 3 | reg, Clr<I __ 0 __ Byte>);
        register(opcode | 0 << 6 | 2 << 3 | reg, Clr<I __ 2 __ Byte>);
        register(opcode | 0 << 6 | 3 << 3 | reg, Clr<I __ 3 __ Byte>);
        register(opcode | 0 << 6 | 4 << 3 | reg, Clr<I __ 4 __ Byte>);
        register(opcode | 0 << 6 | 5 << 3 | reg, Clr<I __ 5 __ Byte>);
        register(opcode | 0 << 6 | 6 << 3 | reg, Clr<I __ 6 __ Byte>);

        register(opcode | 1 << 6 | 0 << 3 | reg, Clr<I __ 0 __ Word>);
        register(opcode | 1 << 6 | 2 << 3 | reg, Clr<I __ 2 __ Word>);
        register(opcode | 1 << 6 | 3 << 3 | reg, Clr<I __ 3 __ Word>);
        register(opcode | 1 << 6 | 4 << 3 | reg, Clr<I __ 4 __ Word>);
        register(opcode | 1 << 6 | 5 << 3 | reg, Clr<I __ 5 __ Word>);
        register(opcode | 1 << 6 | 6 << 3 | reg, Clr<I __ 6 __ Word>);

        register(opcode | 2 << 6 | 0 << 3 | reg, Clr<I __ 0 __ Long>);
        register(opcode | 2 << 6 | 2 << 3 | reg, Clr<I __ 2 __ Long>);
        register(opcode | 2 << 6 | 3 << 3 | reg, Clr<I __ 3 __ Long>);
        register(opcode | 2 << 6 | 4 << 3 | reg, Clr<I __ 4 __ Long>);
        register(opcode | 2 << 6 | 5 << 3 | reg, Clr<I __ 5 __ Long>);
        register(opcode | 2 << 6 | 6 << 3 | reg, Clr<I __ 6 __ Long>);
    }
    register(opcode | 0 << 6 | 7 << 3 | 0, Clr<I __ 7 __ Byte>);
    register(opcode | 0 << 6 | 7 << 3 | 1, Clr<I __ 8 __ Byte>);
    register(opcode | 1 << 6 | 7 << 3 | 0, Clr<I __ 7 __ Word>);
    register(opcode | 1 << 6 | 7 << 3 | 1, Clr<I __ 8 __ Word>);
    register(opcode | 2 << 6 | 7 << 3 | 0, Clr<I __ 7 __ Long>);
    register(opcode | 2 << 6 | 7 << 3 | 1, Clr<I __ 8 __ Long>);
}

void
CPU::registerABCD()
{
    registerAbcdSbcd<ABCD>("1100 ---1 0000 0---",   // Dx,Dy
                           "1100 ---1 0000 1---");  // -(Ax),-(Ay)
}

void
CPU::registerADD()
{
    registerAddSubEaRg<ADD>("1101 ---0 ---- ----"); // <ea>,Dy
    registerAddSubRgEa<ADD>("1101 ---1 ---- ----"); // Dx,<ea>
}

void
CPU::registerADDA()
{
    registerAddSubA<ADDA>("1101 ---- 11-- ----");  // <ea>,Ay
}

void
CPU::registerAND()
{
    u16 opcode1 = parse("1100 ---0 ---- ----");
    u16 opcode2 = parse("1100 ---1 ---- ----");

    // AND
    //            -------------------------------------------------
    //            | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //            -------------------------------------------------
    // <ea>,Dy      X       X   X   X   X   X   X   X   X   X   X
    // Dx,<ea>              X   X   X   X   X   X   X

    ____XXX_SSMMMXXX(opcode1, AND, 0b101111111111, Byte | Word | Long, AndEaRg);
    ____XXX_SSMMMXXX(opcode2, AND, 0b001111111000, Byte | Word | Long, AndRgEa);
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
CPU::registerBCHG()
{
    u16 opcode1 = parse("0000 ---1 01-- ----");
    u16 opcode2 = parse("0000 1000 01-- ----");

    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // Dx,<ea>        X       X   X   X   X   X   X   X
    // #<data>,<ea>   X       X   X   X   X   X   X   X

    ____XXX___MMMXXX(opcode1, BCHG, 0b101111111000, Long, BitDxEa);
    __________MMMXXX(opcode2, BCHG, 0b101111111000, Long, BitImEa);
}

void
CPU::registerBCLR()
{
    u16 opcode1 = parse("0000 ---1 10-- ----");
    u16 opcode2 = parse("0000 1000 10-- ----");

    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // Dx,<ea>        X       X   X   X   X   X   X   X
    // #<data>,<ea>   X       X   X   X   X   X   X   X

    ____XXX___MMMXXX(opcode1, BCLR, 0b101111111000, Long, BitDxEa);
    __________MMMXXX(opcode2, BCLR, 0b101111111000, Long, BitImEa);
}

void
CPU::registerBSET()
{
    u16 opcode1 = parse("0000 ---1 11-- ----");
    u16 opcode2 = parse("0000 1000 11-- ----");

    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // Dx,<ea>        X       X   X   X   X   X   X   X
    // #<data>,<ea>   X       X   X   X   X   X   X   X

    ____XXX___MMMXXX(opcode1, BSET, 0b101111111000, Long, BitDxEa);
    __________MMMXXX(opcode2, BSET, 0b101111111000, Long, BitImEa);
}

void
CPU::registerBTST()
{
    u16 opcode1 = parse("0000 ---1 00-- ----");
    u16 opcode2 = parse("0000 1000 00-- ----");

    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // Dx,<ea>        X       X   X   X   X   X   X   X   X   X
    // #<data>,<ea>   X       X   X   X   X   X   X   X   X   X

    ____XXX___MMMXXX(opcode1, BTST, 0b101111111110, Long, BitDxEa);
    __________MMMXXX(opcode2, BTST, 0b101111111110, Long, BitImEa);
}

void
CPU::registerCLR()
{
    registerClr<CLR>("0100 0010 ---- ----");   // <ea>
}

void
CPU::registerCMP()
{
    u16 opcode = parse("1011 ---0 ---- ----");

    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // <ea>,Dn        X  (X)  X   X   X   X   X   X   X   X   X   X

    ____XXX_SSMMMXXX(opcode, CMP, 0b101111111111, Byte,        Cmp);
    ____XXX_SSMMMXXX(opcode, CMP, 0b111111111111, Word | Long, Cmp);
}

void
CPU::registerCMPA()
{
    u16 opcode = parse("1011 ---- 11-- ----");

    // Modes:       <ea>,An
    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // <ea>,An        X   X   X   X   X   X   X   X   X   X   X   X

    ____XXXS__MMMXXX(opcode, CMPA, 0b111111111111, Word | Long, Cmpa);
}

void
CPU::registerDBcc()
{
    u32 opcode = parse("0101 ---- 1100 1---");

    for (int reg = 0; reg < 8; reg++) {

        register(opcode | 0x0 << 8 | reg, Dbcc<DBT  __ 0 __ Word>);
        register(opcode | 0x1 << 8 | reg, Dbcc<DBF  __ 0 __ Word>);
        register(opcode | 0x2 << 8 | reg, Dbcc<DBHI __ 0 __ Word>);
        register(opcode | 0x3 << 8 | reg, Dbcc<DBLS __ 0 __ Word>);
        register(opcode | 0x4 << 8 | reg, Dbcc<DBCC __ 0 __ Word>);
        register(opcode | 0x5 << 8 | reg, Dbcc<DBCS __ 0 __ Word>);
        register(opcode | 0x6 << 8 | reg, Dbcc<DBNE __ 0 __ Word>);
        register(opcode | 0x7 << 8 | reg, Dbcc<DBEQ __ 0 __ Word>);
        register(opcode | 0x8 << 8 | reg, Dbcc<DBVC __ 0 __ Word>);
        register(opcode | 0x9 << 8 | reg, Dbcc<DBVS __ 0 __ Word>);
        register(opcode | 0xA << 8 | reg, Dbcc<DBPL __ 0 __ Word>);
        register(opcode | 0xB << 8 | reg, Dbcc<DBMI __ 0 __ Word>);
        register(opcode | 0xC << 8 | reg, Dbcc<DBGE __ 0 __ Word>);
        register(opcode | 0xD << 8 | reg, Dbcc<DBLT __ 0 __ Word>);
        register(opcode | 0xE << 8 | reg, Dbcc<DBGT __ 0 __ Word>);
        register(opcode | 0xF << 8 | reg, Dbcc<DBLE __ 0 __ Word>);
    }
}

void
CPU::registerDIVS()
{
    registerMulDiv<DIVS>("1000 ---1 11-- ----");
}

void
CPU::registerDIVU()
{
    registerMulDiv<DIVU>("1000 ---0 11-- ----");
}

void
CPU::registerEOR()
{
    u16 opcode = parse("1011 ---1 ---- ----");

    // EOR
    //            -------------------------------------------------
    //            | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //            -------------------------------------------------
    // Dx,<ea>      X       X   X   X   X   X   X   X

    ____XXX_SSMMMXXX(opcode, EOR, 0b101111111000, Byte | Word | Long, AndRgEa);
}

void
CPU::registerEXT()
{
    u32 opcode = parse("0100 1000 --00 0---");

    for (int reg = 0; reg < 8; reg++) {
        register(opcode | 2 << 6 | reg, Ext<EXT __ 0 __ Word>);
        register(opcode | 3 << 6 | reg, Ext<EXT __ 0 __ Long>);
    }
}

void
CPU::registerLEA()
{
    u16 opcode;

    // LEA
    //
    // Modes:       LEA <ea>,An
    //
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                        X           X   X   X   X   X   X

    for (int an = 0; an < 8; an++) {

        opcode = parse("0100 ---1 11-- ----") | an << 9;
        for (int reg = 0; reg < 8; reg++) {
            register(opcode | 2 << 3 | reg, Lea<LEA __ 2 __ Long>);
            register(opcode | 5 << 3 | reg, Lea<LEA __ 5 __ Long>);
            register(opcode | 6 << 3 | reg, Lea<LEA __ 6 __ Long>);
        }
        register(opcode | 7 << 3 | 0, Lea<LEA __  7 __ Long>);
        register(opcode | 7 << 3 | 1, Lea<LEA __  8 __ Long>);
        register(opcode | 7 << 3 | 2, Lea<LEA __  9 __ Long>);
        register(opcode | 7 << 3 | 3, Lea<LEA __ 10 __ Long>);
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
CPU::registerMOVEA()
{
    // MOVEA
    //
    // Modes:       <ea>,An
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X   X   X   X   X   X   X   X   X   X   X   X

    u16 opcode = parse("00-- ---0 01-- ----");
    u16 opcodeW = opcode | 3 << 12;
    u16 opcodeL = opcode | 2 << 12;

    for (int dst = 0; dst < 8; dst++) {

        for (int src = 0; src < 8; src++) {

            register(opcodeW | dst << 9 | 0 << 3 | src, Movea<MOVEA __ 0 __ Word>);
            register(opcodeW | dst << 9 | 1 << 3 | src, Movea<MOVEA __ 1 __ Word>);
            register(opcodeW | dst << 9 | 2 << 3 | src, Movea<MOVEA __ 2 __ Word>);
            register(opcodeW | dst << 9 | 3 << 3 | src, Movea<MOVEA __ 3 __ Word>);
            register(opcodeW | dst << 9 | 4 << 3 | src, Movea<MOVEA __ 4 __ Word>);
            register(opcodeW | dst << 9 | 5 << 3 | src, Movea<MOVEA __ 5 __ Word>);
            register(opcodeW | dst << 9 | 6 << 3 | src, Movea<MOVEA __ 6 __ Word>);

            register(opcodeL | dst << 9 | 0 << 3 | src, Movea<MOVEA __ 0 __ Long>);
            register(opcodeL | dst << 9 | 1 << 3 | src, Movea<MOVEA __ 1 __ Long>);
            register(opcodeL | dst << 9 | 2 << 3 | src, Movea<MOVEA __ 2 __ Long>);
            register(opcodeL | dst << 9 | 3 << 3 | src, Movea<MOVEA __ 3 __ Long>);
            register(opcodeL | dst << 9 | 4 << 3 | src, Movea<MOVEA __ 4 __ Long>);
            register(opcodeL | dst << 9 | 5 << 3 | src, Movea<MOVEA __ 5 __ Long>);
            register(opcodeL | dst << 9 | 6 << 3 | src, Movea<MOVEA __ 6 __ Long>);

        }
        register(opcodeW | dst << 9 | 7 << 3 | 0, Movea<MOVEA __  7 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 1, Movea<MOVEA __  8 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 2, Movea<MOVEA __  9 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 3, Movea<MOVEA __ 10 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 4, Movea<MOVEA __ 11 __ Word>);

        register(opcodeL | dst << 9 | 7 << 3 | 0, Movea<MOVEA __  7 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 1, Movea<MOVEA __  8 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 2, Movea<MOVEA __  9 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 3, Movea<MOVEA __ 10 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 4, Movea<MOVEA __ 11 __ Long>);
    }
}

void
CPU::registerMOVEQ()
{
    u16 opcode = parse("0111 ---0 ---- ----");

    for (int dst = 0; dst < 8; dst++) {
        for (int src = 0; src < 256; src++) {
            register(opcode | dst << 9 | src, Moveq<MOVEQ __ 11 __ Long>);
        }
    }
}

void
CPU::registerMULS()
{
    registerMulDiv<MULS>("1100 ---1 11-- ----");
}

void
CPU::registerMULU()
{
    registerMulDiv<MULU>("1100 ---0 11-- ----");
}

void
CPU::registerNBCD()
{
    u32 opcode = parse("0100 1000 00-- ----");

    // NBCD
    //
    // Modes:       <ea>
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X

    for (int reg = 0; reg < 8; reg++) {

        register(opcode | 0 << 6 | 0 << 3 | reg, Nbcd<NBCD __ 0 __ Byte>);
        register(opcode | 0 << 6 | 2 << 3 | reg, Nbcd<NBCD __ 2 __ Byte>);
        register(opcode | 0 << 6 | 3 << 3 | reg, Nbcd<NBCD __ 3 __ Byte>);
        register(opcode | 0 << 6 | 4 << 3 | reg, Nbcd<NBCD __ 4 __ Byte>);
        register(opcode | 0 << 6 | 5 << 3 | reg, Nbcd<NBCD __ 5 __ Byte>);
        register(opcode | 0 << 6 | 6 << 3 | reg, Nbcd<NBCD __ 6 __ Byte>);
    }
    register(opcode | 0 << 6 | 7 << 3 | 0, Nbcd<NBCD __ 7 __ Byte>);
    register(opcode | 0 << 6 | 7 << 3 | 1, Nbcd<NBCD __ 8 __ Byte>);
}

void
CPU::registerNEG()
{
    registerNegNot<NEG>("0100 0100 ---- ----");
}

void
CPU::registerNEGX()
{
    registerNegNot<NEGX>("0100 0000 ---- ----");
}

void
CPU::registerNOT()
{
    registerNegNot<NOT>("0100 0110 ---- ----");
}

void
CPU::registerNOP()
{
    register(parse("0100 1110 0111 0001"), Nop<NOP __ 0 __ Long>);
}

void
CPU::registerOR()
{
    u16 opcode1 = parse("1000 ---0 ---- ----");
    u16 opcode2 = parse("1000 ---1 ---- ----");

    // OR
    //            -------------------------------------------------
    //            | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //            -------------------------------------------------
    // <ea>,Dy      X       X   X   X   X   X   X   X   X   X   X
    // Dx,<ea>              X   X   X   X   X   X   X

    ____XXX_SSMMMXXX(opcode1, OR, 0b101111111111, Byte | Word | Long, AndEaRg);
    ____XXX_SSMMMXXX(opcode2, OR, 0b001111111000, Byte | Word | Long, AndRgEa);
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
CPU::registerSBCD()
{
    registerAbcdSbcd<SBCD>("1000 ---1 0000 0---",  // Dx,Dy
                           "1000 ---1 0000 1---"); // -(Ax),-(Ay)
}

void
CPU::registerScc()
{
    registerS<CT, ST>();
    registerS<CF, SF>();
    registerS<HI, SHI>();
    registerS<LS, SLS>();
    registerS<CC, SCC>();
    registerS<CS, SCS>();
    registerS<NE, SNE>();
    registerS<EQ, SEQ>();
    registerS<VC, SVC>();
    registerS<VS, SVS>();
    registerS<PL, SPL>();
    registerS<MI, SMI>();
    registerS<GE, SGE>();
    registerS<LT, SLT>();
    registerS<GT, SGT>();
    registerS<LE, SLE>();
}

template <Cond CC, Instr I> void
CPU::registerS()
{
    // Scc
    //
    // Modes:       <ea>
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X

    u32 opcode = parse("0101 ---- 11-- ----") | CC << 8;

    for (int reg = 0; reg < 8; reg++) {

        register(opcode | 0 << 3 | reg, SccDn<I __ 0 __ Byte>);
        register(opcode | 2 << 3 | reg, SccEa<I __ 2 __ Byte>);
        register(opcode | 3 << 3 | reg, SccEa<I __ 3 __ Byte>);
        register(opcode | 4 << 3 | reg, SccEa<I __ 4 __ Byte>);
        register(opcode | 5 << 3 | reg, SccEa<I __ 5 __ Byte>);
        register(opcode | 6 << 3 | reg, SccEa<I __ 6 __ Byte>);
    }
    register(opcode | 7 << 3 | 0, SccEa<I __ 7 __ Byte>);
    register(opcode | 7 << 3 | 1, SccEa<I __ 8 __ Byte>);
}

void
CPU::registerSUB()
{
    registerAddSubEaRg<SUB>("1001 ---0 ---- ----"); // <ea>,Dy
    registerAddSubRgEa<SUB>("1001 ---1 ---- ----"); // Dx,<ea>
}

void
CPU::registerSUBA()
{
    registerAddSubA<SUBA>("1001 ---- 11-- ----");  // <ea>,Ay
}

void
CPU::registerTAS()
{
    // TAS
    //
    // Modes:       <ea>
    //
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X   X   X

    u16 opcode = parse("0100 1010 11-- ----");

    for (int reg = 0; reg < 8; reg++) {

        register(opcode | 0 << 6 | 0 << 3 | reg, TasDn<TAS __ 0 __ Byte>);
        register(opcode | 0 << 6 | 2 << 3 | reg, TasEa<TAS __ 2 __ Byte>);
        register(opcode | 0 << 6 | 3 << 3 | reg, TasEa<TAS __ 3 __ Byte>);
        register(opcode | 0 << 6 | 4 << 3 | reg, TasEa<TAS __ 4 __ Byte>);
        register(opcode | 0 << 6 | 5 << 3 | reg, TasEa<TAS __ 5 __ Byte>);
        register(opcode | 0 << 6 | 6 << 3 | reg, TasEa<TAS __ 6 __ Byte>);

    }
    register(opcode | 0 << 6 | 7 << 3 | 0, TasEa<TAS __ 7 __ Byte>);
    register(opcode | 0 << 6 | 7 << 3 | 1, TasEa<TAS __ 8 __ Byte>);
}

void
CPU::registerTST()
{
    // TST
    //
    // Modes:       TST <ea>
    //
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X   X   X

    u16 opcode = parse("0100 1010 ---- ----");
    u16 opcodeB = opcode | 0 << 6;
    u16 opcodeW = opcode | 1 << 6;
    u16 opcodeL = opcode | 2 << 6;

    for (int reg = 0; reg < 8; reg++) {

        register(opcodeB | 0 << 3 | reg, Tst<TST __ 0 __ Byte>);
        register(opcodeB | 2 << 3 | reg, Tst<TST __ 2 __ Byte>);
        register(opcodeB | 3 << 3 | reg, Tst<TST __ 3 __ Byte>);
        register(opcodeB | 4 << 3 | reg, Tst<TST __ 4 __ Byte>);
        register(opcodeB | 5 << 3 | reg, Tst<TST __ 5 __ Byte>);
        register(opcodeB | 6 << 3 | reg, Tst<TST __ 6 __ Byte>);

        register(opcodeW | 0 << 3 | reg, Tst<TST __ 0 __ Word>);
        register(opcodeW | 2 << 3 | reg, Tst<TST __ 2 __ Word>);
        register(opcodeW | 3 << 3 | reg, Tst<TST __ 3 __ Word>);
        register(opcodeW | 4 << 3 | reg, Tst<TST __ 4 __ Word>);
        register(opcodeW | 5 << 3 | reg, Tst<TST __ 5 __ Word>);
        register(opcodeW | 6 << 3 | reg, Tst<TST __ 6 __ Word>);

        register(opcodeL | 0 << 3 | reg, Tst<TST __ 0 __ Long>);
        register(opcodeL | 2 << 3 | reg, Tst<TST __ 2 __ Long>);
        register(opcodeL | 3 << 3 | reg, Tst<TST __ 3 __ Long>);
        register(opcodeL | 4 << 3 | reg, Tst<TST __ 4 __ Long>);
        register(opcodeL | 5 << 3 | reg, Tst<TST __ 5 __ Long>);
        register(opcodeL | 6 << 3 | reg, Tst<TST __ 6 __ Long>);
    }
    register(opcodeB | 7 << 3 | 0, Tst<TST __  7 __ Byte>);
    register(opcodeB | 7 << 3 | 1, Tst<TST __  8 __ Byte>);
    register(opcodeB | 7 << 3 | 2, Tst<TST __  9 __ Byte>);
    register(opcodeB | 7 << 3 | 3, Tst<TST __ 10 __ Byte>);

    register(opcodeW | 7 << 3 | 0, Tst<TST __  7 __ Word>);
    register(opcodeW | 7 << 3 | 1, Tst<TST __  8 __ Word>);
    register(opcodeW | 7 << 3 | 2, Tst<TST __  9 __ Word>);
    register(opcodeW | 7 << 3 | 3, Tst<TST __ 10 __ Word>);

    register(opcodeL | 7 << 3 | 0, Tst<TST __  7 __ Long>);
    register(opcodeL | 7 << 3 | 1, Tst<TST __  8 __ Long>);
    register(opcodeL | 7 << 3 | 2, Tst<TST __  9 __ Long>);
    register(opcodeL | 7 << 3 | 3, Tst<TST __ 10 __ Long>);
}

#undef __
