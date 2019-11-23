// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template<Size S, Mode M> uint32_t
Moira::computeEA(uint32_t addr, int x) {
    
    assert(x < 8);
    
    switch (M) {
            
        case 0: // (Dx)
            return reg.d[x].read<S>();
            
        default:
            assert(false);
    }

/*
 if (ea.calculated) return ea.address;
 uint32_t adr;
 
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
 adr = Base::read( AddressRegister{ea.reg} ) + (int16_t)ctx->irc;
 if (specialCase != NoLastPrefetch) readExtensionWord();
 return adr;
 
 case AddressRegisterIndirectWithIndex: {
 adr = Base::read( AddressRegister{ea.reg} );
 d8Xn:
 if(specialCase != NoCyclesD8) ctx->sync(2);
 uint8_t displacement = ctx->irc & 0xff;
 uint8_t reg = (ctx->irc & 0x7000) >> 12;
 uint32_t dispReg = ctx->irc & 0x8000
 ? Base::read( AddressRegister{reg} )
 : Base::read( DataRegister{reg} );
 if ( !(ctx->irc & 0x800) ) dispReg = (int16_t)dispReg;
 adr += dispReg + (int8_t)displacement;
 
 if (specialCase != NoLastPrefetch) readExtensionWord();
 return adr;
 }
 case AbsoluteShort:
 adr = (int16_t)ctx->irc;
 if (specialCase != NoLastPrefetch) readExtensionWord();
 return adr;
 
 case AbsoluteLong:
 adr = ctx->irc << 16;
 readExtensionWord();
 adr |= ctx->irc;
 if (specialCase != NoLastPrefetch) readExtensionWord();
 return adr;
 
 case ProgramCounterIndirectWithDisplacement:
 adr = ctx->prefetchCounterLast + (int16_t)ctx->irc;
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

return 42;
}


template<Size size, Instr instr> uint32_t
Moira::shift(uint32_t cnt, uint32_t data) {
    
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
            flags.c = c;
            flags.v = NEG<size>(v);
            flags.z = ZERO<size>(data);
            flags.n = NEG<size>(data);
            if (cnt) flags.x = c; // IF IS OBSOLETE???
            
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

