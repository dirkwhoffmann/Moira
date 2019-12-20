// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// Adds a single entry to the instruction jump table
#define __ ,
#define register(id, name) { \
if (exec[id] != &Moira::execIllegal) printf("id = %x\n", id); \
assert(exec[id] == &Moira::execIllegal); \
assert(dasm[id] == &Moira::dasmIllegal); \
exec[id] = &Moira::exec##name; \
dasm[id] = &Moira::dasm##name; }

// Registers an instruction in one of the standard instruction formats:
//
//     Variants:  ____ ____ ____ _XXX
//                ____ ____ ____ XXXX
//                ____ ___X XX__ _XXX
//                ____ XXX_ ____ _XXX
//                ____ ____ XXXX XXXX
//                ____ XXX_ XXXX XXXX
//                ____ ____ SS__ _XXX
//                ____ XXX_ SS__ _XXX
//                ____ ____ __MM MXXX
//                ____ XXXM MM__ ____
//                ____ MMMX XXMM MXXX
//                ____ ___S __MM MXXX
//                ____ XXX_ __MM MXXX
//                ____ XXXS __MM MXXX
//                ____ XXX_ SSMM MXXX
//                __SS MMMX XXMM MXXX
//
//       Legend:  XXX : Operand parameters    (0 .. 7)
//                  S : Size information      (0 = Word, 1 = Long)
//                 SS : Size information      (0 = Byte, 1 = Word, 2 = Long)

#define _____________XXX(op,I,M,S,f) { \
for (int j = 0; j < 8; j++) register((op) | j, f<I __ M __ S>); }

#define ____________XXXX(op,I,M,S,f) { \
for (int j = 0; j < 16; j++) register((op) | j, f<I __ M __ S>); }

#define _______XXX___XXX(op,I,M,S,f) { \
for (int i = 0; i < 8; i++) _____________XXX((op) | i << 6, I, M, S, f); }

#define ____XXX______XXX(op,I,M,S,f) { \
for (int i = 0; i < 8; i++) _____________XXX((op) | i << 9, I, M, S, f); }

#define ________XXXXXXXX(op,I,M,S,f) { \
for (int j = 0; j < 256; j++) register((op) | j, f<I __ M __ S>); }

#define ________SS___XXX(op,I,M,s,f) { \
if ((s) & 0b100) _____________XXX((op) | 2 << 6, I, M, Long, f); \
if ((s) & 0b010) _____________XXX((op) | 1 << 6, I, M, Word, f); \
if ((s) & 0b001) _____________XXX((op) | 0 << 6, I, M, Byte, f); }

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

#define ____XXXMMM______(op,I,M,m,S,f) { \
for (int j = 0; j < 8; j++) { \
if ((m) & 0b100000000000) register((op) | 0 << 6 | j << 9, f<I __ M __  0 __ S>); \
if ((m) & 0b010000000000) register((op) | 1 << 6 | j << 9, f<I __ M __  1 __ S>); \
if ((m) & 0b001000000000) register((op) | 2 << 6 | j << 9, f<I __ M __  2 __ S>); \
if ((m) & 0b000100000000) register((op) | 3 << 6 | j << 9, f<I __ M __  3 __ S>); \
if ((m) & 0b000010000000) register((op) | 4 << 6 | j << 9, f<I __ M __  4 __ S>); \
if ((m) & 0b000001000000) register((op) | 5 << 6 | j << 9, f<I __ M __  5 __ S>); \
if ((m) & 0b000000100000) register((op) | 6 << 6 | j << 9, f<I __ M __  6 __ S>); \
} \
if ((m) & 0b000000010000) register((op) | 7 << 6 | 0 << 9, f<I __ M __  7 __ S>); \
if ((m) & 0b000000001000) register((op) | 7 << 6 | 1 << 9, f<I __ M __  8 __ S>); \
if ((m) & 0b000000000100) register((op) | 7 << 6 | 2 << 9, f<I __ M __  9 __ S>); \
if ((m) & 0b000000000010) register((op) | 7 << 6 | 3 << 9, f<I __ M __ 10 __ S>); \
if ((m) & 0b000000000001) register((op) | 7 << 6 | 4 << 9, f<I __ M __ 11 __ S>); }

