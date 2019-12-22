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

namespace moira {

// Flags
#define SKIP_LAST_READ 0b00000001

enum Size { Byte = 1, Word = 2, Long = 4 };

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
 *   7:  (####).w : Absolute addressing short
 *   8:  (####).l : Absolute addressing long
 *   9:    (d,PC) : Program counter indirect with displacement
 *  10: (d,PC,Xi) : Program counter indirect with displacement and indexing
 *  11:      #### : Immediate
 */
typedef u8 Mode;

inline bool isRegMode(Mode M) { return M == 0 || M == 1;  }
inline bool isAbsMode(Mode M) { return M == 7 || M == 8;  }
inline bool isIdxMode(Mode M) { return M == 6 || M == 10; }
inline bool isMemMode(Mode M) { return M >= 2 && M <= 10; }
inline bool isImmMode(Mode M) { return M == 11; }

}
#endif
