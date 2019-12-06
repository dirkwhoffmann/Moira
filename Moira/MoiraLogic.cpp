// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

bool
CPU::addressError(u32 addr)
{
#ifdef MOIRA_EMULATE_ADDRESS_ERROR
    if (addr & 1) {
        execAddressError(addr);
        return true;
    }
#endif
    return false;
}

template<> u32
CPU::read<Byte>(u32 addr)
{
    // printf("read<Byte>(%x)\n", addr);
    return memory->moiraRead8(addr);
}

template<> u32
CPU::read<Word>(u32 addr)
{
    // printf("read<Word>(%x)\n", addr);
    /*
    if (addr & 1) {
        execAddressError(addr);
    }
    */
    return memory->moiraRead16(addr);
}

template<> u32
CPU::read<Long>(u32 addr)
{
    // printf("read<Long>(%x)\n", addr);
    return memory->moiraRead16(addr) << 16 | memory->moiraRead16(addr + 2);
}

template<> void
CPU::write<Byte>(u32 addr, u32 value)
{
    memory->moiraWrite8(addr & 0xFFFFFF, (u8)value);
}

template<> void
CPU::write<Word>(u32 addr, u32 value)
{
    memory->moiraWrite16(addr & 0xFFFFFF, (u16)value);
}

template<> void
CPU::write<Long>(u32 addr, u32 value)
{
    memory->moiraWrite16(addr & 0xFFFFFF, (u16)(value >> 16));
    memory->moiraWrite16((addr + 2) & 0xFFFFFF, (u16)value);
}

template<Mode M, Size S> u32
CPU::computeEA(u32 n, u32 dis, u32 idx) {

    u32 result;

    assert(n < 8);
    
    switch (M) {
            
        case 0: // Dn
        {
            assert(false);
            break;
        }
        case 1: // An
        {
            assert(false);
            break;
        }
        case 2: // (An)
        {
            result = readA(n);
            break;
        }
        case 3: // (An)+
        {
            u32 an = readA(n);
            bool addressError = (S != Byte) && (an & 1);
            u32 offset = (n == 7 && S == Byte) ? 2 : BYTES<S>();
            result = an;
            if (!addressError) writeA(n, an + offset);
            break;
        }
        case 4: // -(An)
        {
            u32 an = readA(n);
            bool addressError = (S != Byte) && (an & 1);
            u32 offset = (n == 7 && S == Byte) ? 2 : BYTES<S>();
            result = an - offset;
            if (!addressError) writeA(n, an - offset);
            break;
        }
        case 5: // (d,An)
        {
            u32 an = readA(n);
            i16  d = (i16)irc;

            result = d + an;
            readExtensionWord();
            break;
        }
        case 6: // (d,An,Xi)
        {
            i8   d = (i8)irc;
            i32 an = readA(n);
            i32 xi = readR((irc >> 12) & 0b1111);

            result = d + an + ((irc & 0x800) ? xi : (i16)xi);
            readExtensionWord();
            break;
        }
        case 7: // ABS.W
        {
            result = irc;
            readExtensionWord();
            break;
        }
        case 8: // ABS.L
        {
            result = irc << 16;
            readExtensionWord();
            result |= irc;
            readExtensionWord();
            break;
        }
        case 9: // (d,PC)
        {
            i16  d = (i16)irc;

            result = pc + d;
            readExtensionWord();
            break;
        }
        case 10: // (d,PC,Xi)
        {
            i8   d = (i8)irc;
            i32 xi = readR((irc >> 12) & 0b1111);

            result = d + pc + ((irc & 0x800) ? xi : (i16)xi);
            readExtensionWord();
            break;
        }
        case 11: // Imm
        {
            assert(false);
            return 0;
        }
        default:
            assert(false);
    }

    return result;
}

template<Size S> u32
CPU::readImm()
{
    u32 result;

    switch (S) {
        case Byte:
            // printf("readImm (Byte)\n");
            result = (u8)irc;
            readExtensionWord();
            break;
        case Word:
            result = irc;
            // printf("readImm (Word) irc = %x\n", irc);
            readExtensionWord();
            break;
        case Long:
            // printf("readImm (Long)\n");
            result = irc << 16;
            readExtensionWord();
            result |= irc;
            readExtensionWord();
            break;
    }

    return result;
}

