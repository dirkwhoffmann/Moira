// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template<Instr I, Size S> u32
Moira::shift(int cnt, u64 data) {

    switch(I) {
            
        case ASL:
        {
            bool carry = false;
            u32 changed = 0;
            for (int i = 0; i < cnt; i++) {
                carry = NBIT<S>(data);
                u64 shifted = data << 1;
                changed |= data ^ shifted;
                data = shifted;
            }
            if (cnt) sr.x = carry;
            sr.c = carry;
            sr.v = NBIT<S>(changed);
            break;
        }
        case ASR:
        {
            bool carry = false;
            u32 changed = 0;
            for (int i = 0; i < cnt; i++) {
                carry = data & 1;
                u64 shifted = SEXT<S>(data) >> 1;
                changed |= data ^ shifted;
                data = shifted;
            }
            if (cnt) sr.x = carry;
            sr.c = carry;
            sr.v = NBIT<S>(changed);
            break;
        }
        case LSL:
        {
            bool carry = false;
            for (int i = 0; i < cnt; i++) {
                carry = NBIT<S>(data);
                data = data << 1;
            }
            if (cnt) sr.x = carry;
            sr.c = carry;
            sr.v = 0;
            break;
        }
        case LSR:
        {
            bool carry = false;
            for (int i = 0; i < cnt; i++) {
                carry = data & 1;
                data = data >> 1;
            }
            if (cnt) sr.x = carry;
            sr.c = carry;
            sr.v = 0;
            break;
        }
        case ROL:
        {
            bool carry = false;
            for (int i = 0; i < cnt; i++) {
                carry = NBIT<S>(data);
                data = data << 1 | carry;
            }
            sr.c = carry;
            sr.v = 0;
            break;
        }
        case ROR:
        {
            bool carry = false;
            for (int i = 0; i < cnt; i++) {
                carry = data & 1;
                data >>= 1;
                if (carry) data |= MSBIT<S>();
            }
            sr.c = carry;
            sr.v = 0;
            break;
        }
        case ROXL:
        {
            bool carry = sr.x;
            for (int i = 0; i < cnt; i++) {
                bool extend = carry;
                carry = NBIT<S>(data);
                data = data << 1 | extend;
            }

            sr.x = carry;
            sr.c = carry;
            sr.v = 0;
            break;
        }
        case ROXR:
        {
            bool carry = sr.x;
            for (int i = 0; i < cnt; i++) {
                bool extend = carry;
                carry = data & 1;
                data >>= 1;
                if (extend) data |= MSBIT<S>();
            }

            sr.x = carry;
            sr.c = carry;
            sr.v = 0;
            break;
        }
        default:
        {
            assert(false);
        }
    }
    sr.n = NBIT<S>(data);
    sr.z = ZERO<S>(data);
    return CLIP<S>(data);
}

template<Instr I, Size S> u32
Moira::arith(u32 op1, u32 op2)
{
    u64 result;

    switch(I) {

        case ADD:   // Numeric addition
        case ADDI:
        case ADDQ:
        {
            result = (u64)op1 + (u64)op2;

            sr.x = sr.c = CARRY<S>(result);
            sr.v = NBIT<S>((op1 ^ result) & (op2 ^ result));
            sr.z = ZERO<S>(result);
            break;
        }
        case ADDX:  // Numeric addition with X flag
        {
            result = (u64)op1 + (u64)op2 + (u64)sr.x;

            sr.x = sr.c = CARRY<S>(result);
            sr.v = NBIT<S>((op1 ^ result) & (op2 ^ result));
            if (CLIP<S>(result)) sr.z = 0;
            break;
        }
        case ABCD:  // BCD addition
        {
            // Extract nibbles
            u16 op1_hi = op1 & 0xF0, op1_lo = op1 & 0x0F;
            u16 op2_hi = op2 & 0xF0, op2_lo = op2 & 0x0F;

            // From portable68000
            u16 resLo = op1_lo + op2_lo + sr.x;
            u16 resHi = op1_hi + op2_hi;
            u64 tmp_result;
            result = tmp_result = resHi + resLo;
            if (resLo > 9) result += 6;
            sr.x = sr.c = (result & 0x3F0) > 0x90;
            if (sr.c) result += 0x60;
            if (CLIP<Byte>(result)) sr.z = 0;
            sr.n = NBIT<Byte>(result);
            sr.v = ((tmp_result & 0x80) == 0) && ((result & 0x80) == 0x80);
            break;
        }
        case SUB:   // Numeric subtraction
        case SUBI:
        case SUBQ:
        {
            result = (u64)op2 - (u64)op1;

            sr.x = sr.c = CARRY<S>(result);
            sr.v = NBIT<S>((op1 ^ op2) & (op2 ^ result));
            sr.z = ZERO<S>(result);
            break;
        }
        case SUBX:  // Numeric subtraction with X flag
        {
            result = (u64)op2 - (u64)op1 - (u64)sr.x;

            sr.x = sr.c = CARRY<S>(result);
            sr.v = NBIT<S>((op1 ^ op2) & (op2 ^ result));
            if (CLIP<S>(result)) sr.z = 0;
            break;
        }
        case SBCD:  // BCD subtraction
        {
            // Extract nibbles
            u16 op1_hi = op1 & 0xF0, op1_lo = op1 & 0x0F;
            u16 op2_hi = op2 & 0xF0, op2_lo = op2 & 0x0F;

            // From portable68000
            u16 resLo = op2_lo - op1_lo - sr.x;
            u16 resHi = op2_hi - op1_hi;
            u64 tmp_result;
            result = tmp_result = resHi + resLo;
            int bcd = 0;
            if (resLo & 0xf0) {
                bcd = 6;
                result -= 6;
            }
            if (((op2 - op1 - sr.x) & 0x100) > 0xff) result -= 0x60;
            sr.c = sr.x = ((op2 - op1 - bcd - sr.x) & 0x300) > 0xff;

            if (CLIP<Byte>(result)) sr.z = 0;
            sr.n = NBIT<Byte>(result);
            sr.v = ((tmp_result & 0x80) == 0x80) && ((result & 0x80) == 0);
            break;
        }
        case CMP:
        {
            result = (u64)op1 - (u64)op2;

            sr.c = CARRY<S>(result);
            sr.v = NBIT<S>((op1 ^ op2) & (op2 ^ result));
            sr.z = ZERO<S>(result);
            break;
        }
        default:
        {
            assert(false);
        }
    }
    sr.n = NBIT<S>(result);
    return (u32)result;
}

