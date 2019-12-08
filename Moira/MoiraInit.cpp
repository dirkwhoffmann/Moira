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
assert(exec[id] == &CPU::execIllegal); \
assert(dasm[id] == &CPU::dasmIllegal); \
exec[id] = &CPU::exec##name; \
dasm[id] = &CPU::dasm##name; \
}

// Registers an instruction in standard format with two parameterized operands:
//
//                                  222 = Operand 2
//       FEDC BA98 7654 3210         ss = Size (Byte, Word, Long)
//       ____ 222_ ssmm m111        mmm = Addressing mode
//                                  111 = Operand 1

#define REGISTER(op,I,s,m,f) \
if ((s) & 0b001) { REG((op) | 0 << 6, I, Byte, m, f) } \
if ((s) & 0b010) { REG((op) | 1 << 6, I, Word, m, f) } \
if ((s) & 0b100) { REG((op) | 2 << 6, I, Long, m, f) }

#define REG(op,I,S,m,f) \
for (int i = 0; i < 8; i++) { \
for (int j = 0; j < 8; j++) { \
if ((m) & 0b100000000000) register(op | i << 9 | 0 << 3 | j, f<I __  0 __ S>); \
if ((m) & 0b010000000000) register(op | i << 9 | 1 << 3 | j, f<I __  1 __ S>); \
if ((m) & 0b001000000000) register(op | i << 9 | 2 << 3 | j, f<I __  2 __ S>); \
if ((m) & 0b000100000000) register(op | i << 9 | 3 << 3 | j, f<I __  3 __ S>); \
if ((m) & 0b000010000000) register(op | i << 9 | 4 << 3 | j, f<I __  4 __ S>); \
if ((m) & 0b000001000000) register(op | i << 9 | 5 << 3 | j, f<I __  5 __ S>); \
if ((m) & 0b000000100000) register(op | i << 9 | 6 << 3 | j, f<I __  6 __ S>); \
} \
if ((m) & 0b000000010000) register(op | i << 9 | 7 << 3 | 0, f<I __  7 __ S>); \
if ((m) & 0b000000001000) register(op | i << 9 | 7 << 3 | 1, f<I __  8 __ S>); \
if ((m) & 0b000000000100) register(op | i << 9 | 7 << 3 | 2, f<I __  9 __ S>); \
if ((m) & 0b000000000010) register(op | i << 9 | 7 << 3 | 3, f<I __ 10 __ S>); \
if ((m) & 0b000000000001) register(op | i << 9 | 7 << 3 | 4, f<I __ 11 __ S>); \
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
    registerABCD();
    registerADD();
    registerADDA();
    registerAND();
    registerASL();
    registerASR();
    registerBCHG();
    registerBCLR();
    registerBSET();
    registerBTST();
    registerCLR();
    registerCMP();
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

    // ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR
    //
    // Modes: (1)   Dx,Dy       8,16,32
    //        (2)   #<data>,Dy  8,16,32
    //        (3)   <ea>          16
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X?? X??         X?!

    // (1)
    for (int dx = 0; dx < 8; dx++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse(patternReg) | dx << 9 | dy;
            register(opcode | 0 << 6, Shift<I __ 0 __ Byte>);
            register(opcode | 1 << 6, Shift<I __ 0 __ Word>);
            register(opcode | 2 << 6, Shift<I __ 0 __ Long>);
        }
    }

    // (2)
    for (int data = 0; data < 8; data++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse(patternImm) | data << 9 | dy;
            register(opcode | 0 << 6, Shift<I __ 11 __ Byte>);
            register(opcode | 1 << 6, Shift<I __ 11 __ Word>);
            register(opcode | 2 << 6, Shift<I __ 11 __ Long>);
        }
    }

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
CPU::registerBit(const char *patternReg, const char *patternImm)
{
    // BCHG, BSET, BCLR
    //
    // Modes: (1)   Dx,<ea>
    //        (2)   #<data>,<ea>
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X
    //
    // BTST
    //
    // Modes: (1)   Dx,<ea>
    //        (2)   #<data>,<ea>
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X   X   X

    // (1)
    u16 opcode = parse(patternReg);

    for (int src = 0; src < 8; src++) {
        for (int dst = 0; dst < 8; dst++) {

            register(opcode | src << 9 | 0 << 3 | dst, BitDxDy<I __ 0>);
            register(opcode | src << 9 | 2 << 3 | dst, BitDxEa<I __ 2>);
            register(opcode | src << 9 | 3 << 3 | dst, BitDxEa<I __ 3>);
            register(opcode | src << 9 | 4 << 3 | dst, BitDxEa<I __ 4>);
            register(opcode | src << 9 | 5 << 3 | dst, BitDxEa<I __ 5>);
            register(opcode | src << 9 | 6 << 3 | dst, BitDxEa<I __ 6>);
        }
        register(opcode | src << 9 | 7 << 3 | 0, BitDxEa<I __ 7>);
        register(opcode | src << 9 | 7 << 3 | 1, BitDxEa<I __ 8>);
        if (I == BTST) {
            register(opcode | src << 9 | 7 << 3 | 2, BitDxEa<I __  9>);
            register(opcode | src << 9 | 7 << 3 | 3, BitDxEa<I __ 10>);
        }
    }

    // (2)
    opcode = parse(patternImm);

    for (int dst = 0; dst < 8; dst++) {

        register(opcode | 0 << 3 | dst, BitImDy<I __ 0>);
        register(opcode | 2 << 3 | dst, BitImEa<I __ 2>);
        register(opcode | 3 << 3 | dst, BitImEa<I __ 3>);
        register(opcode | 4 << 3 | dst, BitImEa<I __ 4>);
        register(opcode | 5 << 3 | dst, BitImEa<I __ 5>);
        register(opcode | 6 << 3 | dst, BitImEa<I __ 6>);
    }
    register(opcode | 7 << 3 | 0, BitImEa<I __ 7>);
    register(opcode | 7 << 3 | 1, BitImEa<I __ 8>);
    if (I == BTST) {
        register(opcode | 7 << 3 | 2, BitImEa<I __  9>);
        register(opcode | 7 << 3 | 3, BitImEa<I __ 10>);
    }
}

