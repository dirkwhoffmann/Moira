// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

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
            u32 an     = readA(n);
            u32 offset = (n == 7 && S == Byte) ? 2 : BYTES<S>();

            result = an;
            writeA(n, an + offset);
            break;
        }
        case 4: // -(An)
        {
            u32 an     = readA(n);
            u32 offset = (n == 7 && S == Byte) ? 2 : BYTES<S>();

            result = an - offset;
            writeA(n, result);
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

    /*
     if (ea.calculated) return ea.address;
     u32 adr;

     switch(ea.mode) {
     case DataRegisterDirect:
     return Base::read<Size>( DataRegister{ea.reg} );

     case AddressRegisterDirect:
     return Base::read<Size>( AddressRegister{ea.reg} );

     case AddressRegisterIndirect:
     return Base::read( AddressRegister{ea.reg} );

     case AddressRegisterIndirectWithPostIncrement:
     return Base::read( AddressRegister{ea.reg} );

     case AddressRegisterIndirectWithPreDecrement:
     if(specialCase != NoCyclesPreDec) ctx->sync(2);
     adr = Base::read( AddressRegister{ea.reg} );
     adr -= bytes<Size>();
     if (Size == Byte && ea.reg == 7) adr -= 1;
     return adr;

     case AddressRegisterIndirectWithDisplacement:
     adr = Base::read( AddressRegister{ea.reg} ) + (i16)ctx->irc;
     if (specialCase != NoLastPrefetch) readExtensionWord();
     return adr;

     case AddressRegisterIndirectWithIndex: {
     adr = Base::read( AddressRegister{ea.reg} );
     d8Xn:
     if(specialCase != NoCyclesD8) ctx->sync(2);
     u8 displacement = ctx->irc & 0xff;
     u8 reg = (ctx->irc & 0x7000) >> 12;
     u32 dispReg = ctx->irc & 0x8000
     ? Base::read( AddressRegister{reg} )
     : Base::read( DataRegister{reg} );
     if ( !(ctx->irc & 0x800) ) dispReg = (i16)dispReg;
     adr += dispReg + (i8)displacement;

     if (specialCase != NoLastPrefetch) readExtensionWord();
     return adr;
     }
     case AbsoluteShort:
     adr = (i16)ctx->irc;
     if (specialCase != NoLastPrefetch) readExtensionWord();
     return adr;

     case AbsoluteLong:
     adr = ctx->irc << 16;
     readExtensionWord();
     adr |= ctx->irc;
     if (specialCase != NoLastPrefetch) readExtensionWord();
     return adr;

     case ProgramCounterIndirectWithDisplacement:
     adr = ctx->prefetchCounterLast + (i16)ctx->irc;
     if (specialCase != NoLastPrefetch) readExtensionWord();
     return adr;

     case ProgramCounterIndirectWithIndex:
     adr = ctx->prefetchCounterLast;
     goto d8Xn;

     case Immediate:
     if (Size == Byte) {
     adr = ctx->irc & 0xff;
     } else if (Size == Word) {
     adr = ctx->irc;
     } else {
     adr = ctx->irc << 16;
     readExtensionWord();
     adr |= ctx->irc;
     }
     readExtensionWord();
     return adr;
     }
     */

    return result;
}

template<Mode M, Size S> u32
CPU::readEA(u32 n, u32 dis, u32 idx)
{
    u32 addr = computeEA<M,S>(n, dis, idx);
    printf("readEA: M: %d addr: %x result: %x\n", M, addr, read<S>(addr));
    return read<S>(addr);
    
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

template<Size S, Instr I> u32
CPU::shift(u32 cnt, u32 data) {
    
    assert(cnt > 0);
    
    bool c = 0;
    bool v = 0;
    
    switch(I) {
            
        case ASL:
            
            for (int i = 0; i < cnt; i++) {
                c = MSBIT<S>(data);
                v |= data ^ (data << 1);
                data <<= 1;
            }
            sr.c = c;
            sr.v = NEG<S>(v);
            sr.z = ZERO<S>(data);
            sr.n = NEG<S>(data);
            if (cnt) sr.x = c; // IF IS OBSOLETE???
            
            return CLIP<S>(data);
            
        case ASR:    return 2; // asr<Size>( data, shift );
        case LSL:    return 3; // lsl<Size>( data, shift );
        case LSR:    return 4; // lsr<Size>( data, shift );
        case ROL:    return 5; // rol<Size>( data, shift );
        case ROR:    return 6; // ror<Size>( data, shift );
        case ROXL:   return 7; // roxl<Size>( data, shift );
        case ROXR:   return 8; // roxr<Size>( data, shift );
            
        default: assert(false);
    }
    
    return 0;
}

template<Size S> u32
CPU::add(u32 op1, u32 op2)
{
    u64 result = (u64)op1 + (u64)op2;

    // printf("add(%x,%x) = %llx\n",op1, op2, result);
    
    sr.c = NEG<S>((u32)(result >> 1));
    sr.v = NEG<S>((u32)((op1 ^ result) & (op2 ^ result)));
    sr.z = ZERO<S>((u32)result);
    sr.n = NEG<S>((u32)result);
    sr.x = sr.c;

    return (u32)result;
}
