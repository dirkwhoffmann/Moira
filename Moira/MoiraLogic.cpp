// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template<Size S, Mode M> u32
CPU::computeEA(u32 n, u32 dis, u32 idx) {

    u32 result;

    assert(n < 8);
    
    switch (M) {
            
        case 0: // Dn
            result = reg.d[n].read<S>();
            break;

        case 1: // An
            result = reg.a[n].read<S>();
            break;

        case 2: // (An)
            assert(false);
            return 0;

        case 3: // (An)+
            assert(false);
            return 0;

        case 4: // -(An)
            assert(false);
            return 0;

        case 5: // (d,An)
            assert(false);
            return 0;

        case 6: // (d,An,Xi)
            assert(false);
            return 0;

        case 7: // ABS.W
            result = CLIP<Word>(irc);
            readExtensionWord();
            break;

        case 8: // ABS.L
            result = irc;
            readExtensionWord();
            break;

        case 9: // (d,PC)
            assert(false);
            return 0;

        case 10: // (d,PC,Xi)
            assert(false);
            return 0;

        case 11: // Imm
            assert(false);
            return 0;

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


template<Size size, Instr instr> u32
CPU::shift(u32 cnt, u32 data) {
    
    assert(cnt > 0);
    
    bool c = 0;
    bool v = 0;
    
    switch(instr) {
            
        case ASL:
            
            for (int i = 0; i < cnt; i++) {
                c = MSBIT<size>(data);
                v |= data ^ (data << 1);
                data <<= 1;
            }
            sr.c = c;
            sr.v = NEG<size>(v);
            sr.z = ZERO<size>(data);
            sr.n = NEG<size>(data);
            if (cnt) sr.x = c; // IF IS OBSOLETE???
            
            return CLIP<size>(data);
            
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

