// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>
#include <optional>

namespace moira {

typedef int8_t             i8;
typedef int16_t            i16;
typedef int32_t            i32;
typedef long long          i64;
typedef uint8_t            u8;
typedef uint16_t           u16;
typedef uint32_t           u32;
typedef unsigned long long u64;

typedef enum
{
    M68000,             // Fully supported
    M68010,             // Experimentally supported
    M68020              // Experimentally supported
}
Core;

typedef enum
{
    DASM_MOIRA,         // Default disassembler style
    DASM_MUSASHI,       // Musashi style
    DASM_VDA68K_MOT,    // Vda68k style (Motorola)
    DASM_VDA68K_MIT     // Vda68k style (MIT)
}
DasmStyle;

typedef enum
{
    DASM_MIXED_CASE,    // Style is determined by the selected DasmStyle
    DASM_LOWER_CASE,    // Everything is printed in lowercase
    DASM_UPPER_CASE     // Everything is printed in uppercase
}
DasmLetterCase;

typedef struct
{
    const char *prefix; // Prefix for hexidecimal numbers
    u8 radix;           // 10 or 16
    bool upperCase;     // Lettercase for hexadecimal digits A...F
    bool plainZero;     // Determines if 0 is printed with or without prefix
}
DasmNumberFormat;

typedef enum
{
    ILLEGAL,            // Illegal instruction
    LINE_A,             // Unused instruction (line A)
    LINE_F,             // Unused instruction (line F)

    ABCD,               // Add decimal with extend
    ADD,                // Add binary
    ADDA,               // Add address
    ADDI,               // Add immediate
    ADDQ,               // Add quick
    ADDX,               // Add extended
    AND,                // AND logical
    ANDI,               // AND immediate
    ANDICCR,            // AND immediate to condition code register
    ANDISR,             // AND immediate to status register
    ASL,                // Arithmetic shift left
    ASR,                // Arithmetic shift right
    BCC,                // Branch on carry clear
    BCS,                // Branch on carry set
    BEQ,                // Branch on equal
    BGE,                // Branch on greater than or equal
    BGT,                // Branch on greater than
    BHI,                // Branch on higher than
    BLE,                // Branch on less than or equal
    BLS,                // Branch on lower than or same
    BLT,                // Branch on less than
    BMI,                // Branch on minus
    BNE,                // Branch on not equal
    BPL,                // Branch on plus
    BVC,                // Branch on overflow clear
    BVS,                // Branch on overflow set
    BFCHG,              // 68020+: Test bit field and change
    BFCLR,              // 68020+: Test bit field and clear
    BFEXTS,             // 68020+: Extract bit field signed
    BFEXTU,             // 68020+: Extract bit field unsigned
    BFFFO,              // 68020+: Find first one in bit field
    BFINS,              // 68020+: Insert bit field
    BFSET,              // 68020+: Set bit field
    BFTST,              // 68020+: Test bit field
    BCHG,               // Test a bit and change
    BCLR,               // Test a bit and clear
    BKPT,               // 68010+: Run breakbpoint cycle
    BRA,                // Branch always
    BSET,               // Test a bit and set
    BSR,                // Branch to subroutine
    BTST,               // Test a bit
    CALLM,              // 68020+: Call module
    CAS,                // 68020+: Compare and swap with operand
    CAS2,               // 68020+: Compare and swap with operand
    CHK,                // Check register against bounds
    CHK2,               // 68020+: Check register against bounds
    CLR,                // Clear an operand
    CMP,                // Compare
    CMP2,               // 68020+: Compare register against bounds
    CMPA,               // Compare address
    CMPI,               // Compare immediate
    CMPM,               // Compare memory with memory
    DBCC,               // Test, decrement, and branch on carry clear
    DBCS,               // Test, decrement, and branch on carry set
    DBEQ,               // Test, decrement, and branch on equal
    DBGE,               // Test, decrement, and branch on greater than or equal
    DBGT,               // Test, decrement, and branch on greater than
    DBHI,               // Test, decrement, and branch on higher than
    DBLE,               // Test, decrement, and branch on less than or equal
    DBLS,               // Test, decrement, and branch on lower than or same
    DBLT,               // Test, decrement, and branch on less than
    DBMI,               // Test, decrement, and branch on minus
    DBNE,               // Test, decrement, and branch on not equal
    DBPL,               // Test, decrement, and branch on on plus
    DBVC,               // Test, decrement, and branch on overflow clear
    DBVS,               // Test, decrement, and branch on overflow set
    DBF,                // Test, decrement, and branch on false (never)
    DBT,                // Test, decrement, and branch on true (always)
    DIVL,               // 68020+: Long divide
    DIVS,               // Signed divide
    DIVU,               // Unsigned divide
    EOR,                // Exclusive OR logical
    EORI,               // Exclusive OR immediate
    EORICCR,            // Exclusive OR immediate to condition code register
    EORISR,             // Exclusive OR immediate to status register
    EXG,                // Exchange registers
    EXT,                // Sign-extend a data register
    EXTB,               // 68020+: Sign-extend a data register
    cpBcc,              // 68020+: Coprocessor branch conditionally
    cpDBcc,             // 68020+: Test coprocessor condition and count
    cpGEN,              // 68020+: Coprocessor general function
    cpRESTORE,          // 68020+: Coprocessor restore function
    cpSAVE,             // 68020+: Save internal state of coprocessor
    cpScc,              // 68020+: Coprocessor set according to condition
    cpTRAPcc,           // 68020+: Trap on coprocessor condition
    JMP,                // Jump
    JSR,                // Jump to subroutine
    LEA,                // Load effective address
    LINK,               // Link and allocate
    LSL,                // Logical shift left
    LSR,                // Logical shift right
    MOVE,               // Copy data from source to destination
    MOVEA,              // Move address
    MOVEC,              // 68010+: Move Control Register
    MOVEFCCR,           // Copy data from condition code register to destination
    MOVETCCR,           // Copy data to condition code register from source
    MOVEFSR,            // Copy data from status register to destination
    MOVETSR,            // Copy data to status register from source
    MOVEUSP,            // Copy data to or from USP
    MOVEM,              // Move multiple registers
    MOVEP,              // Move peripheral data
    MOVEQ,              // Move quick
    MOVES,              // 68010+: Move address space
    MULL,               // 68020+: Long multiply
    MULS,               // Signed multiply
    MULU,               // Unsigned multiply
    NBCD,               // Negate decimal with sign extend
    NEG,                // Negate
    NEGX,               // Negate with extend
    NOP,                // No operation
    NOT,                // Logical complement
    OR,                 // OR logical
    ORI,                // OR immediate
    ORICCR,             // OR immediate to condition code register
    ORISR,              // OR immediate to status register
    PACK,               // 68020+: Pack BCD
    PEA,                // Push effective address
    RESET,              // Reset external devices
    ROL,                // Rotate left
    ROR,                // Rotate right
    ROXL,               // Rotate left with extend
    ROXR,               // Rotate righ with extend
    RTD,                // 68010+: Return and deallocate
    RTE,                // Return from exception
    RTM,                // 68020+: Return from module
    RTR,                // Return and restore condition codes
    RTS,                // Return from subroutine
    SBCD,               // Subtract decimal with extend
    SCC,                // Set on carry clear
    SCS,                // Set on carry set
    SEQ,                // Set on equal
    SGE,                // Set on greater than or equal
    SGT,                // Set on greater than
    SHI,                // Set on higher than
    SLE,                // Set on less than or equal
    SLS,                // Set on lower than or same
    SLT,                // Set on less than
    SMI,                // Set on minus
    SNE,                // Set on not equal
    SPL,                // Set on plus
    SVC,                // Set on overflow clear
    SVS,                // Set on overflow set
    SF,                 // Set on false (never set)
    ST,                 // Set on true (always set)
    STOP,               // Load status register and stop
    SUB,                // Subtract binary
    SUBA,               // Subtract address
    SUBI,               // Subtract immediate
    SUBQ,               // Subtract quick
    SUBX,               // Subtract extended
    SWAP,               // Swap register halves
    TAS,                // Test and set an operand
    TRAP,               // Trap
    TRAPV,              // Trap on overflow
    TRAPCC,             // 68020+: Trap on carry clear
    TRAPCS,             // 68020+: Trap on carry set
    TRAPEQ,             // 68020+: Trap on equal
    TRAPGE,             // 68020+: Trap on greater than or equal
    TRAPGT,             // 68020+: Trap on greater than
    TRAPHI,             // 68020+: Trap on higher than
    TRAPLE,             // 68020+: Trap on less than or equal
    TRAPLS,             // 68020+: Trap on lower than or same
    TRAPLT,             // 68020+: Trap on less than
    TRAPMI,             // 68020+: Trap on minus
    TRAPNE,             // 68020+: Trap on not equal
    TRAPPL,             // 68020+: Trap on on plus
    TRAPVC,             // 68020+: Trap on overflow clear
    TRAPVS,             // 68020+: Trap on overflow set
    TRAPF,              // 68020+: Trap on false (never)
    TRAPT,              // 68020+: Trap on true (always)
    TST,                // Test an operand
    UNLK,               // Unlink
    UNPK,               // 68020+: Unpack BCD

    ABCD_LOOP,
    ADD_LOOP,
    ADDA_LOOP,
    ADDX_LOOP,
    AND_LOOP,
    ASL_LOOP,
    ASR_LOOP,
    CLR_LOOP,
    CMP_LOOP,
    CMPA_LOOP,
    DBCC_LOOP,
    DBCS_LOOP,
    DBEQ_LOOP,
    DBGE_LOOP,
    DBGT_LOOP,
    DBHI_LOOP,
    DBLE_LOOP,
    DBLS_LOOP,
    DBLT_LOOP,
    DBMI_LOOP,
    DBNE_LOOP,
    DBPL_LOOP,
    DBVC_LOOP,
    DBVS_LOOP,
    DBF_LOOP,
    DBT_LOOP,
    EOR_LOOP,
    LSL_LOOP,
    LSR_LOOP,
    MOVE_LOOP,
    NBCD_LOOP,
    NEG_LOOP,
    NEGX_LOOP,
    NOT_LOOP,
    OR_LOOP,
    ROL_LOOP,
    ROR_LOOP,
    ROXL_LOOP,
    ROXR_LOOP,
    SBCD_LOOP,
    SUB_LOOP,
    SUBA_LOOP,
    SUBX_LOOP,
    TST_LOOP
}
Instr;

template <Instr I>
constexpr bool looping() { return I >= ABCD_LOOP; }

typedef enum
{
    MODE_DN,   //  0         Dn : Data register direct
    MODE_AN,   //  1         An : Address register direct
    MODE_AI,   //  2       (An) : Address register indirect
    MODE_PI,   //  3      (An)+ : Postincrement register indirect
    MODE_PD,   //  4      -(An) : Predecrement register indirect
    MODE_DI,   //  5     (d,An) : Register indirect with displacement
    MODE_IX,   //  6  (d,An,Xi) : Indexed register indirect with displacement
    MODE_AW,   //  7   (####).w : Absolute addressing short
    MODE_AL,   //  8   (####).l : Absolute addressing long
    MODE_DIPC, //  9     (d,PC) : PC relative with displacement
    MODE_IXPC, // 10  (d,PC,Xi) : Indexed PC relative with displacement
    MODE_IM,   // 11       #### : Immediate data addressing
    MODE_IP    // 12       ---- : Implied addressing
}
Mode;

constexpr bool isRegMode(Mode M) { return M == 0 || M == 1;  }
constexpr bool isAbsMode(Mode M) { return M == 7 || M == 8;  }
constexpr bool isIdxMode(Mode M) { return M == 6 || M == 10; }
constexpr bool isMemMode(Mode M) { return M >= 2 && M <= 10; }
constexpr bool isPrgMode(Mode M) { return M == 9 || M == 10; }
constexpr bool isDspMode(Mode M) { return M == 5 || M == 6 || M == 9 || M == 10; }
constexpr bool isImmMode(Mode M) { return M == 11; }

typedef enum
{
    Byte = 1,       // .b : Byte addressing
    Word = 2,       // .w : Word addressing
    Long = 4        // .l : Long word addressing
}
Size;

typedef enum
{
    COND_BT,        // Always true
    COND_BF,        // Always false
    COND_HI,        // Higher than
    COND_LS,        // Lower or same
    COND_CC,        // Carry clear
    COND_CS,        // Carry set
    COND_NE,        // Not equal
    COND_EQ,        // Equal
    COND_VC,        // Overflow clear
    COND_VS,        // Overflow set
    COND_PL,        // Plus
    COND_MI,        // Minus
    COND_GE,        // Greater or equal
    COND_LT,        // Less than
    COND_GT,        // Greater than
    COND_LE         // Less than
}
Cond;

typedef enum
{
    EXC_RESET               = 1,
    EXC_BUS_ERROR           = 2,
    EXC_ADDRESS_ERROR       = 3,
    EXC_ILLEGAL             = 4,
    EXC_DIVIDE_BY_ZERO      = 5,
    EXC_CHK                 = 6,
    EXC_TRAPV               = 7,
    EXC_PRIVILEGE_VIOLATION = 8,
    EXC_TRACE               = 9,
    EXC_LINEA               = 10,
    EXC_LINEF               = 11,
    EXC_FORMAT_ERROR        = 14,
    EXC_IRQ_UNINITIALIZED   = 15,
    EXC_IRQ_SPURIOUS        = 24,
    EXC_TRAP                = 32
}
ExceptionType;

typedef struct
{
    Instr I;
    Mode  M;
    Size  S;
}
InstrInfo;

typedef enum
{
    IRQ_AUTO,
    IRQ_USER,
    IRQ_SPURIOUS,
    IRQ_UNINITIALIZED
}
IrqMode;

typedef enum
{
    FC_USER_DATA       = 1,
    FC_USER_PROG       = 2,
    FC_SUPERVISOR_DATA = 5,
    FC_SUPERVISOR_PROG = 6
}
FunctionCode;

typedef enum
{
    FC_FROM_FCL,
    FC_FROM_SFC,
    FC_FROM_DFC
}
FCSource;

typedef enum
{
    MEM_DATA = 1,
    MEM_PROG = 2
}
MemSpace;

typedef struct
{
    u16 code;
    u32 addr;
    u16 ird;
    u16 sr;
    u32 pc;
}
AEStackFrame;

struct StatusRegister {