template<Instr I, Size S> u32
CPU::shift(int cnt, u64 data) {

    switch(I) {
            
        case ASL:
        {
            bool carry = false;
            u32 changed = 0;
            for (int i = 0; i < cnt; i++) {
                carry = MSBIT<S>(data);
                u64 shifted = data << 1;
                changed |= data ^ shifted;
                data = shifted;
            }
            if (cnt) sr.x = carry;
            sr.c = carry;
            sr.v = MSBIT<S>(changed);
            break;
        }
        case ASR:
        {
            bool carry = false;
            u32 changed = 0;
            for (int i = 0; i < cnt; i++) {
                carry = data & 1;
                u64 shifted = SIGN<S>(data) >> 1;
                changed |= data ^ shifted;
                data = shifted;
            }
            if (cnt) sr.x = carry;
            sr.c = carry;
            sr.v = MSBIT<S>(changed);
            break;
        }
        case LSL:
        {
            bool carry = false;
            for (int i = 0; i < cnt; i++) {
                carry = MSBIT<S>(data);
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
                carry = MSBIT<S>(data);
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
                if(carry) data |= MSBIT<S>(data);
            }
            sr.c = carry;
            sr.v = 0;
            break;
        }
        case ROXL:
        {
            bool carry = false;
            for (int i = 0; i < cnt; i++) {
                bool extend = carry;
                carry = MSBIT<S>(data);
                data = data << 1 | extend;
            }

            sr.x = carry;
            sr.c = carry;
            sr.v = 0;
            break;
        }
        case ROXR:
        {
            bool carry = false;
            for (int i = 0; i < cnt; i++) {
                bool extend = carry;
                carry = data & 1;
                data >>= 1;
                if(extend) data |= MSBIT<S>(data);
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
    sr.n = MSBIT<S>(data);
    sr.z = ZERO <S>(data);
    return CLIP<S>(data);
}

template<Instr I, Size S> u32
CPU::arith(u32 op1, u32 op2)
{
    u64 result;

    switch(I) {

        case ADD:   // Numeric addition
        {
            result = (u64)op1 + (u64)op2;

            sr.x = sr.c = CARRY<S>(result);
            sr.v = MSBIT<S>((op1 ^ result) & (op2 ^ result));
            sr.z = ZERO<S>(result);
            break;
        }
        case ADDX:  // Numeric addition with X flag
        {
            result = (u64)op1 + (u64)op2 + (u64)sr.x;

            sr.x = sr.c = CARRY<S>(result);
            sr.v = MSBIT<S>((op1 ^ result) & (op2 ^ result));
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
            sr.n = MSBIT<Byte>(result);
            sr.v = ((tmp_result & 0x80) == 0) && ((result & 0x80) == 0x80);
            break;
        }
        case SUB:   // Numeric subtraction
        {
            result = (u64)op2 - (u64)op1;
            // printf("arith::SUB %x %x %llx \n", op1, op2, result);

            sr.x = sr.c = CARRY<S>(result);
            sr.v = MSBIT<S>((op1 ^ op2) & (op2 ^ result));
            sr.z = ZERO<S>(result);
            break;
        }
        case SUBX:  // Numeric subtraction with X flag
        {
            result = (u64)op2 - (u64)op1 - (u64)sr.x;

            sr.x = sr.c = CARRY<S>(result);
            sr.v = MSBIT<S>((op1 ^ op2) & (op2 ^ result));
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

            printf("SBCD result = %llx c: %d x: %d\n", result, sr.c, sr.x);
            if (CLIP<Byte>(result)) sr.z = 0;
            sr.n = MSBIT<Byte>(result);
            sr.v = ((tmp_result & 0x80) == 0x80) && ((result & 0x80) == 0);
            break;
        }
        case CMP:
        {
            result = (u64)op1 - (u64)op2;

            sr.c = CARRY<S>(result);
            sr.v = MSBIT<S>((op1 ^ op2) & (op2 ^ result));
            sr.z = ZERO<S>(result);
            break;
        }
        default:
        {
            assert(false);
        }
    }
    sr.n = MSBIT<S>(result);
    return (u32)result;
}

template<Instr I, Size S> u32
CPU::logic(u32 op1, u32 op2)
{
    u32 result;

    switch(I) {

        case AND:
        {
            result = op1 & op2;
            break;
        }
        case OR:
        {
            result = op1 | op2;
            break;
        }
        case EOR:
        {
            result = op1 ^ op2;
            break;
        }
        default:
        {
            assert(false);
        }
    }

    sr.c = 0;
    sr.v = 0;
    sr.n = MSBIT<S>(result);
    sr.z = ZERO<S>(result);

    return result;
}

template <Cond C> bool
CPU::check() {

    switch(C) {

        case CT: return true;
        case CF: return false;
        case HI: return !sr.c && !sr.z;
        case LS: return sr.c || sr.z;
        case CC: return !sr.c;
        case CS: return sr.c;
        case NE: return !sr.z;
        case EQ: return sr.z;
        case VC: return !sr.v;
        case VS: return sr.v;
        case PL: return !sr.n;
        case MI: return sr.n;
        case GE: return sr.n == sr.v;
        case LT: return sr.n != sr.v;
        case GT: return sr.n == sr.v && !sr.z;
        case LE: return sr.n != sr.v || sr.z;
    }
}