template<Instr I> void
CPU::registerLogicXXReg(const char *pattern)
{
    // Modes: (1)   <ea>,Dy
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X       X   X   X   X   X   X   X   X   X   X

    u16 opcode  = parse(pattern);
    REGISTER(opcode, I, Byte | Word | Long, 0b101111111111, AndEaRg);
}

template<Instr I> void
CPU::registerLogicRegXX(const char *pattern, bool mode0)
{
    // Modes:       Dx,<ea>
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //               (X)      X   X   X   X   X   X   X

    u16 opcode  = parse(pattern);

    if (mode0) {
        REGISTER(opcode, I, Byte | Word | Long, 0b101111111000, AndRgEa);
    } else {
        REGISTER(opcode, I, Byte | Word | Long, 0b001111111000, AndRgEa);
    }
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
            register(opcode | dst << 9 | src, Abcd<I __ 0>);
        }
    }

    // (2)
    opcode = parse(patternInd);
    for (int src = 0; src < 8; src++) {
        for (int dst = 0; dst < 8; dst++) {
            register(opcode | dst << 9 | src, Abcd<I __ 4>);
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

    REGISTER(opcode1, AND, Byte | Word | Long, 0b101111111111, AndEaRg);
    REGISTER(opcode2, AND, Byte | Word | Long, 0b001111111000, AndRgEa);
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
    registerBit<BCHG>("0000 ---1 01-- ----",   // Dx,<ea>
                      "0000 1000 01-- ----");  // ##,<ea>
}

void
CPU::registerBCLR()
{
    registerBit<BCLR>("0000 ---1 10-- ----",   // Dx,<ea>
                      "0000 1000 10-- ----");  // ##,<ea>
}

void
CPU::registerBSET()
{
    registerBit<BSET>("0000 ---1 11-- ----",   // Dx,<ea>
                      "0000 1000 11-- ----");  // ##,<ea>
}

void
CPU::registerBTST()
{
    registerBit<BTST>("0000 ---1 00-- ----",   // Dx,<ea>
                      "0000 1000 00-- ----");  // ##,<ea>
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

    // CMP
    //
    // Modes:       <ea>,Dn
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    //                X  (X)  X   X   X   X   X   X   X   X   X   X

    REGISTER(opcode, CMP, Byte,        0b101111111111, Cmp);
    REGISTER(opcode, CMP, Word | Long, 0b111111111111, Cmp);
}

void
CPU::registerDBcc()
{
    u32 opcode = parse("0101 ---- 1100 1---");

    for (int reg = 0; reg < 8; reg++) {

        register(opcode | 0x0 << 8 | reg, Dbcc<CT>);
        register(opcode | 0x1 << 8 | reg, Dbcc<CF>);
        register(opcode | 0x2 << 8 | reg, Dbcc<HI>);
        register(opcode | 0x3 << 8 | reg, Dbcc<LS>);
        register(opcode | 0x4 << 8 | reg, Dbcc<CC>);
        register(opcode | 0x5 << 8 | reg, Dbcc<CS>);
        register(opcode | 0x6 << 8 | reg, Dbcc<NE>);
        register(opcode | 0x7 << 8 | reg, Dbcc<EQ>);
        register(opcode | 0x8 << 8 | reg, Dbcc<VC>);
        register(opcode | 0x9 << 8 | reg, Dbcc<VS>);
        register(opcode | 0xA << 8 | reg, Dbcc<PL>);
        register(opcode | 0xB << 8 | reg, Dbcc<MI>);
        register(opcode | 0xC << 8 | reg, Dbcc<GE>);
        register(opcode | 0xD << 8 | reg, Dbcc<LT>);
        register(opcode | 0xE << 8 | reg, Dbcc<GT>);
        register(opcode | 0xF << 8 | reg, Dbcc<LE>);
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
    registerLogicRegXX<EOR>("1011 ---1 ---- ----", true);
}

void
CPU::registerEXT()
{
    u32 opcode = parse("0100 1000 --00 0---");

    for (int reg = 0; reg < 8; reg++) {
        register(opcode | 2 << 6 | reg, Ext<Word>);
        register(opcode | 3 << 6 | reg, Ext<Long>);
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
            register(opcode | 2 << 3 | reg, Lea<2>);
            register(opcode | 5 << 3 | reg, Lea<5>);
            register(opcode | 6 << 3 | reg, Lea<6>);
        }
        register(opcode | 7 << 3 | 0, Lea<7>);
        register(opcode | 7 << 3 | 1, Lea<8>);
        register(opcode | 7 << 3 | 2, Lea<9>);
        register(opcode | 7 << 3 | 3, Lea<10>);
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

            register(opcodeW | dst << 9 | 0 << 3 | src, Movea<0 __ Word>);
            register(opcodeW | dst << 9 | 1 << 3 | src, Movea<1 __ Word>);
            register(opcodeW | dst << 9 | 2 << 3 | src, Movea<2 __ Word>);
            register(opcodeW | dst << 9 | 3 << 3 | src, Movea<3 __ Word>);
            register(opcodeW | dst << 9 | 4 << 3 | src, Movea<4 __ Word>);
            register(opcodeW | dst << 9 | 5 << 3 | src, Movea<5 __ Word>);
            register(opcodeW | dst << 9 | 6 << 3 | src, Movea<6 __ Word>);

            register(opcodeL | dst << 9 | 0 << 3 | src, Movea<0 __ Long>);
            register(opcodeL | dst << 9 | 1 << 3 | src, Movea<1 __ Long>);
            register(opcodeL | dst << 9 | 2 << 3 | src, Movea<2 __ Long>);
            register(opcodeL | dst << 9 | 3 << 3 | src, Movea<3 __ Long>);
            register(opcodeL | dst << 9 | 4 << 3 | src, Movea<4 __ Long>);
            register(opcodeL | dst << 9 | 5 << 3 | src, Movea<5 __ Long>);
            register(opcodeL | dst << 9 | 6 << 3 | src, Movea<6 __ Long>);

        }
        register(opcodeW | dst << 9 | 7 << 3 | 0, Movea< 7 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 1, Movea< 8 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 2, Movea< 9 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 3, Movea<10 __ Word>);
        register(opcodeW | dst << 9 | 7 << 3 | 4, Movea<11 __ Word>);

        register(opcodeL | dst << 9 | 7 << 3 | 0, Movea< 7 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 1, Movea< 8 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 2, Movea< 9 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 3, Movea<10 __ Long>);
        register(opcodeL | dst << 9 | 7 << 3 | 4, Movea<11 __ Long>);
    }
}

