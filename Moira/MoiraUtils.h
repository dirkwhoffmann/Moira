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

#define _____________xxx(opcode) (u16)((opcode >> 0)  & 0b111)
#define ____xxx_________(opcode) (u16)((opcode >> 9)  & 0b111)
#define xxxx____________(opcode) (u16)((opcode >> 12) & 0b1111)

using namespace Moira;

enum Size { Byte, Word, Long };

template<Size>   int  BYTES();
template<Size>   i32  SIGN (u64 data);
template<Size>   u32  CLIP (u64 data);
template<Size>   u32  CLEAR(u64 data);
template<Size>   bool MSBIT(u64 data);
template<Size>   bool CARRY(u64 data);
template<Size>   bool ZERO (u64 data);
template<Size S> u32  SUBS (u32 d1, u32 d2) { return CLEAR<S>(d1) | CLIP<S>(d2); }

enum Instr {
    ABCD,    ADD,     ADDA,    ADDI,    ADDQ,    ADDX,    AND,     ANDI,
    ANDICCR, ANDISR,  ASL,     ASR,     BCC,     BCHG,    BCLR,    BRA,
    BSET,    BSR,     BTST,    CHK,     CLR,     CMP,     CMPA,    CMPI,
    CMPM,    DBCC,    DIVS,    DIVU,    EOR,     EORI,    EORICCR, EORISR,
    EXG,     EXT,     ILLEGAL, JMP,     JSR,     LEA,     LINK,    LSL,
    LSR,
    MOVE,    MOVEA,   MOVECCR, MOVEFSR, MOVETSR, MOVEUSP, MOVEM,   MOVEP,
    MOVEQ,   MULS,    MULU,    NBCD,    NEG,     NEGX,    NOP,     NOT,
    OR,      ORI,     ORICCR,  ORISR,   PEA,     RESET,   ROL,     ROR,
    ROXL,    ROXR,    RTE,     RTR,     RTS,     SBCD,    SCC,     STOP,
    SUB,     SUBA,    SUBI,    SUBQ,    SUBX,    SWAP,    TAS,     TRAP,
    TRAPV,   TST,     UNLK
};

static const char *instrStr[] {
    "abcd",  "add",   "adda", "addi",  "addq",   "addx",  "and",   "andi",
    "andi",  "andi",  "asl",  "asr",   "bcc",    "bchg",  "bclr",  "bra",
    "bset",  "bsr",   "btst", "chk",   "clr",    "cmp",   "cmpa",  "cmpi",
    "cmpm",  "dbcc",  "divs", "divu",  "eor",    "eori",  "eori",  "eori",
    "exg",   "ext",   "",     "jmp",   "jsr",    "lea",   "link",  "lsl",
    "lsr",
    "move",  "movea", "move", "move",  "move",   "move",  "movem", "movep",
    "moveq", "muls",  "mulu", "nbcd",  "neg",    "negx",  "nop",   "not",
    "or",    "ori",   "ori",  "ori",   "pea",    "reset", "rol",   "ror",
    "roxl",  "roxr",  "rte",  "rtr",   "rts",    "sbcd",  "scc",   "stop",
    "sub",   "suba",  "subi", "subq",  "subx",   "swap",  "tas",   "trap",
    "trapv", "tst",   "unlk"
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

#endif
