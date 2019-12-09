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
//                ____ ____ XXXX XXXX
//                ____ ____ SS__ _XXX
//                ____ XXX_ ____ _XXX
//                ____ XXX_ XXXX XXXX
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

#define ________XXXXXXXX(op,I,M,S,f) { \
for (int j = 0; j < 256; j++) register((op) | j, f<I __ M __ S>); }

#define ________SS___XXX(op,I,M,s,f) { \
if ((s) & 0b100) _____________XXX((op) | 2 << 6, I, M, Long, f); \
if ((s) & 0b010) _____________XXX((op) | 1 << 6, I, M, Word, f); \
if ((s) & 0b001) _____________XXX((op) | 0 << 6, I, M, Byte, f); }

#define ____XXX______XXX(op,I,M,S,f) { \
for (int i = 0; i < 8; i++) _____________XXX((op) | i << 9, I, M, S, f); }

#define ____XXX_XXXXXXXX(op,I,M,S,f) { \
for (int i = 0; i < 8; i++) ________XXXXXXXX((op) | i << 9, I, M, S, f); }

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
    createJumpTable();

    // Register the instruction set (DEPRECATED)
    registerSBCD();
    registerScc();
    registerSUB();
    registerSUBA();
    registerTAS();
    registerTST();
}

template<Instr I> void
CPU::registerShift(const char *patternReg,
                   const char *patternImm,
                   const char *patternEa)
{
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

    ____XXX_SS___XXX(opcode1, I, 0b0, Byte | Word | Long, ShiftRg);
    ____XXX_SS___XXX(opcode2, I, 0xB, Byte | Word | Long, ShiftIm);
    __________MMMXXX(opcode3, I, 0b001111111000, Word, Shift);
}

void
CPU::createJumpTable()
{
    // Start with clean tables
    for (int i = 0; i < 0x10000; i++) {
        exec[i] = &CPU::execIllegal;
        dasm[i] = &CPU::dasmIllegal;
        sync[i] = &CPU::syncIllegal;
    }

    // Unimplemented instructions are identified by the following bit patterns:
    //
    //    1010 ---- ---- ---- : (Line A instructions)
    //    1111 ---- ---- ---- : (Line F instructions)
    //
    // Both instructions types are handles similarly. They only differ in the
    // associated vector number.

    for (int i = 0; i < 0x1000; i++) {

        exec[0b1010 << 12 | i] = &CPU::execLineA;
        dasm[0b1010 << 12 | i] = &CPU::dasmLineA;
        sync[0b1010 << 12 | i] = &CPU::syncLineA;

        exec[0b1111 << 12 | i] = &CPU::execLineF;
        dasm[0b1111 << 12 | i] = &CPU::dasmLineF;
        sync[0b1111 << 12 | i] = &CPU::syncLineF;
    }

    // Register all instructions
    registerInstructions();

}