void
CPU::registerMOVEQ()
{
    u16 opcode = parse("0111 ---0 ---- ----");

    for (int dst = 0; dst < 8; dst++) {
        for (int src = 0; src < 256; src++) {
            register(opcode | dst << 9 | src, Moveq);
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

        register(opcode | 0 << 6 | 0 << 3 | reg, Nbcd<0>);
        register(opcode | 0 << 6 | 2 << 3 | reg, Nbcd<2>);
        register(opcode | 0 << 6 | 3 << 3 | reg, Nbcd<3>);
        register(opcode | 0 << 6 | 4 << 3 | reg, Nbcd<4>);
        register(opcode | 0 << 6 | 5 << 3 | reg, Nbcd<5>);
        register(opcode | 0 << 6 | 6 << 3 | reg, Nbcd<6>);
    }
    register(opcode | 0 << 6 | 7 << 3 | 0, Nbcd<7>);
    register(opcode | 0 << 6 | 7 << 3 | 1, Nbcd<8>);
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
    register(parse("0100 1110 0111 0001"), Nop);
}

void
CPU::registerOR()
{
    registerLogicXXReg<OR>("1000 ---0 ---- ----"); // <ea>,Dy
    registerLogicRegXX<OR>("1000 ---1 ---- ----"); // Dx,<ea>
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
    registerS<CT>();
    registerS<CF>();
    registerS<HI>();
    registerS<LS>();
    registerS<CC>();
    registerS<CS>();
    registerS<NE>();
    registerS<EQ>();
    registerS<VC>();
    registerS<VS>();
    registerS<PL>();
    registerS<MI>();
    registerS<GE>();
    registerS<LT>();
    registerS<GT>();
    registerS<LE>();
}

template <Cond CC> void
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

        register(opcode | 0 << 3 | reg, SccDn<CC __ 0>);
        register(opcode | 2 << 3 | reg, SccEa<CC __ 2>);
        register(opcode | 3 << 3 | reg, SccEa<CC __ 3>);
        register(opcode | 4 << 3 | reg, SccEa<CC __ 4>);
        register(opcode | 5 << 3 | reg, SccEa<CC __ 5>);
        register(opcode | 6 << 3 | reg, SccEa<CC __ 6>);
    }
    register(opcode | 7 << 3 | 0, SccEa<CC __ 7>);
    register(opcode | 7 << 3 | 1, SccEa<CC __ 8>);
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

        register(opcode | 0 << 6 | 0 << 3 | reg, TasDn<0>);
        register(opcode | 0 << 6 | 2 << 3 | reg, TasEa<2>);
        register(opcode | 0 << 6 | 3 << 3 | reg, TasEa<3>);
        register(opcode | 0 << 6 | 4 << 3 | reg, TasEa<4>);
        register(opcode | 0 << 6 | 5 << 3 | reg, TasEa<5>);
        register(opcode | 0 << 6 | 6 << 3 | reg, TasEa<6>);

    }
    register(opcode | 0 << 6 | 7 << 3 | 0, TasEa<7>);
    register(opcode | 0 << 6 | 7 << 3 | 1, TasEa<8>);
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

        register(opcodeB | 0 << 3 | reg, Tst<0 __ Byte>);
        register(opcodeB | 2 << 3 | reg, Tst<2 __ Byte>);
        register(opcodeB | 3 << 3 | reg, Tst<3 __ Byte>);
        register(opcodeB | 4 << 3 | reg, Tst<4 __ Byte>);
        register(opcodeB | 5 << 3 | reg, Tst<5 __ Byte>);
        register(opcodeB | 6 << 3 | reg, Tst<6 __ Byte>);

        register(opcodeW | 0 << 3 | reg, Tst<0 __ Word>);
        register(opcodeW | 2 << 3 | reg, Tst<2 __ Word>);
        register(opcodeW | 3 << 3 | reg, Tst<3 __ Word>);
        register(opcodeW | 4 << 3 | reg, Tst<4 __ Word>);
        register(opcodeW | 5 << 3 | reg, Tst<5 __ Word>);
        register(opcodeW | 6 << 3 | reg, Tst<6 __ Word>);

        register(opcodeL | 0 << 3 | reg, Tst<0 __ Long>);
        register(opcodeL | 2 << 3 | reg, Tst<2 __ Long>);
        register(opcodeL | 3 << 3 | reg, Tst<3 __ Long>);
        register(opcodeL | 4 << 3 | reg, Tst<4 __ Long>);
        register(opcodeL | 5 << 3 | reg, Tst<5 __ Long>);
        register(opcodeL | 6 << 3 | reg, Tst<6 __ Long>);
    }
    register(opcodeB | 7 << 3 | 0, Tst< 7 __ Byte>);
    register(opcodeB | 7 << 3 | 1, Tst< 8 __ Byte>);
    register(opcodeB | 7 << 3 | 2, Tst< 9 __ Byte>);
    register(opcodeB | 7 << 3 | 3, Tst<10 __ Byte>);

    register(opcodeW | 7 << 3 | 0, Tst< 7 __ Word>);
    register(opcodeW | 7 << 3 | 1, Tst< 8 __ Word>);
    register(opcodeW | 7 << 3 | 2, Tst< 9 __ Word>);
    register(opcodeW | 7 << 3 | 3, Tst<10 __ Word>);

    register(opcodeL | 7 << 3 | 0, Tst< 7 __ Long>);
    register(opcodeL | 7 << 3 | 1, Tst< 8 __ Long>);
    register(opcodeL | 7 << 3 | 2, Tst< 9 __ Long>);
    register(opcodeL | 7 << 3 | 3, Tst<10 __ Long>);
}

#undef __