    bool t1;              // Trace flag
    bool t0;              // Trace flag         (68020 only)
    bool s;               // Supervisor flag
    bool m;               // Master flag        (68020 only)
    bool x;               // Extend flag
    bool n;               // Negative flag
    bool z;               // Zero flag
    bool v;               // Overflow flag
    bool c;               // Carry flag

    u8 ipl;               // Required Interrupt Priority Level
};

struct Registers {

    u32 pc;                 // Program counter
    u32 pc0;                // Beginning of the currently executed instruction
    StatusRegister sr;      // Status register

    union {
        struct {
            u32 d[8];       // D0, D1 ... D7
            u32 a[8];       // A0, A1 ... A7
        };
        struct {
            u32 r[16];      // D0, D1 ... D7, A0, A1 ... A7
        };
        struct {
            u32 _pad[15];
            u32 sp;         // Visible stack pointer (overlays a[7])
        };
    };

    u32 usp;                // User Stack Pointer
    u32 isp;                // Interrupt Stack Pointer
    u32 msp;                // Master Stack Pointer             (68020+)

    u8 ipl;                 // Polled Interrupt Priority Level

    u32 vbr;                // Vector Base Register             (68010+)
    u32 sfc;                // Source Function Code             (68010+)
    u32 dfc;                // Destination Function Code        (68010+)