template<Instr I, Size S> u32
Moira::logic(u32 op)
{
    u32 result;

    switch(I) {

        case NOT:
        {
            result = ~op;
            sr.setNZVC<S>(result);
            break;
        }
        case NEG:
        {
            result = arith<SUB,S>(op, 0);
            break;
        }
        case NEGX:
        {
            result = arith<SUBX,S>(op, 0);
            break;
        }
        default:
        {
            assert(false);
        }
    }
    return result;
}

template<Instr I, Size S> u32
Moira::logic(u32 op1, u32 op2)
{
    u32 result;

    switch(I) {

        case AND: case ANDI: case ANDICCR: case ANDISR:
        {
            result = op1 & op2;
            break;
        }
        case OR: case ORI: case ORICCR: case ORISR:
        {
            result = op1 | op2;
            break;
        }
        case EOR: case EORI: case EORICCR: case EORISR:
        {
            result = op1 ^ op2;
            break;
        }
        default:
        {
            assert(false);
        }
    }
    sr.setNZVC<S>(result);
    return result;
}

template <Instr I> u32
Moira::bitop(u32 op, u8 bit)
{
    switch (I) {
        case BCHG:
        {
            sr.z = 1 ^ ((op >> bit) & 1);
            op ^= (1 << bit);
            break;
        }
        case BSET:
        {
            sr.z = 1 ^ ((op >> bit) & 1);
            op |= (1 << bit);
            break;
        }
        case BCLR:
        {
            sr.z = 1 ^ ((op >> bit) & 1);
            op &= ~(1 << bit);
            break;
        }
        case BTST:
        {
            sr.z = 1 ^ ((op >> bit) & 1);
            break;
        }
        default:
        {
            assert(false);
        }
    }
    return op;
}

template <Size S> void
Moira::cmp(u32 op1, u32 op2)
{
    u64 result = (u64)op2 - (u64)op1;

    sr.c = NBIT<S>(result >> 1);
    sr.v = NBIT<S>((op2 ^ op1) & (op2 ^ result));
    sr.z = ZERO<S>(result);
    sr.n = NBIT<S>(result);
}

template <Instr I> bool
Moira::bcond() {

    switch(I) {

        case BRA: case DBT:  case ST:  return true;
        case DBF: case SF:             return false;
        case BHI: case DBHI: case SHI: return !sr.c && !sr.z;
        case BLS: case DBLS: case SLS: return sr.c || sr.z;
        case BCC: case DBCC: case SCC: return !sr.c;
        case BCS: case DBCS: case SCS: return sr.c;
        case BNE: case DBNE: case SNE: return !sr.z;
        case BEQ: case DBEQ: case SEQ: return sr.z;
        case BVC: case DBVC: case SVC: return !sr.v;
        case BVS: case DBVS: case SVS: return sr.v;
        case BPL: case DBPL: case SPL: return !sr.n;
        case BMI: case DBMI: case SMI: return sr.n;
        case BGE: case DBGE: case SGE: return sr.n == sr.v;
        case BLT: case DBLT: case SLT: return sr.n != sr.v;
        case BGT: case DBGT: case SGT: return sr.n == sr.v && !sr.z;
        case BLE: case DBLE: case SLE: return sr.n != sr.v || sr.z;
    }
}