void
CPU::registerInstructions()
{
    u16 opcode;

    // ABCD
    //
    //       Syntax: (1) ABCD Dy,Dx
    //               (2) ABCD -(Ay),-(Ax)
    //         Size: Byte

    // Dy,Dx
    opcode = parse("1100 ---1 0000 0---");
    ____XXX______XXX(opcode, ABCD, 0, Byte, Abcd);

    // -(Ay),-(Ax)
    opcode = parse("1100 ---1 0000 1---");
    ____XXX______XXX(opcode, ABCD, 4, Byte, Abcd);


    // ADD
    //
    //       Syntax: (1) ADD <ea>,Dy
    //               (2) ADD Dx,<ea>
    //         Size: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>,Dy       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X  (X)  X   X   X   X   X   X   X   X   X   X

    opcode = parse("1101 ---0 ---- ----");
    ____XXX_SSMMMXXX(opcode, ADD, 0b101111111111, Byte,        AddEaRg);
    ____XXX_SSMMMXXX(opcode, ADD, 0b111111111111, Word | Long, AddEaRg);

    //               -------------------------------------------------
    // Dx,<ea>       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X   X   X   X   X   X   X

     opcode = parse("1101 ---1 ---- ----");
     ____XXX_SSMMMXXX(opcode, ADD, 0b001111111000, Byte | Word | Long, AddRgEa);


    // ADDA
    //
    //       Syntax: ADD <ea>,Ay
    //         Size: Word, Longword
    //
    //               -------------------------------------------------
    // <ea>,Ay       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X   X   X   X   X   X   X   X   X   X   X   X

    opcode = parse("1101 ---- 11-- ----");
    ____XXXS__MMMXXX(opcode, ADDA, 0b111111111111, Word | Long, Adda)


    // AND
    //
    //       Syntax: (1) AND <ea>,Dy
    //               (2) AND Dx,<ea>
    //         Size: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>,Dy       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X

    opcode = parse("1100 ---0 ---- ----");
    ____XXX_SSMMMXXX(opcode, AND, 0b101111111111, Byte | Word | Long, AndEaRg);

    //               -------------------------------------------------
    // Dx,<ea>       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X   X   X   X   X   X   X

    opcode = parse("1100 ---1 ---- ----");
    ____XXX_SSMMMXXX(opcode, AND, 0b001111111000, Byte | Word | Long, AndRgEa);


    // ASL, ASR
    //
    //       Syntax: (1) ASx Dx,Dy
    //               (2) ASx #<data>,Dy
    //               (3) ASx <ea>
    //         Size: Byte, Word, Longword

    // Dx,Dy
    opcode = parse("1110 ---1 --10 0---");
    ____XXX_SS___XXX(opcode, ASL, 0,  Byte | Word | Long, ShiftRg);

    opcode = parse("1110 ---0 --10 0---");
    ____XXX_SS___XXX(opcode, ASR, 0,  Byte | Word | Long, ShiftRg);

    // #<data>,Dy
    opcode = parse("1110 ---1 --00 0---");
    ____XXX_SS___XXX(opcode, ASL, 11, Byte | Word | Long, ShiftIm);

    opcode = parse("1110 ---0 --00 0---");
    ____XXX_SS___XXX(opcode, ASR, 11, Byte | Word | Long, ShiftIm);

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X   X   X   X   X   X   X

    opcode = parse("1110 0001 11-- ----");
    __________MMMXXX(opcode, ASL, 0b001111111000, Word, Shift);

    opcode = parse("1110 0000 11-- ----");
    __________MMMXXX(opcode, ASR, 0b001111111000, Word, Shift);


    // BCHG, BCLR
    //
    //       Syntax: (1) BCxx Dn,<ea>
    //               (2) BCxx #<data>,<ea>
    //         Size: Byte, Longword

    //               -------------------------------------------------
    // Dx,<ea>       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 ---1 01-- ----");
    ____XXX___MMMXXX(opcode, BCHG, 0b101111111000, Long, BitDxEa);
    opcode = parse("0000 ---1 10-- ----");
    ____XXX___MMMXXX(opcode, BCLR, 0b101111111000, Long, BitDxEa);

    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 1000 01-- ----");
    __________MMMXXX(opcode, BCHG, 0b101111111000, Long, BitImEa);
    opcode = parse("0000 1000 10-- ----");
    __________MMMXXX(opcode, BCLR, 0b101111111000, Long, BitImEa);


    // BSET
    //
    //       Syntax: (1) BSET Dx,<ea>
    //               (2) BSET #<data>,<ea>
    //         Size: Byte, Longword

    //               -------------------------------------------------
    // Dx,<ea>       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 ---1 11-- ----");
    ____XXX___MMMXXX(opcode, BSET, 0b101111111000, Long, BitDxEa);

    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 1000 11-- ----");
    __________MMMXXX(opcode, BSET, 0b101111111000, Long, BitImEa);


    // BTST
    //
    //       Syntax: (1) BTST Dx,<ea>
    //               (2) BTST #<data>,<ea>
    //         Size: Byte, Longword

    //               -------------------------------------------------
    // Dx,<ea>       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X

    opcode = parse("0000 ---1 00-- ----");
    ____XXX___MMMXXX(opcode, BTST, 0b101111111110, Long, BitDxEa);

    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X

    opcode = parse("0000 1000 00-- ----");
    __________MMMXXX(opcode, BTST, 0b101111111110, Long, BitImEa);


    // CLR
    //
    //       Syntax: CLR <ea>
    //         Size: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0100 0010 ---- ----");
    ________SSMMMXXX(opcode, CLR, 0b101111111000, Byte | Word | Long, Clr);


    // CMP
    //
    //       Syntax: CMP <ea>,Dy
    //         Size: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>,Dy       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X  (X)  X   X   X   X   X   X   X   X   X   X

    opcode = parse("1011 ---0 ---- ----");
    ____XXX_SSMMMXXX(opcode, CMP, 0b101111111111, Byte,        Cmp);
    ____XXX_SSMMMXXX(opcode, CMP, 0b111111111111, Word | Long, Cmp);


    // CMPA
    //
    //       Syntax: CMPA <ea>,Ay
    //         Size: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>,Ay       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X   X   X   X   X   X   X   X   X   X   X   X

    opcode = parse("1011 ---- 11-- ----");
    ____XXXS__MMMXXX(opcode, CMPA, 0b111111111111, Word | Long, Cmpa);


    // DBcc
    //
    //       Syntax: DBcc Dn,<label>
    //         Size: Word

    // Dn,<label>
    opcode = parse("0101 ---- 1100 1---");
    _____________XXX(opcode | 0x000, DBT,  0, Word, Dbcc);
    _____________XXX(opcode | 0x100, DBF,  0, Word, Dbcc);
    _____________XXX(opcode | 0x200, DBHI, 0, Word, Dbcc);
    _____________XXX(opcode | 0x300, DBLS, 0, Word, Dbcc);
    _____________XXX(opcode | 0x400, DBCC, 0, Word, Dbcc);
    _____________XXX(opcode | 0x500, DBCS, 0, Word, Dbcc);
    _____________XXX(opcode | 0x600, DBNE, 0, Word, Dbcc);
    _____________XXX(opcode | 0x700, DBEQ, 0, Word, Dbcc);
    _____________XXX(opcode | 0x800, DBVC, 0, Word, Dbcc);
    _____________XXX(opcode | 0x900, DBVS, 0, Word, Dbcc);
    _____________XXX(opcode | 0xA00, DBPL, 0, Word, Dbcc);
    _____________XXX(opcode | 0xB00, DBMI, 0, Word, Dbcc);
    _____________XXX(opcode | 0xC00, DBGE, 0, Word, Dbcc);
    _____________XXX(opcode | 0xD00, DBLT, 0, Word, Dbcc);
    _____________XXX(opcode | 0xE00, DBGT, 0, Word, Dbcc);
    _____________XXX(opcode | 0xF00, DBLE, 0, Word, Dbcc);


    // DIVS, DIVU
    //
    //       Syntax: DIVx <ea>,Dy
    //        Sizes: Longword, Word -> Longword

    //               -------------------------------------------------
    // <ea>,Dn       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X

    opcode = parse("1000 ---1 11-- ----");
    ____XXX___MMMXXX(opcode, DIVS, 0b101111111111, Long, MulDiv);

    opcode = parse("1000 ---0 11-- ----");
    ____XXX___MMMXXX(opcode, DIVU, 0b101111111111, Long, MulDiv);


    // EOR
    //
    //       Syntax: EOR Dx,<ea>
    //        Sizes: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>,Dn       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("1011 ---1 ---- ----");
    ____XXX_SSMMMXXX(opcode, EOR, 0b101111111000, Byte | Word | Long, AndRgEa);


    // EXT
    //
    //       Syntax: EXT Dx
    //        Sizes: Word, Longword

    // Dx
    opcode = parse("0100 1000 --00 0---");
    _____________XXX(opcode | 2 << 6, EXT, 0, Word, Ext);
    _____________XXX(opcode | 3 << 6, EXT, 0, Long, Ext);


    // LEA
    //
    //       Syntax: LEA <ea>,Ay
    //        Sizes: Longword

    //               -------------------------------------------------
    // <ea>,Ay       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X           X   X   X   X   X   X

    opcode = parse("0100 ---1 11-- ----");
    ____XXX___MMMXXX(opcode, LEA, 0b001001111110, Long, Lea);


    // LSL, LSR
    //
    //       Syntax: (1) LSx Dx,Dy
    //               (2) LSx #<data>,Dy
    //               (3) LSx <ea>
    //        Sizes: Byte, Word, Longword

    // Dx,Dy
    opcode = parse("1110 ---1 --10 1---");
     ____XXX_SS___XXX(opcode, LSL, 0b0, Byte | Word | Long, ShiftRg);

    opcode = parse("1110 ---0 --10 1---");
     ____XXX_SS___XXX(opcode, LSR, 0b0, Byte | Word | Long, ShiftRg);

    // #<data>,Dy
    opcode = parse("1110 ---1 --00 1---");
    ____XXX_SS___XXX(opcode, LSL, 0xB, Byte | Word | Long, ShiftIm);

    opcode = parse("1110 ---0 --00 1---");
    ____XXX_SS___XXX(opcode, LSR, 0xB, Byte | Word | Long, ShiftIm);

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X   X   X   X   X   X   X

    opcode = parse("1110 0011 11-- ----");
     __________MMMXXX(opcode, LSL, 0b001111111000, Word, Shift);

    opcode = parse("1110 0010 11-- ----");
      __________MMMXXX(opcode, LSR, 0b001111111000, Word, Shift);


    // MOVEA
    //
    //       Syntax: MOVEA <ea>,Ay
    //        Sizes: Word, Longword

    //               -------------------------------------------------
    // <ea>,Ay       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X   X   X   X   X   X   X   X   X   X   X   X

    opcode = parse("001- ---0 01-- ----");

    ____XXX___MMMXXX(opcode | 0 << 12, MOVEA, 0b111111111111, Long, Movea)
    ____XXX___MMMXXX(opcode | 1 << 12, MOVEA, 0b111111111111, Word, Movea)


    // MOVEQ
    //
    //       Syntax: MOVEQ #<data>,Dn
    //        Sizes: Longword

    // #<data>,Dn
    opcode = parse("0111 ---0 ---- ----");
    ____XXX_XXXXXXXX(opcode, MOVEQ, 11, Long, Moveq);


    // MULS, MULU
    //
    //       Syntax: MULx <ea>,Dy
    //        Sizes: Word x Word -> Longword

    //               -------------------------------------------------
    // <ea>,Dy       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X

    opcode = parse("1100 ---1 11-- ----");
    ____XXX___MMMXXX(opcode, MULS, 0b101111111111, Long, MulDiv);

    opcode = parse("1100 ---0 11-- ----");
    ____XXX___MMMXXX(opcode, MULU, 0b101111111111, Long, MulDiv);


    // NBCD
    //
    //       Syntax: NBCD <ea>
    //        Sizes: Byte

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0100 1000 00-- ----");
    __________MMMXXX(opcode, NBCD, 0b101111111000, Byte, Nbcd);


    // NEG, NEGX, NOT
    //
    //       Syntax: Nxx <ea>
    //        Sizes: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0100 0100 ---- ----");
    ________SSMMMXXX(opcode, NEG, 0b101111111000, Byte | Word | Long, NegNot);

    opcode = parse("0100 0000 ---- ----");
    ________SSMMMXXX(opcode, NEGX, 0b101111111000, Byte | Word | Long, NegNot);

    opcode = parse("0100 0110 ---- ----");
    ________SSMMMXXX(opcode, NOT, 0b101111111000, Byte | Word | Long, NegNot);


    // NOP
    //
    //       Syntax: NOP
    //        Sizes: Unsized

    opcode = parse("0100 1110 0111 0001");
    register(opcode, Nop<NOP __ 0 __ Long>);


    // OR
    //
    //       Syntax: OR <ea>,Dy
    //               OR Dx,<ea>
    //        Sizes: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>,Dy       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X

     opcode = parse("1000 ---0 ---- ----");
    ____XXX_SSMMMXXX(opcode, OR, 0b101111111111, Byte | Word | Long, AndEaRg);

    //               -------------------------------------------------
    // Dx,<ea>       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X

    opcode = parse("1000 ---1 ---- ----");
    ____XXX_SSMMMXXX(opcode, OR, 0b001111111000, Byte | Word | Long, AndRgEa);


    // ROL, ROR, ROXL, ROXR
    //
    //       Syntax: (1) ROxx Dx,Dy
    //               (2) ROxx #<data>,Dy
    //               (3) ROxx <ea>
    //        Sizes: Byte, Word, Longword

    // Dx,Dy
    opcode = parse("1110 ---1 --11 1---");
    ____XXX_SS___XXX(opcode, ROL, 0b0, Byte | Word | Long, ShiftRg);

    opcode = parse("1110 ---0 --11 1---");
    ____XXX_SS___XXX(opcode, ROR, 0b0, Byte | Word | Long, ShiftRg);

    opcode = parse("1110 ---1 --11 0---");
    ____XXX_SS___XXX(opcode, ROXL, 0b0, Byte | Word | Long, ShiftRg);

    opcode = parse("1110 ---0 --11 0---");
    ____XXX_SS___XXX(opcode, ROXR, 0b0, Byte | Word | Long, ShiftRg);

    // #<data>,Dy
    opcode = parse("1110 ---1 --01 1---");
    ____XXX_SS___XXX(opcode, ROL, 0xB, Byte | Word | Long, ShiftIm);

    opcode = parse("1110 ---0 --01 1---");
    ____XXX_SS___XXX(opcode, ROR, 0xB, Byte | Word | Long, ShiftIm);

    opcode = parse("1110 ---1 --01 0---");
    ____XXX_SS___XXX(opcode, ROXL, 0xB, Byte | Word | Long, ShiftIm);

    opcode = parse("1110 ---0 --01 0---");
    ____XXX_SS___XXX(opcode, ROXR, 0xB, Byte | Word | Long, ShiftIm);

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X   X   X   X   X   X   X

    opcode = parse("1110 0111 11-- ----");
    __________MMMXXX(opcode, ROL, 0b001111111000, Word, Shift);

    opcode = parse("1110 0110 11-- ----");
    __________MMMXXX(opcode, ROR, 0b001111111000, Word, Shift);

    opcode = parse("1110 0101 11-- ----");
    __________MMMXXX(opcode, ROXL, 0b001111111000, Word, Shift);

    opcode = parse("1110 0100 11-- ----");
    __________MMMXXX(opcode, ROXR, 0b001111111000, Word, Shift);

}