    // Unemulated registers
    u32 cacr;               // Cache Control Register           (68020+)
    u32 caar;               // Cache Address Register           (68020+)
};

struct PrefetchQueue {    // http://pasti.fxatari.com/68kdocs/68kPrefetch.html

    u16 irc;              // The most recent word prefetched from memory
    u16 ird;              // The instruction currently being executed
};

/* Execution flags
 *
 * The Motorola 68000 is a well organized processor that utilizes the same
 * general execution scheme for many instructions. However, the schemes
 * slighty differ between instruction. To take care of the subtle differences,
 * some execution functions take an dditional 'flags' argument which allows to
 * alter their behavior. All flags are passed as a template parameter for
 * efficiency.
 */
 
typedef u64 Flags;

// Memory access flags
static const u64 REVERSE        (1 << 0);  // Reverse the long word access order
static const u64 SKIP_LAST_READ (1 << 1);  // Don't read the extension word

// Interrupt flags
static const u64 POLLIPL        (1 << 2);  // Poll the interrupt lines
                           
// Address error stack frame flags
static const u64 AE_WRITE       (1 << 3);  // Clear read flag in code word
static const u64 AE_PROG        (1 << 4);  // Set FC pins to program space
static const u64 AE_DATA        (1 << 5);  // Set FC pins to user space
static const u64 AE_INC_PC      (1 << 6);  // Increment PC by 2 in stack frame
static const u64 AE_DEC_PC      (1 << 7);  // Decrement PC by 2 in stack frame
static const u64 AE_INC_ADDR    (1 << 8);  // Increment ADDR by 2 in stack frame
static const u64 AE_DEC_ADDR    (1 << 9);  // Decrement ADDR by 2 in stack frame
static const u64 AE_SET_CB3     (1 << 10); // Set bit 3 in CODE segment

// Experimental
static const u64 IMPLICIT_DECR  (1 << 11); // Omit 2 cycle delay in -(An) mode

}