#define ____XXXMMMMMMXXX(op,I,m,m2,S,f) { \
for (int k = 0; k < 8; k++) { \
if ((m) & 0b100000000000) ____XXXMMM______((op) | 0 << 3 | k, I,  0, m2, S, f); \
if ((m) & 0b010000000000) ____XXXMMM______((op) | 1 << 3 | k, I,  1, m2, S, f); \
if ((m) & 0b001000000000) ____XXXMMM______((op) | 2 << 3 | k, I,  2, m2, S, f); \
if ((m) & 0b000100000000) ____XXXMMM______((op) | 3 << 3 | k, I,  3, m2, S, f); \
if ((m) & 0b000010000000) ____XXXMMM______((op) | 4 << 3 | k, I,  4, m2, S, f); \
if ((m) & 0b000001000000) ____XXXMMM______((op) | 5 << 3 | k, I,  5, m2, S, f); \
if ((m) & 0b000000100000) ____XXXMMM______((op) | 6 << 3 | k, I,  6, m2, S, f); \
} \
if ((m) & 0b000000010000) ____XXXMMM______((op) | 7 << 3 | 0, I,  7, m2, S, f); \
if ((m) & 0b000000001000) ____XXXMMM______((op) | 7 << 3 | 1, I,  8, m2, S, f); \
if ((m) & 0b000000000100) ____XXXMMM______((op) | 7 << 3 | 2, I,  9, m2, S, f); \
if ((m) & 0b000000000010) ____XXXMMM______((op) | 7 << 3 | 3, I, 10, m2, S, f); \
if ((m) & 0b000000000001) ____XXXMMM______((op) | 7 << 3 | 4, I, 11, m2, S, f); }

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

#define ______MMXXXMMXXX(op,I,m,s,f) { \
for (int i = 0; i < 8; i++) ________SSMMMXXX((op) | i << 9, I, m, s, f) }