void
CPU::registerSBCD()
{
    u16 opcode;

    // SBCD
    //
    //       Syntax: (1) SBCD Dy,Dx
    //               (2) SBCD -(Ay),-(Ax)
    //         Size: Byte

    // Dy,Dx
    opcode = parse("1000 ---1 0000 0---");
    ____XXX______XXX(opcode, SBCD, 0, Byte, Abcd);

    // -(Ay),-(Ax)
    opcode = parse("1000 ---1 0000 1---");
    ____XXX______XXX(opcode, SBCD, 4, Byte, Abcd);
}

void
CPU::registerScc()
{
    u16 opcode;

    // Scc
     //
     //       Syntax: Scc Dn,<label>
     //         Size: Word

     // Dn,<label>
    /*
     opcode = parse("0101 ---- 1100 0---");
     _____________XXX(opcode | 0x000, ST,  0, Word, Scc);
     _____________XXX(opcode | 0x100, SF,  0, Word, Scc);
     _____________XXX(opcode | 0x200, SHI, 0, Word, Scc);
     _____________XXX(opcode | 0x300, SLS, 0, Word, Scc);
     _____________XXX(opcode | 0x400, SCC, 0, Word, Scc);
     _____________XXX(opcode | 0x500, SCS, 0, Word, Dbcc);
     _____________XXX(opcode | 0x600, SNE, 0, Word, Dbcc);
     _____________XXX(opcode | 0x700, SEQ, 0, Word, Dbcc);
     _____________XXX(opcode | 0x800, SVC, 0, Word, Dbcc);
     _____________XXX(opcode | 0x900, SVS, 0, Word, Dbcc);
     _____________XXX(opcode | 0xA00, SPL, 0, Word, Dbcc);
     _____________XXX(opcode | 0xB00, SMI, 0, Word, Dbcc);
     _____________XXX(opcode | 0xC00, SGE, 0, Word, Dbcc);
     _____________XXX(opcode | 0xD00, SLT, 0, Word, Dbcc);
     _____________XXX(opcode | 0xE00, SGT, 0, Word, Dbcc);
     _____________XXX(opcode | 0xF00, SLE, 0, Word, Dbcc);

*/
    
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

        register(opcode | 0 << 3 | reg, SccEa<I __ 0 __ Byte>);
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
    // SUB
    //
    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // <ea>,Dy        X  (X)  X   X   X   X   X   X   X   X   X   X

    u16 opcode = parse("1001 ---0 ---- ----");
    ____XXX_SSMMMXXX(opcode, SUB, 0b101111111111, Byte,        AddEaRg);
    ____XXX_SSMMMXXX(opcode, SUB, 0b111111111111, Word | Long, AddEaRg);

    //              -------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // Dx,<ea>                X   X   X   X   X   X   X

    opcode = parse("1001 ---1 ---- ----");
    ____XXX_SSMMMXXX(opcode, SUB, 0b001111111000, Byte | Word | Long, AddRgEa);
}

void
CPU::registerSUBA()
{
    u16 opcode = parse("1001 ---- 11-- ----");

    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // <ea>,An        X   X   X   X   X   X   X   X   X   X   X   X

    ____XXXS__MMMXXX(opcode, SUBA, 0b111111111111, Word | Long, Adda)
}

void
CPU::registerTAS()
{
    u16 opcode = parse("0100 1010 11-- ----");

    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // <ea>           X       X   X   X   X   X   X   X

    __________MMMXXX(opcode, TAS, 0b101111111000, Byte, Tas);
}

void
CPU::registerTST()
{
    u16 opcode = parse("0100 1010 ---- ----");

    //              -------------------------------------------------
    // Modes:       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //              -------------------------------------------------
    // <ea>           X       X   X   X   X   X   X   X   X   X

    ________SSMMMXXX(opcode, TST, 0b101111111110, Byte | Word | Long, Tst);
}

#undef __
