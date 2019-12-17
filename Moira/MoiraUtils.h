// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef MOIRA_BASE_H
#define MOIRA_BASE_H

#include "types.h"

#define REVERSE_8(x) (((x) * 0x0202020202ULL & 0x010884422010ULL) % 1023)
#define REVERSE_16(x) ((REVERSE_8((x) & 0xFF) << 8) | REVERSE_8(((x) >> 8) & 0xFF))

#define ______________xx(opcode) (u16)((opcode >> 0)  & 0b11)
#define _____________xxx(opcode) (u16)((opcode >> 0)  & 0b111)
#define ____________xxxx(opcode) (u16)((opcode >> 0)  & 0b1111)
#define ________xxxxxxxx(opcode) (u16)((opcode >> 0)  & 0b11111111)
#define __________xxx___(opcode) (u16)((opcode >> 3)  & 0b111)
#define __________xx____(opcode) (u16)((opcode >> 4)  & 0b11)
#define _______xxx______(opcode) (u16)((opcode >> 6)  & 0b111)
#define _________x______(opcode) (u16)((opcode >> 6)  & 0b1)
#define ________x_______(opcode) (u16)((opcode >> 7)  & 0b1)
#define _______x________(opcode) (u16)((opcode >> 8)  & 0b1)
#define _____xx_________(opcode) (u16)((opcode >> 9)  & 0b11)
#define ____xxx_________(opcode) (u16)((opcode >> 9)  & 0b111)
#define ____x___________(opcode) (u16)((opcode >> 11) & 0b1)
#define xxxx____________(opcode) (u16)((opcode >> 12) & 0b1111)


namespace moira {

enum Size { Byte = 1, Word = 2, Long = 4 };

template<Size>   int  BYTES();
template<Size>   u32  MASK();
template<Size>   u32  MSBIT();
template<Size>   i32  SIGN (u64 data);
template<Size>   u32  CLIP (u64 data);
template<Size>   u32  CLEAR(u64 data);
template<Size>   u32  SEXT (u64 data);
template<Size>   bool NBIT (u64 data);
template<Size>   bool CARRY(u64 data);
template<Size>   bool ZERO (u64 data);
template<Size S> u32  SUBS (u32 d1, u32 d2) { return CLEAR<S>(d1) | CLIP<S>(d2); }

enum Instr {
    ABCD,    ADD,     ADDA,    ADDI,    ADDQ,    ADDX,    AND,     ANDI,
    ANDICCR, ANDISR,  ASL,     ASR,
    BCC,     BCS,     BEQ,     BGE,     BGT,     BHI,     BLE,     BLS,
    BLT,     BMI,     BNE,     BPL,     BVC,     BVS,     BCHG,    BCLR,
    BRA,     BSET,    BSR,     BTST,
    CHK,     CLR,     CMP,     CMPA,    CMPI,    CMPM,
    DBCC,    DBCS,    DBEQ,    DBGE,    DBGT,    DBHI,    DBLE,    DBLS,
    DBLT,    DBMI,    DBNE,    DBPL,    DBVC,    DBVS,    DBF,     DBT,
    DIVS,    DIVU,
    EOR,     EORI,    EORICCR, EORISR,  EXG,     EXT,
    ILLEGAL,
    JMP,     JSR,
    LEA,     LINK,    LSL,     LSR,
    MOVE,    MOVEA,   MOVECCR, MOVEFSR, MOVETSR, MOVEUSP, MOVEM,   MOVEP,
    MOVEQ,   MULS,    MULU,
    NBCD,    NEG,     NEGX,    NOP,     NOT,
    OR,      ORI,     ORICCR,  ORISR,
    PEA,
    RESET,   ROL,     ROR,     ROXL,    ROXR,    RTE,     RTR,     RTS,
    SBCD,    SCC,     SCS,     SEQ,     SGE,     SGT,     SHI,     SLE,
    SLS,     SLT,     SMI,     SNE,     SPL,     SVC,     SVS,     SF,
    ST,      STOP,    SUB,     SUBA,    SUBI,    SUBQ,    SUBX,    SWAP,
    TAS,     TRAP,    TRAPV,   TST,
    UNLK
};

static const char *instrStr[] {
    "abcd" , "add",   "adda",  "addi",  "addq",  "addx",  "and",    "andi",
    "andi" , "andi",  "asl",   "asr",
    "bcc",   "bcs",   "beq",   "bge",   "bgt",   "bhi",   "ble",    "bls",
    "blt",   "bmi",   "bne",   "bpl",   "bvc",   "bvs",   "bchg",  "bclr",
    "bra",   "bset",  "bsr",   "btst",
    "chk",   "clr",   "cmp",   "cmpa",  "cmpi",  "cmpm",
    "dbcc",  "dbcs",  "dbeq",  "dbge",  "dbgt",  "dbhi",  "dble",   "dbls",
    "dblt",  "dbmi",  "dbne",  "dbpl",  "dbvc",  "dbvs",  "dbra",   "dbt",
    "divs",  "divu",
    "eor",   "eori",  "eori",  "eori",  "exg", "ext",
    "???",
    "jmp",   "jsr",
    "lea",   "link",  "lsl",   "lsr",
    "move" , "movea", "move",  "move",  "move",  "move",  "movem",  "movep",
    "moveq", "muls",  "mulu",
    "nbcd",  "neg",   "negx",  "nop",   "not",
    "or"   , "ori",   "ori",   "ori",
    "pea",
    "reset", "rol",   "ror",   "roxl",  "roxr",  "rte",   "rtr",   "rts",
    "sbcd",  "scc",   "scs",   "seq",   "sge",   "sgt",   "shi",   "sle",
    "sls",   "slt",   "smi",   "sne",   "spl",   "svc",   "svs",   "sf",
    "st",    "stop",  "sub",   "suba",  "subi",  "subq",  "subx",  "swap",
    "tas",   "trap",  "trapv", "tst",   "unlk"
};

enum Cond {
    CT, // Always true
    CF, // Always false
    HI, // Branch on higher than
    LS, // Branch on lower than or same
    CC, // Branch on carry clear
    CS, // Branch on carry set
    NE, // Branch on not equal
    EQ, // Branch on equal
    VC, // Branch on overflow clear
    VS, // Branch on overflow set
    PL, // Branch on plus (positive)
    MI, // Branch on minus (negative)
    GE, // Branch on greater than or equal
    LT, // Branch on less than
    GT, // Branch on greater than
    LE  // Branch on less than or equal
};

static const char *condStr[] {
    "t ", "f ", "hi", "ls", "cc", "cs", "ne", "eq",
    "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le"
};

/* Adressing modes
 *
 *   0:        Dn : Data register direct
 *   1:        An : Address register direct
 *   2:      (An) : Address register indirect
 *   3:     (An)+ : Address register indirect with post increment
 *   4:     -(An) : Address register indirect with pre decrement
 *   5:    (d,An) : Address register indirect with displacement
 *   6: (d,An,Xi) : Address register indirect with displacement and indexing
 *   7:    ####.w : Absolute addressing short
 *   8:    ####.l : Absolute addressing long
 *   9:    (d,PC) : Program counter indirect with displacement
 *  10: (d,PC,Xi) : Program counter indirect with displacement and indexing
 */
typedef u8 Mode;

struct Reg {

    u32 value;

    template<Size S> u32 read() { return CLIP<S>(value); }
    template<Size S> void write(u32 v) { value = CLIP<S>(v); }

    Reg & operator=(const u32 & rhs) { value = rhs; return *this; }
};

}
#endif