#define __SSXXXMMMMMMXXX(op,I,m,m2,s,f) { \
if ((s) & 0b100) ____XXXMMMMMMXXX((op) | 2 << 12, I, m, m2, Long, f); \
if ((s) & 0b010) ____XXXMMMMMMXXX((op) | 3 << 12, I, m, m2, Word, f); \
if ((s) & 0b001) ____XXXMMMMMMXXX((op) | 1 << 12, I, m, m2, Byte, f); }


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
Moira::createJumpTables()
{
    u16 opcode;

    //
    // Start with clean tables
    //

    for (int i = 0; i < 0x10000; i++) {
        exec[i] = &Moira::execIllegal;
        dasm[i] = &Moira::dasmIllegal;
    }


    // Unimplemented instructions
    //
    //       Format: 1010 ---- ---- ---- (Line A instructions)
    //               1111 ---- ---- ---- (Line F instructions)

    for (int i = 0; i < 0x1000; i++) {

        exec[0b1010 << 12 | i] = &Moira::execLineA;
        dasm[0b1010 << 12 | i] = &Moira::dasmLineA;

        exec[0b1111 << 12 | i] = &Moira::execLineF;
        dasm[0b1111 << 12 | i] = &Moira::dasmLineF;
    }


    // ABCD
    //
    //       Syntax: (1) ABCD Dx,Dy
    //               (2) ABCD -(Ax),-(Ay)
    //         Size: Byte

    // Dx,Dy
    opcode = parse("1100 ---1 0000 0---");
    ____XXX______XXX(opcode, ABCD, 0, Byte, Abcd);

    // -(Ax),-(Ay)
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
    //       Syntax: ADDA <ea>,Ay
    //         Size: Word, Longword
    //
    //               -------------------------------------------------
    // <ea>,Ay       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X   X   X   X   X   X   X   X   X   X   X   X

    opcode = parse("1101 ---- 11-- ----");
    ____XXXS__MMMXXX(opcode, ADDA, 0b111111111111, Word | Long, Adda)


    // ADDI
    //
    //       Syntax: ADDI #<data>,<ea>
    //         Size: Byte, Word, Longword
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 0110 ---- ----");
    ________SSMMMXXX(opcode, ADDI, 0b101111111000, Byte | Word | Long, Addi);


    // ADDQ
    //
    //       Syntax: ADDQ #<data>,<ea>
    //         Size: Byte, Word, Longword
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X  (X)  X   X   X   X   X   X   X

    opcode = parse("0101 ---0 ---- ----");
    ____XXX_SSMMMXXX(opcode, ADDQ, 0b101111111000, Byte | Word | Long, Addq);
    ____XXX_SSMMMXXX(opcode, ADDQ, 0b010000000000, Word | Long, AddqAn);


    // ADDX
    //
    //       Syntax: (1) ADDX Dx,Dy
    //               (2) ADDX -(Ax),-(Ay)
    //         Size: Byte, Word, Longword

    // Dx,Dy
    opcode = parse("1101 ---1 --00 0---");
    ____XXX_SS___XXX(opcode, ADDX, 0, Byte | Word | Long, AddxRg);

    // -(Ax),-(Ay)
    opcode = parse("1101 ---1 --00 1---");
    ____XXX_SS___XXX(opcode, ADDX, 4, Byte | Word | Long, AddxEa);


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


    // ANDI
    //
    //       Syntax: ANDI #<data>,<ea>
    //         Size: Byte, Word, Longword
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 0010 ---- ----");
    ________SSMMMXXX(opcode, ANDI, 0b101111111000, Byte | Word | Long, Andi);


    // ANDI to CCR
    //
    //       Syntax: ANDI #<data>,CCR
    //         Size: Byte
    //

    register(parse("0000 0010 0011 1100"), Andiccr<ANDICCR __ 11 __ Byte>);


    // ANDI to SR
    //
    //       Syntax: ANDI #<data>,SR
    //         Size: Byte
    //

    register(parse("0000 0010 0111 1100"), Andisr<ANDISR __ 11 __ Word>);


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
    __________MMMXXX(opcode, ASL, 0b001111111000, Word, ShiftEa);

    opcode = parse("1110 0000 11-- ----");
    __________MMMXXX(opcode, ASR, 0b001111111000, Word, ShiftEa);


    // Bcc
    //
    //       Syntax: Bcc <label>
    //         Size: Word

    // Dn,<label>
    opcode = parse("0110 ---- ---- ----");

    register(opcode | 0x000, Bcc<BRA __ 0 __ Word>);
    register(opcode | 0x200, Bcc<BHI __ 0 __ Word>);
    register(opcode | 0x300, Bcc<BLS __ 0 __ Word>);
    register(opcode | 0x400, Bcc<BCC __ 0 __ Word>);
    register(opcode | 0x500, Bcc<BCS __ 0 __ Word>);
    register(opcode | 0x600, Bcc<BNE __ 0 __ Word>);
    register(opcode | 0x700, Bcc<BEQ __ 0 __ Word>);
    register(opcode | 0x800, Bcc<BVC __ 0 __ Word>);
    register(opcode | 0x900, Bcc<BVS __ 0 __ Word>);
    register(opcode | 0xA00, Bcc<BPL __ 0 __ Word>);
    register(opcode | 0xB00, Bcc<BMI __ 0 __ Word>);
    register(opcode | 0xC00, Bcc<BGE __ 0 __ Word>);
    register(opcode | 0xD00, Bcc<BLT __ 0 __ Word>);
    register(opcode | 0xE00, Bcc<BGT __ 0 __ Word>);
    register(opcode | 0xF00, Bcc<BLE __ 0 __ Word>);

    for (int i = 1; i < 255; i++) {
        register(opcode | 0x000 | i, Bcc<BRA __ 0 __ Byte>);
        register(opcode | 0x200 | i, Bcc<BHI __ 0 __ Byte>);
        register(opcode | 0x300 | i, Bcc<BLS __ 0 __ Byte>);
        register(opcode | 0x400 | i, Bcc<BCC __ 0 __ Byte>);
        register(opcode | 0x500 | i, Bcc<BCS __ 0 __ Byte>);
        register(opcode | 0x600 | i, Bcc<BNE __ 0 __ Byte>);
        register(opcode | 0x700 | i, Bcc<BEQ __ 0 __ Byte>);
        register(opcode | 0x800 | i, Bcc<BVC __ 0 __ Byte>);
        register(opcode | 0x900 | i, Bcc<BVS __ 0 __ Byte>);
        register(opcode | 0xA00 | i, Bcc<BPL __ 0 __ Byte>);
        register(opcode | 0xB00 | i, Bcc<BMI __ 0 __ Byte>);
        register(opcode | 0xC00 | i, Bcc<BGE __ 0 __ Byte>);
        register(opcode | 0xD00 | i, Bcc<BLT __ 0 __ Byte>);
        register(opcode | 0xE00 | i, Bcc<BGT __ 0 __ Byte>);
        register(opcode | 0xF00 | i, Bcc<BLE __ 0 __ Byte>);
    }


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


    // BSR
    //
    //       Syntax: (1) BSR <label>
    //         Size: Byte, Word

    opcode = parse("0110 0001 ---- ----");
    register(opcode, Bsr<BSR __ 0 __ Word>);
    for (int i = 1; i < 255; i++) {
        register(opcode | i, Bsr<BSR __ 0 __ Byte>);
    }

    // BTST
    //
    //       Syntax: (1) BTST Dx,<ea>
    //               (2) BTST #<data>,<ea>
    //         Size: Byte, Longword

    //               -------------------------------------------------
    // Dx,<ea>       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X?

    opcode = parse("0000 ---1 00-- ----");
    ____XXX___MMMXXX(opcode, BTST, 0b101111111111, Byte, BitDxEa);

    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X

    opcode = parse("0000 1000 00-- ----");
    __________MMMXXX(opcode, BTST, 0b101111111110, Long, BitImEa);


    // CHK
    //
    //       Syntax: CHK <ea>,Dy
    //         Size: Word

    //               -------------------------------------------------
    // <ea>,Ay       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X?

    opcode = parse("0100 ---1 10-- ----");
    ____XXX___MMMXXX(opcode, CHK, 0b101111111111, Word, Chk);


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


    // CMPI
    //
    //       Syntax: CMPI #<data>,<ea>
    //         Size: Byte, Word, Longword
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 1100 ---- ----");
    ________SSMMMXXX(opcode, CMPI, 0b101111111000, Byte | Word | Long, Cmpi);


    // CMPM
    //
    //       Syntax: (1) CMPM (Ax)+,(Ay)+
    //         Size: Byte, Word, Longword

    // (Ax)+,(Ay)+
    opcode = parse("1011 ---1 --00 1---");
    ____XXX_SS___XXX(opcode, CMPM, 3, Byte | Word | Long, Cmpm);


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
    ____XXX___MMMXXX(opcode, DIVS, 0b101111111111, Word, MulDiv);

    opcode = parse("1000 ---0 11-- ----");
    ____XXX___MMMXXX(opcode, DIVU, 0b101111111111, Word, MulDiv);


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


    // EORI
    //
    //       Syntax: EORI #<data>,<ea>
    //         Size: Byte, Word, Longword
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 1010 ---- ----");
    ________SSMMMXXX(opcode, EORI, 0b101111111000, Byte | Word | Long, Andi);


    // EORI to CCR
    //
    //       Syntax: EORI #<data>,CCR
    //         Size: Byte
    //

    register(parse("0000 1010 0011 1100"), Andiccr<EORICCR __ 11 __ Byte>);


    // EORI to SR
    //
    //       Syntax: EORI #<data>,SR
    //         Size: Byte
    //

    register(parse("0000 1010 0111 1100"), Andisr<EORISR __ 11 __ Word>);


    // EXG
    //
    //       Syntax: EXG Dx,Dy
    //               EXG Ax,Dy
    //               EXG Ax,Ay
    //         Size: Longword

    opcode = parse("1100 ---1 0100 0---");
    ____XXX______XXX(opcode, EXG, 0, Long, ExgDxDy);

    opcode = parse("1100 ---1 1000 1---");
    ____XXX______XXX(opcode, EXG, 0, Long, ExgAxDy);

    opcode = parse("1100 ---1 0100 1---");
    ____XXX______XXX(opcode, EXG, 0, Long, ExgAxAy);


    // EXT
    //
    //       Syntax: EXT Dx
    //        Sizes: Word, Longword

    opcode = parse("0100 1000 --00 0---");
    _____________XXX(opcode | 2 << 6, EXT, 0, Word, Ext);
    _____________XXX(opcode | 3 << 6, EXT, 0, Long, Ext);


    // LINK
    //
    //       Syntax: LINK An,#<displacement>
    //        Sizes: Word

    opcode = parse("0100 1110 0101 0---");
    _____________XXX(opcode, LINK, 0, Word, Link);


    // JMP
    //
    //       Syntax: JMP <ea>
    //        Sizes: Longword

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X           X   X   X   X   X   X

     opcode = parse("0100 1110 11-- ----");
     __________MMMXXX(opcode, JMP, 0b001001111110, Long, Jmp);


    // JSR
    //
    //       Syntax: JSR <ea>
    //        Sizes: Longword

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X           X   X   X   X   X   X

     opcode = parse("0100 1110 10-- ----");
     __________MMMXXX(opcode, JSR, 0b001001111110, Long, Jsr);


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
    __________MMMXXX(opcode, LSL, 0b001111111000, Word, ShiftEa);

    opcode = parse("1110 0010 11-- ----");
    __________MMMXXX(opcode, LSR, 0b001111111000, Word, ShiftEa);


    // MOVE
    //
    //       Syntax: MOVE <ea>,<e>
    //        Sizes: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X  (X)   X   X   X   X   X   X   X   X   X   X

    //               -------------------------------------------------
    // <e>           | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("00-- ---- ---- ----");

    __SSXXXMMMMMMXXX(opcode, MOVE,
                     0b101111111111, 0b101111111000,
                     Byte, Move);

    __SSXXXMMMMMMXXX(opcode, MOVE,
                     0b111111111111, 0b101111111000,
                     Word | Long, Move);


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


    // MOVEM
    //
    //       Syntax: MOVEM <ea>,<register list>
    //               MOVEM <register list>,<ea>
    //        Sizes: Word, Longword

    //               -------------------------------------------------
    // <ea>,<list>   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X   X       X   X   X   X   X   X

    //               -------------------------------------------------
    // <list>,<ea>   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X       X   X   X   X   X

    opcode = parse("0100 1100 1--- ----");
    __________MMMXXX(opcode | 0 << 6, MOVEM, 0b001101111110, Word, MovemEaRg);
    __________MMMXXX(opcode | 1 << 6, MOVEM, 0b001101111110, Long, MovemEaRg);

    opcode = parse("0100 1000 1--- ----");
    __________MMMXXX(opcode | 0 << 6, MOVEM, 0b001011111000, Word, MovemRgEa);
    __________MMMXXX(opcode | 1 << 6, MOVEM, 0b001011111000, Long, MovemRgEa);


    // MOVEP
    //
    //       Syntax: MOVEP Dx,(d,Ay)
    //               MOVEP (d,Ay),Dx
    //         Size: Word, Longword

    // MOVEP Dx,(d,Ay)
    opcode = parse("0000 ---1 1-00 1---");
    ____XXX______XXX(opcode | 0 << 6, MOVEP, 5, Word, MovepDxEa);
    ____XXX______XXX(opcode | 1 << 6, MOVEP, 5, Long, MovepDxEa);

    // MOVEP (d,Ay),Dx
    opcode = parse("0000 ---1 0-00 1---");
    ____XXX______XXX(opcode | 0 << 6, MOVEP, 5, Word, MovepEaDx);
    ____XXX______XXX(opcode | 1 << 6, MOVEP, 5, Long, MovepEaDx);


    // MOVEQ
    //
    //       Syntax: MOVEQ #<data>,Dn
    //        Sizes: Longword

    // #<data>,Dn
    opcode = parse("0111 ---0 ---- ----");
    ____XXX_XXXXXXXX(opcode, MOVEQ, 11, Long, Moveq);


    // MOVE to CCR
    //
    //       Syntax: MOVE <ea>,CCR
    //         Size: Word
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X

    opcode = parse("0100 0100 11-- ----");
    __________MMMXXX(opcode, MOVETSR, 0b101111111111, Byte, MoveToCcr);


    // MOVE from SR
    //
    //       Syntax: MOVE SR,<ea>
    //         Size: Word
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0100 0000 11-- ----");
    __________MMMXXX(opcode, MOVEFSR, 0b101111111000, Word, MoveFromSr);


    // MOVE to SR
    //
    //       Syntax: MOVE <ea>,SR
    //         Size: Word
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X

    opcode = parse("0100 0110 11-- ----");
    __________MMMXXX(opcode, MOVETSR, 0b101111111111, Word, MoveToSr);


    // MOVEUSP
    //
    //       Syntax: MOVE USP,An
    //               MOVE An,USP
    //        Sizes: Longword

    opcode = parse("0100 1110 0110 ----");
    _____________XXX(opcode | 1 << 3, MOVEUSP, 0, Long, MoveUspAn);
    _____________XXX(opcode | 0 << 3, MOVEUSP, 0, Long, MoveAnUsp);


    // MULS, MULU
    //
    //       Syntax: MULx <ea>,Dy
    //        Sizes: Word x Word -> Longword

    //               -------------------------------------------------
    // <ea>,Dy       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X   X   X   X

    opcode = parse("1100 ---1 11-- ----");
    ____XXX___MMMXXX(opcode, MULS, 0b101111111111, Word, MulDiv);

    opcode = parse("1100 ---0 11-- ----");
    ____XXX___MMMXXX(opcode, MULU, 0b101111111111, Word, MulDiv);


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


    // ORI
    //
    //       Syntax: ORI #<data>,<ea>
    //         Size: Byte, Word, Longword
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 0000 ---- ----");
    ________SSMMMXXX(opcode, ORI, 0b101111111000, Byte | Word | Long, Andi);


    // ORI to CCR
    //
    //       Syntax: ORI #<data>,CCR
    //         Size: Byte
    //

    register(parse("0000 0000 0011 1100"), Andiccr<ORICCR __ 11 __ Byte>);


    // ORI to SR
    //
    //       Syntax: ORI #<data>,SR
    //         Size: Byte
    //

    register(parse("0000 0000 0111 1100"), Andisr<ORISR __ 11 __ Word>);


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
    __________MMMXXX(opcode, ROL, 0b001111111000, Word, ShiftEa);

    opcode = parse("1110 0110 11-- ----");
    __________MMMXXX(opcode, ROR, 0b001111111000, Word, ShiftEa);

    opcode = parse("1110 0101 11-- ----");
    __________MMMXXX(opcode, ROXL, 0b001111111000, Word, ShiftEa);

    opcode = parse("1110 0100 11-- ----");
    __________MMMXXX(opcode, ROXR, 0b001111111000, Word, ShiftEa);


    // PEA
    //
    //       Syntax: PEA <ea>,Ay
    //        Sizes: Longword

    //               -------------------------------------------------
    // <ea>,Ay       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X           X   X   X   X   X   X

    opcode = parse("0100 1000 01-- ----");
    __________MMMXXX(opcode, PEA, 0b001001111110, Long, Pea);


    // RESET
    //
    //       Syntax: RESET
    //        Sizes: Unsized

    register(parse("0100 1110 0111 0000"), Reset<RESET __ 0 __ Long>);


    // RTE
    //
    //       Syntax: RTE
    //        Sizes: Unsized

    register(parse("0100 1110 0111 0011"), Rte<RTE __ 0 __ Long>);


    // RTR
    //
    //       Syntax: RTR
    //        Sizes: Unsized

    register(parse("0100 1110 0111 0111"), Rtr<RTR __ 0 __ Long>);


    // RTS
    //
    //       Syntax: RTS
    //        Sizes: Unsized

    register(parse("0100 1110 0111 0101"), Rts<RTS __ 0 __ Long>);


    // SBCD
    //
    //       Syntax: (1) SBCD Dx,Dy
    //               (2) SBCD -(Ax),-(Ay)
    //         Size: Byte

    // Dx,Dy
    opcode = parse("1000 ---1 0000 0---");
    ____XXX______XXX(opcode, SBCD, 0, Byte, Abcd);

    // -(Ax),-(Ay)
    opcode = parse("1000 ---1 0000 1---");
    ____XXX______XXX(opcode, SBCD, 4, Byte, Abcd);


    // Scc
    //
    //       Syntax: Scc <ea>
    //         Size: Word

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0101 ---- 11-- ----");
    __________MMMXXX(opcode | 0x000, ST,  0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0x100, SF,  0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0x200, SHI, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0x300, SLS, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0x400, SCC, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0x500, SCS, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0x600, SNE, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0x700, SEQ, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0x800, SVC, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0x900, SVS, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0xA00, SPL, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0xB00, SMI, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0xC00, SGE, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0xD00, SLT, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0xE00, SGT, 0b101111111000, Word, Scc);
    __________MMMXXX(opcode | 0xF00, SLE, 0b101111111000, Word, Scc);


    // STOP
    //
    //       Syntax: STOP
    //        Sizes: Unsized

    register(parse("0100 1110 0111 0010"), Stop<STOP __ 11 __ Word>);


    // SUB
    //
    //       Syntax: (1) SUB <ea>,Dy
    //               (2) SUB Dx,<ea>
    //         Size: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>,Dy       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X  (X)  X   X   X   X   X   X   X   X   X   X

    opcode = parse("1001 ---0 ---- ----");
    ____XXX_SSMMMXXX(opcode, SUB, 0b101111111111, Byte,        AddEaRg);
    ____XXX_SSMMMXXX(opcode, SUB, 0b111111111111, Word | Long, AddEaRg);

    //               -------------------------------------------------
    // Dx,<ea>       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                         X   X   X   X   X   X   X

    opcode = parse("1001 ---1 ---- ----");
    ____XXX_SSMMMXXX(opcode, SUB, 0b001111111000, Byte | Word | Long, AddRgEa);


    // SUBA
    //
    //       Syntax: SUBA <ea>,Ay
    //         Size: Word, Longword
    //
    //               -------------------------------------------------
    // <ea>,Ay       | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X   X   X   X   X   X   X   X   X   X   X   X

    opcode = parse("1001 ---- 11-- ----");
    ____XXXS__MMMXXX(opcode, SUBA, 0b111111111111, Word | Long, Adda)


    // SUBI
    //
    //       Syntax: SUBI #<data>,<ea>
    //         Size: Byte, Word, Longword
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0000 0100 ---- ----");
    ________SSMMMXXX(opcode, SUBI, 0b101111111000, Byte | Word | Long, Addi);


    // SUBQ
    //
    //       Syntax: SUBQ #<data>,<ea>
    //         Size: Byte, Word, Longword
    //
    //               -------------------------------------------------
    // #<data>,<ea>  | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X  (X)  X   X   X   X   X   X   X

    opcode = parse("0101 ---1 ---- ----");
    ____XXX_SSMMMXXX(opcode, SUBQ, 0b101111111000, Byte | Word | Long, Addq);
    ____XXX_SSMMMXXX(opcode, SUBQ, 0b010000000000, Word | Long, AddqAn);


    // SUBX
    //
    //       Syntax: (1) SUBX Dx,Dy
    //               (2) SUBX -(Ax),-(Ay)
    //         Size: Byte, Word, Longword

    // Dx,Dy
    opcode = parse("1001 ---1 --00 0---");
    ____XXX_SS___XXX(opcode, SUBX, 0, Byte | Word | Long, AddxRg);

    // -(Ax),-(Ay)
    opcode = parse("1001 ---1 --00 1---");
    ____XXX_SS___XXX(opcode, SUBX, 4, Byte | Word | Long, AddxEa);


    // SWAP
    //
    //       Syntax: SWAP Dn
    //         Size: Word

    opcode = parse("0100 1000 0100 0---");
    _____________XXX(opcode, SWAP, 0, Word, Swap);


    // TAS
    //
    //       Syntax: TAS <ea>
    //         Size: Byte

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0100 1010 11-- ----");
    __________MMMXXX(opcode, TAS, 0b101111111000, Byte, Tas);


    // TRAP
    //
    //       Syntax: TRAP #<vector>
    //        Sizes: Unsized

    opcode = parse("0100 1110 0100 ----");
    ____________XXXX(opcode, TRAP, 0, Long, Trap);


    // TRAPV
    //
    //       Syntax: TRAPV
    //        Sizes: Unsized

    register(parse("0100 1110 0111 0110"), Trapv<TRAPV __ 0 __ Long>);


    // TST
    //
    //       Syntax: TST <ea>
    //         Size: Byte, Word, Longword

    //               -------------------------------------------------
    // <ea>          | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | A | B |
    //               -------------------------------------------------
    //                 X       X   X   X   X   X   X   X

    opcode = parse("0100 1010 ---- ----");
    ________SSMMMXXX(opcode, TST, 0b101111111000, Byte | Word | Long, Tst);


    // UNLK
    //
    //       Syntax: UNLK An
    //        Sizes: Word

    opcode = parse("0100 1110 0101 1---");
    _____________XXX(opcode, UNLK, 0, Word, Unlk);
}

#undef __

