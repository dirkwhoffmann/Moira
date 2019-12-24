// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef MOIRA_TYPES_H
#define MOIRA_TYPES_H

namespace moira {

// Operand size
typedef enum
{
    Byte = 1,
    Word = 2,
    Long = 4
}
Size;

typedef enum
{
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
}
Instr;

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
typedef enum {

    MODE_DN,   //           Dn : Data register direct
    MODE_AN,   //           An : Address register direct
    MODE_AI,   //         (An) : Register indirect
    MODE_PI,   //        (An)+ : Postincrement register indirect
    MODE_PD,   //        -(An) : Predecrement register indirect
    MODE_DI,   //       (d,An) : Register indirect with displacement
    MODE_IX,   //    (d,An,Xi) : Indexed register indirect with displacement
    MODE_AW,   //     (####).w : Absolute addressing short
    MODE_AL,   //     (####).l : Absolute addressing long
    MODE_DIPC, //       (d,PC) : PC relative with displacement
    MODE_PCIX, //    (d,An,Xi) : Indexed PC relative with displacement
    MODE_IM,   //         #### : Immediate data addressing
    MODE_IP    //         ---- : Implied addressing
}
Mode;

inline bool isRegMode(Mode M) { return M == 0 || M == 1;  }
inline bool isAbsMode(Mode M) { return M == 7 || M == 8;  }
inline bool isIdxMode(Mode M) { return M == 6 || M == 10; }
inline bool isMemMode(Mode M) { return M >= 2 && M <= 10; }
inline bool isImmMode(Mode M) { return M == 11; }


}
#endif
