// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template <Size S> u32
Moira::dasmRead(u32 &addr)
{
    switch (S) {

        case Byte:

            U32_INC(addr, 2);
            return read16Dasm(addr) & 0xFF;

        case Long:

            return dasmRead<Word>(addr) << 16 | dasmRead<Word>(addr);

        default:

            U32_INC(addr, 2);
            return read16Dasm(addr);
    }
}

template <Mode M, Size S> Ea<M,S>
Moira::Op(u16 reg, u32 &pc)
{
    Ea<M,S> result;
    result.reg = reg;
    result.pc = pc;
    
    // Read extension words
    switch (M)
    {
        case 5:  // (d,An)
        case 7:  // ABS.W
        case 9:  // (d,PC)
        {
            result.ext1 = dasmRead<Word>(pc);
            break;
        }
        case 8:  // ABS.L
        {
            result.ext1 = dasmRead<Word>(pc);
            result.ext1 = result.ext1 << 16 | dasmRead<Word>(pc);
            break;
        }
        case 6:  // (d,An,Xi)
        case 10: // (d,PC,Xi)
        {
            result.ext1 = dasmRead<Word>(pc);
            result.ext2 = 0;
            result.ext3 = 0;
            
            if (result.ext1 & 0x100) {
                
                result.dw = u8(baseDispWords((u16)result.ext1));
                result.ow = u8(outerDispWords((u16)result.ext1));
                
                // Compensate Musashi bug (?)
                if (style == DASM_MUSASHI && (result.ext1 & 0x47) >= 0x44) {
                    
                    result.ow = 0;
                }
                
                if (result.dw == 1) result.ext2 = (i16)dasmRead<Word>(pc);
                if (result.dw == 2) result.ext2 = (i32)dasmRead<Long>(pc);
                if (result.ow == 1) result.ext3 = (i16)dasmRead<Word>(pc);
                if (result.ow == 2) result.ext3 = (i32)dasmRead<Long>(pc);
            }
            break;
        }
        case 11: // Imm
        {
            result.ext1 = dasmRead<S>(pc);
            break;
        }
        default:
        {
            break;
        }
    }
    
    return result;
}

template <Instr I, Mode M, Size S> void
Moira::dasmIllegal(StrWriter &str, u32 &addr, u16 op)
{
    str << "dc.w " << UInt16{op} << "; ILLEGAL";
}

template <Instr I, Mode M, Size S> void
Moira::dasmLineA(StrWriter &str, u32 &addr, u16 op)
{
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << "linea";
            break;
            
        default:
            
            str << "dc.w " << tab << UInt16{op} << "; opcode 1010";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmLineF(StrWriter &str, u32 &addr, u16 op)
{
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << "linef";
            break;
            
        default:
            
            str << "dc.w " << tab << UInt16{op} << "; opcode 1111";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmShiftRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );
    auto src = Dn ( ____xxx_________(op) );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmShiftIm(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imd ( ____xxx_________(op) );
    auto dst = Dn  ( _____________xxx(op) );
    
    if (src.raw == 0) src.raw = 8;
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmShiftEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAbcdRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAbcdEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddEaRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddRgEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAdda(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddiRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << Sz<S>{} << tab << Ims<S>(src) << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddiEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << Ims<S>(src) << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddqDn(StrWriter &str, u32 &addr, u16 op)
{
    auto src = ____xxx_________(op);
    auto dst = _____________xxx(op);
    
    if (src == 0) src = 8;
    
    switch (str.style) {
            
        case DASM_MUSASHI:
            
            str << Ins<I>{} << Sz<S>{} << tab << Imd{src} << Sep{} << Dn{dst};
            break;
            
        default:
            
            str << Ins<I>{} << Sz<S>{} << tab << Ims<S>{src} << Sep{} << Dn{dst};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddqAn(StrWriter &str, u32 &addr, u16 op)
{
    auto src = ____xxx_________(op);
    auto dst = _____________xxx(op);
    
    if (src == 0) src = 8;
    
    switch (str.style) {
            
        case DASM_MUSASHI:
            
            str << Ins<I>{} << Sz<S>{} << tab << Imd{src} << Sep{} << An{dst};
            break;
            
        default:
            
            str << Ins<I>{} << Sz<S>{} << tab << Ims<S>{src} << Sep{} << An{dst};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddqEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src =          ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    if (src == 0) src = 8;
    
    switch (str.style) {
            
        case DASM_MUSASHI:
            
            str << Ins<I>{} << Sz<S>{} << tab << Imd{src} << Sep{} << dst;
            break;
            
        default:
            
            str << Ins<I>{} << Sz<S>{} << tab << Ims<S>{src} << Sep{} << dst;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddxRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddxEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndEaRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndRgEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndiRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = _____________xxx(op);
    
    if (style == DASM_MUSASHI) {
        str << Ins<I>{} << Sz<S>{} << tab << Imu{src} << Sep{} << Dn{dst};
    } else {
        str << Ins<I>{} << Sz<S>{} << tab << Ims<S>(src) << Sep{} << Dn{dst};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndiEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    if (style == DASM_MUSASHI) {
        str << Ins<I>{} << Sz<S>{} << tab << Imu{src} << Sep{} << dst;
    } else {
        str << Ins<I>{} << Sz<S>{} << tab << Ims<S>(src) << "," << dst;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndiccr(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    
    if (str.style == DASM_MUSASHI) {
        str << Ins<I>{} << tab << Imu{src} << ", CCR";
    } else {
        str << Ins<I>{} << Sz<S>{} << tab << Ims<S>(src) << ",ccr";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndisr(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    
    if (str.style == DASM_MUSASHI) {
        str << Ins<I>{} << tab << Imu{src} << ", SR";
    } else {
        str << Ins<I>{} << Sz<S>{} << tab << Ims<S>(src) << ",sr";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitFieldDn(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    auto spc = str.style == DASM_MOIRA || str.style == DASM_MUSASHI ? " " : "";
    
    str << Ins<I>{} << tab;
    
    if constexpr (I == BFINS) {
        str << Dn ( _xxx____________(ext) ) << Sep{};
    }
    
    str << dst;
    
    if (ext & 0x0800) {
        str << spc << "{" << Dn ( _______xxx______(ext) ) << ":";
    } else {
        str << spc << "{" << _____xxxxx______(ext) << ":";
    }
    
    if (ext & 0x0020) {
        str << Dn ( _____________xxx(ext) ) << "}";
    } else {
        auto width = ___________xxxxx(ext);
        str << (width ? width : 32) << "}";
    }
    
    if constexpr (I == BFEXTU || I == BFEXTS || I == BFFFO) {
        str << Sep{} << Dn ( _xxx____________(ext) );
    }
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitFieldEa(StrWriter &str, u32 &addr, u16 op)
{
    dasmBitFieldDn<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmBkpt(StrWriter &str, u32 &addr, u16 op)
{
    auto nr = _____________xxx(op);
    
    switch (str.style) {
            
        case DASM_MUSASHI:
            
            str << Ins<I>{} << tab << Imd(nr);
            break;
            
        default:
            
            str << Ins<I>{} << tab << Imu(nr);
    }
    
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmBsr(StrWriter &str, u32 &addr, u16 op)
{
    // TODO: Redirect to dasmBcc<I,M,S>(str, addr, op);
    
    u32 dst = addr;
    U32_INC(dst, 2);
    U32_INC(dst, S == Byte ? (i8)op : SEXT<S>(dasmRead<S>(addr)));
    
    switch (style) {
            
        case DASM_MUSASHI:
            
            if (S == Byte && (u8)op == 0xFF) {
                
                dasmIllegal<I, M, S>(str, addr, op);
                break;
            }
            
            str << Ins<I>{} << tab << UInt(dst) << Av<I, M, S>{};
            break;
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            if (S == Byte && (u8)op == 0xFF) {
                
                dasmBsr<I, M, Long>(str, addr, op);
                break;
            }
            
            str << Ins<I>{} << Sz<S>{} << tab << UInt(dst) << Av<I, M, S>{};
            break;
            
        default:
            
            str << Ins<I>{} << tab << UInt(dst) << Av<I, M, S>{};
            break;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmCallm(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<Byte>(addr);
    auto dst = Op<M, S>( _____________xxx(op), addr );
    
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << Ins<I>{} << tab << Ims<Byte>(src) << Sep{} << dst;
            break;
            
        default:
            
            str << Ins<I>{} << tab << Imu(src) << Sep{} << dst << Av<I, M, S>{};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmCas(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto dc  = Dn ( _____________xxx(ext) );
    auto du  = Dn ( _______xxx______(ext) );
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << dc << Sep{} << du << Sep{} << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCas2(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Long> (addr);
    auto dc1 = Dn ( (ext >> 16) & 0b111  );
    auto dc2 = Dn ( (ext >> 0)  & 0b111  );
    auto du1 = Dn ( (ext >> 22) & 0b111  );
    auto du2 = Dn ( (ext >> 6)  & 0b111  );
    auto rn1 = Rn ( (ext >> 28) & 0b1111 );
    auto rn2 = Rn ( (ext >> 12) & 0b1111 );
    
    str << Ins<I>{} << Sz<S>{} << tab;
    str << dc1 << ":" << dc2 << Sep{} << du1 << ":" << du2 << Sep{};
    
    if (str.style == DASM_VDA68K_MIT) {
        str << rn1 << "@:" << rn2 << "@";
    } else {
        str << "(" << rn1 << "):(" << rn2 << ")";
    }
    
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmChk(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmChkCmp2(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Rn       ( xxxx____________(ext)      );
    
    if (ext & 0x0800) {
        str << Ins<CHK2>{} << Sz<S>{} << tab << src << Sep{} << dst;
    } else {
        str << Ins<CMP2>{} << Sz<S>{} << tab << src << Sep{} << dst;
    }
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmClr(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmp(StrWriter &str, u32 &addr, u16 op)
{
    Ea<M,S> src = Op <M,S> ( _____________xxx(op), addr );
    Dn      dst = Dn       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpiRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << Sz<S>{} << tab << Ims<S>(src) << Sep{} << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpiEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << Ims<S>(src) << Sep{} << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpm(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpBcc(StrWriter &str, u32 &addr, u16 op)
{
    auto id   = ( ____xxx_________(op) );
    auto cnd  = ( __________xxxxxx(op) );

    if (id == 2 && str.style == DASM_MUSASHI) {

        // Mimic Musashi bug
        dasmLineF<I, M, S>(str, addr, op);
        return;
    }

    if (id == 0) {

        auto pc   = addr + 2;
        auto disp = dasmRead<S>(addr);
        // auto ext1 = dasmRead<Word>(addr);
        // auto ext2 = dasmRead<Word>(addr);

        pc += SEXT<S>(disp);

        str << "pb" << Mmucc{cnd & 0xF} << tab << UInt(pc);

    } else {

        auto pc   = addr + 2;
        auto ext1 = dasmRead<Word>(addr);
        auto disp = dasmRead<S>(addr);
        auto ext2 = dasmRead<Word>(addr);

        pc += SEXT<S>(disp);

        str << id << Ins<I>{} << Cpcc{cnd} << tab << Ims<Word>(ext2);
        str << "; " << UInt(pc) << " (extension = " << Int(ext1) << ") (2-3)";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpDbcc(StrWriter &str, u32 &addr, u16 op)
{
    auto pc   = addr + 2;
    auto ext1 = dasmRead<Word>(addr);
    auto ext2 = dasmRead<Word>(addr);

    auto dn   = ( _____________xxx(op)   );
    auto id   = ( ____xxx_________(op)   );
    auto cnd  = ( __________xxxxxx(ext1) );

    if (id == 0) {

        str << "pb" << Mmucc{cnd & 0xF} << tab << UInt(pc + ext2);

    } else {

        auto ext3 = dasmRead<Word>(addr);
        auto ext4 = dasmRead<Word>(addr);

        pc += i16(ext3);

        str << id << Ins<I>{} << Cpcc{cnd} << tab << Dn{dn} << "," << Ims<Word>(ext4);
        str << "; " << UInt(pc) << " (extension = " << Int(ext2) << ") (2-3)";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpGen(StrWriter &str, u32 &addr, u16 op)
{
    auto id = ( ____xxx_________(op) );

    if (id == 0 && hasMMU()) {

        switch ((op >> 3) & 0b111) {

            case 0b000: dasmMMU<I, Mode(0), S>(str, addr, op); break;
            case 0b001: dasmMMU<I, Mode(1), S>(str, addr, op); break;
            case 0b010: dasmMMU<I, Mode(2), S>(str, addr, op); break;
            case 0b011: dasmMMU<I, Mode(3), S>(str, addr, op); break;
            case 0b100: dasmMMU<I, Mode(4), S>(str, addr, op); break;
            case 0b101: dasmMMU<I, Mode(5), S>(str, addr, op); break;
            case 0b110: dasmMMU<I, Mode(6), S>(str, addr, op); break;

            default:

                switch(op & 0b111) {

                    case 0b000: dasmMMU<I, Mode(7), S>(str, addr, op); break;
                    case 0b001: dasmMMU<I, Mode(8), S>(str, addr, op); break;
                    case 0b010: dasmMMU<I, Mode(9), S>(str, addr, op); break;
                    case 0b011: dasmMMU<I, Mode(10), S>(str, addr, op); break;
                    case 0b100: dasmMMU<I, Mode(11), S>(str, addr, op); break;

                    default:

                        dasmMMU<I, Mode(12), S>(str, addr, op); break;
                }
        }
        return;
    }

    if (hasCPI()) {

        auto ext = Imu ( dasmRead<Long>(addr) );

        str << id << Ins<I>{} << tab << ext;
        str << Av<I, M, S>{};
        return;
    }

    dasmLineF<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpRestore(StrWriter &str, u32 &addr, u16 op)
{
    auto dn = ( _____________xxx(op) );
    auto id = ( ____xxx_________(op) );
    auto ea = Op <M,S> (dn, addr);

    if (id == 1) {

        str << "frestore" << tab << ea;

    } else {

        str << id << Ins<I>{} << " " << ea;
        str << Av<I, M, S>{};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpRestoreInvalid(StrWriter &str, u32 &addr, u16 op)
{
    dasmCpRestore<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpSave(StrWriter &str, u32 &addr, u16 op)
{
    auto dn = ( _____________xxx(op) );
    auto id = ( ____xxx_________(op) );
    auto ea = Op <M,S> (dn, addr);

    if (str.style == DASM_MUSASHI && (op & 0xffe0) == 0xf500) {

        // Mimic Musashi bug
        dasmLineF<I, M, S>(str, addr, op);
        return;
    }
    
    if (id == 1) {

        str << "fsave" << tab << ea;

    } else {

        str << id << Ins<I>{} << tab << ea;
        str << Av<I, M, S>{};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpScc(StrWriter &str, u32 &addr, u16 op)
{
    auto dn   = ( _____________xxx(op) );
    auto id   = ( ____xxx_________(op) );

    if (id == 2 && str.style == DASM_MUSASHI) {

        // Mimic Musashi bug
        dasmLineF<I, M, S>(str, addr, op);
        return;
    }

    if (id == 0) {

            str << "MMU 001 group";

    } else {

        auto ext1 = dasmRead<Word>(addr);
        auto cnd  = ( __________xxxxxx(ext1) );

        auto ext2 = dasmRead<Word>(addr);
        auto ea = Op<M, S>(dn, addr);

        str << id << Ins<I>{} << Cpcc{cnd} << tab << ea;
        str << "; (extension = " << Int(ext2) << ") (2-3)";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpTrapcc(StrWriter &str, u32 &addr, u16 op)
{
    auto ext1 = dasmRead<Word>(addr);
    auto id   = ( ____xxx_________(op)   );
    auto cnd  = ( __________xxxxxx(ext1) );

    if (id == 0) {

        auto ext2 = dasmRead<Word>(addr);

        switch (op & 0b111) {

            case 0b010:
            {
                auto ext = dasmRead<Word>(addr);
                str << id << Ins<I>{} << Cpcc{cnd} << Tab{9};
                str << Tab{10} << Imu(ext);
                str << "; (extension = " << Int(ext2) << ") (2-3)";
                break;
            }
            case 0b011:
            {
                auto ext = dasmRead<Long>(addr);
                str << id << Ins<I>{} << Cpcc{cnd} << Tab{9};
                str << Tab{10} << Imu(ext);
                str << "; (extension = " << Int(ext2) << ") (2-3)";
                break;
            }
            case 0b100:
            {
                // (void)dasmRead<Long>(addr);
                str << id << Ins<I>{} << Cpcc{cnd} << Tab{9};
                str << "; (extension = " << Int(ext2) << ") (2-3)";
                break;
            }
            default:

                addr -= 4;
                str << "MMU 001 group";
                break;
        }

    } else {

        auto ext2 = dasmRead<Word>(addr);

        switch (op & 0b111) {

            case 0b010:
            {
                auto ext = dasmRead<Word>(addr);
                str << id << Ins<I>{} << Cpcc{cnd} << Tab{9};
                str << Tab{10} << Imu(ext);
                str << "; (extension = " << Int(ext2) << ") (2-3)";
                break;
            }
            case 0b011:
            {
                auto ext = dasmRead<Long>(addr);
                str << id << Ins<I>{} << Cpcc{cnd} << Tab{9};
                str << Tab{10} << Imu(ext);
                str << "; (extension = " << Int(ext2) << ") (2-3)";
                break;
            }
            case 0b100:
            {
                // (void)dasmRead<Long>(addr);
                str << id << Ins<I>{} << Cpcc{cnd} << Tab{9};
                str << "; (extension = " << Int(ext2) << ") (2-3)";
                break;
            }
            default:

                addr -= 4;
                dasmLineF<I, M, S>(str, addr, op);
                break;
        }
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmBcc(StrWriter &str, u32 &addr, u16 op)
{
    u32 dst = addr;
    U32_INC(dst, 2);
    U32_INC(dst, S == Byte ? (i8)op : SEXT<S>(dasmRead<S>(addr)));
    
    switch (style) {
            
        case DASM_MUSASHI:
            
            if (S == Byte && (u8)op == 0xFF) {
                
                dasmIllegal<I, M, S>(str, addr, op);
                break;
            }
            
            str << Ins<I>{} << tab << UInt(dst) << Av<I, M, S>{};
            break;
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            if (S == Byte && (u8)op == 0xFF) {
                
                dasmBsr<I, M, Long>(str, addr, op);
                break;
            }
            
            str << Ins<I>{} << Sz<S>{} << tab << UInt(dst) << Av<I, M, S>{};
            break;
            
        default:
            
            str << Ins<I>{} << tab << UInt(dst) << Av<I, M, S>{};
            break;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmBra(StrWriter &str, u32 &addr, u16 op)
{
    dasmBcc<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitDxDy(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitDxEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitImDy(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << Ins<I>{} << tab << Ims<S>(src) << Sep{} << dst;
            break;
            
        default:
            
            str << Ins<I>{} << tab << Imu(src) << Sep{} << dst;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitImEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << Ins<I>{} << tab << Ims<S>(src) << Sep{} << dst;
            break;
            
        default:
            
            str << Ins<I>{} << tab << Imu(src) << Sep{} << dst;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmDbcc(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( _____________xxx(op) );
    auto dst = addr + 2;
    
    U32_INC(dst, (i16)dasmRead<Word>(addr));
    
    str << Ins<I>{} << tab << src << Sep{} << UInt(dst);
}

template <Instr I, Mode M, Size S> void
Moira::dasmExgDxDy(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( ____xxx_________(op) );
    auto dst = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmExgAxDy(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( ____xxx_________(op) );
    auto dst = An ( _____________xxx(op) );
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmExgAxAy(StrWriter &str, u32 &addr, u16 op)
{
    auto src = An ( ____xxx_________(op) );
    auto dst = An ( _____________xxx(op) );
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmExt(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << Sz<S>{} << tab << Dn{src};
}

template <Instr I, Mode M, Size S> void
Moira::dasmExtb(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << Sz<S>{} << tab << Dn{src};
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmJmp(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmJsr(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> (_____________xxx(op), addr);
    
    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmLea(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmLink(StrWriter &str, u32 &addr, u16 op)
{
    auto dsp = dasmRead<S>(addr);
    auto src = An ( _____________xxx(op) );
    
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << Ims<S>(dsp);
            break;
            
        default:
            
            str << Ins<I>{} << tab << src << Sep{} << Ims<S>(dsp) << Av<I, M, S>{};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove0(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove2(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_AI,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove3(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_PI,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove4(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_PD,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove5(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_DI,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove6(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_IX,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove7(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_AW,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove8(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_AL,S> ( ____xxx_________(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovea(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovecRcRx(StrWriter &str, u32 &addr, u16 op)
{
    auto arg = u16(dasmRead<Word>(addr));
    auto src = Cn(____xxxxxxxxxxxx(arg));
    auto dst = Rn(xxxx____________(arg));
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
    str << Av<I, M, S>{arg};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovecRxRc(StrWriter &str, u32 &addr, u16 op)
{
    auto arg = u16(dasmRead<Word>(addr));
    auto dst = Cn(____xxxxxxxxxxxx(arg));
    auto src = Rn(xxxx____________(arg));
    
    str << Ins<I>{} << tab << src << Sep{} << dst;
    str << Av<I, M, S>{arg};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovemEaRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = RegRegList ( (u16)dasmRead<Word>(addr)  );
    auto src = Op <M,S>   ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovemRgEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = RegRegList ( (u16)dasmRead<Word>(addr)  );
    auto dst = Op <M,S>   ( _____________xxx(op), addr );
    
    if constexpr (M == 4) { src.raw = REVERSE_16(src.raw); }
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovepDxEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn             ( ____xxx_________(op)       );
    auto dst = Op <MODE_DI,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
    
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovepEaDx(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <MODE_DI,S> ( _____________xxx(op), addr );
    auto dst = Dn             ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveq(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( ____xxx_________(op) );
    
    str << Ins<I>{} << tab << Ims<Byte>(op) << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoves(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = (u16)dasmRead<Word>(addr);
    auto ea = Op <M,S> ( _____________xxx(op), addr );
    auto rg = Rn ( xxxx____________(ext) );
    
    if (ext & 0x800) {      // Rg -> Ea
        str << Ins<I>{} << Sz<S>{} << tab << rg << Sep{} << ea;
    } else {                // Ea -> Rg
        str << Ins<I>{} << Sz<S>{} << tab << ea << Sep{} << rg;
    }
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromCcrRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << tab << Ccr{} << Sep{} << dst << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromCcrEa(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << tab << Ccr{} << Sep{} << dst << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveToCcr(StrWriter &str, u32 &addr, u16 op)
{
    auto src = _____________xxx(op);
    
    switch (str.style) {
            
        case DASM_MOIRA:
        case DASM_MUSASHI:
            
            str << Ins<I>{} << tab << Op<M, Byte>(src, addr) << Sep{} << Ccr{};
            break;
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << Ins<I>{} << tab << Op<M, S>(src, addr) << Sep{} << Ccr{};
            break;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromSrRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << tab << Sr{} << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromSrEa(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op<M, S>( _____________xxx(op), addr );
    
    str << Ins<I>{} << tab << Sr{} << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveToSr(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << tab << src << Sep{} << Sr{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveUspAn(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = An ( _____________xxx(op) );
    
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << Ins<I>{} << Sz<S>{} << tab << Usp{} << Sep{} << dst;
            break;
            
        default:
            
            str << Ins<I>{} << tab << Usp{} << Sep{} << dst;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveAnUsp(StrWriter &str, u32 &addr, u16 op)
{
    auto src = An ( _____________xxx(op) );
    
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << Usp{};
            break;
            
        default:
            
            str << Ins<I>{} << tab << src << Sep{} << Usp{};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmMuls(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMulu(StrWriter &str, u32 &addr, u16 op)
{
    dasmMuls<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmMull(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dl  = Dn       ( _xxx____________(ext)      );
    auto dh  = Dn       ( _____________xxx(ext)      );
    
    auto fill = str.style == DASM_VDA68K_MIT ? "," : ":";
    
    (ext & 1 << 11) ? str << Ins<MULS>{} : str << Ins<MULU>{};
    str << Sz<S>{} << tab << src << Sep{};
    (ext & 1 << 10) ? str << dh << fill << dl : str << dl;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmDivs(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );
    
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmDivu(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmDivl(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dl  = Dn       ( _xxx____________(ext)      );
    auto dh  = Dn       ( _____________xxx(ext)      );
    
    auto fill = str.style == DASM_VDA68K_MIT ? "," : ":";
    
    (ext & 1 << 11) ? str << Ins<DIVS>{} : str << Ins<DIVU>{};
    
    if (ext & 1 << 10) {
        str << Sz<S>{} << tab << src << Sep{} << dh << fill << dl;
    } else if (dl.raw == dh.raw) {
        str << Sz<S>{} << tab << src << Sep{} << dl;
    } else {
        str << "l" << Sz<S>{} << tab << src << Sep{} << dh << fill << dl;
    }
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmNbcdRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<NBCD>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmNbcdEa(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<NBCD>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmNop(StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmPackDn(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto rx = Op <M,S> ( _____________xxx(op), addr );
    auto ry = Op <M,S> ( ____xxx_________(op), addr );
    
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << Ins<I>{} << tab << rx << Sep{} << ry << Sep{} << Ims<S>(ext);
            break;
            
        default:
            
            str << Ins<I>{} << tab << rx << Sep{} << ry << Sep{} << Imu(ext);
            str << Av<I, M, S>{};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmPackPd(StrWriter &str, u32 &addr, u16 op)
{
    dasmPackDn<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmPea(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmReset(StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRtd(StrWriter &str, u32 &addr, u16 op)
{
    auto disp = dasmRead<Word>(addr);
    
    str << Ins<I>{} << tab << Ims<Word>(disp);
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRte(StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRtm(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Rn ( ____________xxxx(op) );
    
    str << Ins<I>{} << tab << src;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRtr(StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRts(StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmSccRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmSccEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmStop(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    
    str << Ins<I>{} << tab << Ims<S>(src);
}

template <Instr I, Mode M, Size S> void
Moira::dasmNegRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmNegEa(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmSwap(StrWriter &str, u32 &addr, u16 op)
{
    Dn reg = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << tab << reg;
}

template <Instr I, Mode M, Size S> void
Moira::dasmTasRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );
    
    str << Ins<I>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmTasEa(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmTrap(StrWriter &str, u32 &addr, u16 op)
{
    auto nr = Imu ( ____________xxxx(op) );
    
    str << Ins<I>{} << tab << nr;
}

template <Instr I, Mode M, Size S> void
Moira::dasmTrapv(StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmTrapcc(StrWriter &str, u32 &addr, u16 op)
{
    switch (str.style) {
            
        case DASM_MOIRA:
        case DASM_MUSASHI:
            
            switch (S) {
                    
                case Byte:
                    
                    str << Ins<I>{} << tab;
                    break;
                    
                case Word:
                case Long:
                    
                    auto ext = dasmRead<S>(addr);
                    str << Ins<I>{} << tab << Imu(ext);
                    break;
            }
            break;
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            switch (S) {
                    
                case Byte:
                    
                    str << Ins<I>{} << Sz<Long>{};
                    break;
                    
                case Word:
                case Long:
                    
                    auto ext = dasmRead<S>(addr);
                    str << Ins<I>{} << Sz<S>{} << tab << Ims<S>(ext);
                    break;
            }
            break;
    }
    
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmTst(StrWriter &str, u32 &addr, u16 op)
{
    auto ea = Op <M,S> ( _____________xxx(op), addr );
    
    str << Ins<I>{} << Sz<S>{} << tab << ea;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmUnlk(StrWriter &str, u32 &addr, u16 op)
{
    auto reg = An ( _____________xxx(op) );
    
    str << Ins<I>{} << tab << reg;
}

template <Instr I, Mode M, Size S> void
Moira::dasmUnpkDn(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto rx = Op <M,S> ( _____________xxx(op), addr );
    auto ry = Op <M,S> ( ____xxx_________(op), addr );
    
    switch (str.style) {
            
        case DASM_VDA68K_MOT:
        case DASM_VDA68K_MIT:
            
            str << Ins<I>{} << tab << rx << Sep{} << ry << Sep{} << Ims<S>(ext);
            break;
            
        default:
            
            str << Ins<I>{} << tab << rx << Sep{} << ry << Sep{} << Imu(ext);
            str << Av<I, M, S>{};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmUnpkPd(StrWriter &str, u32 &addr, u16 op)
{
    dasmUnpkDn<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmMMU(StrWriter &str, u32 &addr, u16 op)
{
    //  PFLUSH: 001x xx0x xxxx xxxx
    //   PLOAD: 0010 00x0 000x xxxx
    // PVALID1: 0010 1000 0000 0000
    // PVALID2: 0010 1100 0000 0xxx
    //   PMOVE: 010x xxx0 0000 0000 (Format 1)
    //          011x xxx0 000x xx00 (Format 2)
    //          011x xxx0 0000 0000 (Format 3)
    //   PTEST: 100x xxxx xxxx xxxx
    // PFLUSHR: 1010 0000 0000 0000

    auto ext = dasmRead<Word>(addr);

    // PLOAD
    if ((ext & 0xFDE0) == 0x2000) {

        dasmPLoad<PLOAD, M, Long>(str, addr, op);
        return;
    }

    // PFLUSH
    if ((ext & 0xE200) == 0x2000) {

        dasmPFlush<PFLUSH, M, Long>(str, addr, op);
        return;
    }

    // PFLUSHR
    if ((ext & 0xFFFF) == 0xA000) {

        dasmPFlushA<PFLUSHA, M, Long>(str, addr, op);
        return;
    }

    // PVALID
    if ((ext & 0xFFFF) == 0x2800 || (ext & 0xFFF8) == 0x2C00) {

        str << "TODO: dasmMMU PVALID";
        // dasmPValid<I, M, S>(str, addr, op);
        return;
    }

    // PTEST
    if ((ext & 0xE000) == 0x8000) {

        dasmPFlushA<PTEST, M, Long>(str, addr, op);
        return;

    }

    // PMOVE
    // if ((ext & 0xE000) == 0x0000 || (ext & 0xE000) == 0x6000 || (ext & 0xE000) == 0x4000) {
    {
        dasmPMove<PMOVE, M, S>(str, addr, op);
        return;

    }

    // str << "TODO: dasmMMU";
}

template <Instr I, Mode M, Size S> void
Moira::dasmPFlush(StrWriter &str, u32 &addr, u16 op)
{
    addr -= 2;
    auto ext = dasmRead<Word>(addr);

    if ((ext & 0xE200) == 0x2000) { // PFLUSH

        if (M == MODE_IP) {

            str << "pflushr" << tab << u8(ext & 0x1F) << Sep{} << u8((ext >> 5) & 0xF);
            str << Sep{} << "INVALID " << Int(op & 0x3f);

        } else {

            auto ea = Op <M,S> ( _____________xxx(op), addr );
            str << "pflushr" << tab << u8(ext & 0x1F) << Sep{} << u8((ext >> 5) & 0xF);
            str << Sep{} << ea;
        }

    } else if (ext == 0xa000) { // PFLUSHR

        if (M == MODE_IP) {

            str << "pflushr ??? IP" << tab;

        } else {

            auto ea = Op <M,S> ( _____________xxx(op), addr );
            str << "pflushr" << tab << ea;

        }

    } else {

        str << "pflushr ???";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmPFlushA(StrWriter &str, u32 &addr, u16 op)
{
    str << "TODO: dasmPFlushA";
}

template <Instr I, Mode M, Size S> void
Moira::dasmPLoad(StrWriter &str, u32 &addr, u16 op)
{
    assert(M != MODE_IP);
    
    auto ea = Op <M,S> ( _____________xxx(op), addr );

    addr -= 2;
    auto ext = dasmRead<Word>(addr);

    switch (str.style) {

        case DASM_MUSASHI:

            str << "pload" << tab;

            if (ext & 0x200) {

                str << Imd((ext >> 10) & 7) << Sep{} << ea;

            } else {

                str << ea << Sep{} << Imd((ext >> 10) & 7);
            }
            break;

        default:

            str << "pload" << ((ext & 0x200) ? "r" : "w") << tab;
            str << Fc(ext & 0b11111) << Sep{} << ea;
            break;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmPMove(StrWriter &str, u32 &addr, u16 op)
{
    const char *const mmuregs[8] =
    { "tc", "drp", "srp", "crp", "cal", "val", "sccr", "acr" };

    addr -= 2;
    auto ext = dasmRead<Word>(addr);
    const char *suffix = (ext & 0x100) ? "fd" : "";

    auto reg  = _____________xxx(op);
    auto fmt  = xxx_____________(ext);
    auto preg = ___xxx__________(ext);
    auto nr   = ___________xxx__(ext);

    const char *pStr = "";

    switch (fmt) {

        case 0:

            pStr = mmuregs[preg];
            break;

        case 2:

            switch (preg) {

                case 0:     pStr = "tc"; break;
                case 1:     pStr = "drp"; break;
                case 2:     pStr = "srp"; break;
                case 3:     pStr = "crp"; break;
                case 4:     pStr = "cal"; break;
                case 5:     pStr = "val"; break;
                case 6:     pStr = "scc"; break;
                case 7:     pStr = "ac"; break;
            }
            break;

        case 3:

            switch (preg) {

                case 0:     pStr = "mmusr"; break;
                case 1:     pStr = "pcsr"; break;
                case 4:     pStr = "bad"; break;
                case 5:     pStr = "bac"; break;
                default:    pStr = "???"; break;
            }
            break;

        default:

            pStr = "[unknown form]";
            suffix = "";
            break;
    }

    if (!(ext & 0x200)) {

        str << Ins<I>{} << suffix << tab;

        if constexpr (M == MODE_IP) {
            str << "[unknown form]" << Sep{} << "INVALID " << Int(op & 0x3f);
        } else {
            str << Op<M, S>(reg, addr) << Sep{} << pStr;
            if (fmt == 3 && preg > 1) str << Int(nr);
        }

    } else {

        str << Ins<I>{} << suffix;

        if (M == MODE_IP) {
            str << tab << "[unknown form]" << Sep{} << "INVALID " << Int(op & 0x3f);
        } else {
            if (fmt == 3 && preg > 1) str << Int(nr);
            str << tab << pStr << Sep{} << Op<M, S>(reg, addr);
        }
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmPTest(StrWriter &str, u32 &addr, u16 op)
{
    str << "TODO: dasmPTest";
}

template <Instr I, Mode M, Size S> void
Moira::dasmFpu(StrWriter &str, u32 &addr, u16 op)
{
    auto reg  = _____________xxx(op);
    
    // Ported from Musashi.
    // TODO: CLEANUP

    static const char float_data_format[8][3] =
    {
        ".l", ".s", ".x", ".p", ".w", ".d", ".b", ".p"
    };

    u32 w2, src, dst_reg;
    // LIMIT_CPU_TYPES(M68030_PLUS);

    w2 = dasmRead<Word>(addr);

    src = (w2 >> 10) & 0x7;
    dst_reg = (w2 >> 7) & 0x7;

    // special override for FMOVECR
    if ((((w2 >> 13) & 0x7) == 2) && (((w2>>10)&0x7) == 7)) {

        str << "fmovecr" << tab << Imd{w2 & 0x7f} << Sep{} << "fp" << dst_reg;
        // sprintf(g_dasm_str, "fmovecr   #$%0x, fp%d", (w2&0x7f), dst_reg);
        return;
    }

    const char *mnemonic = "";

    switch ((w2 >> 13) & 0x7) {

        case 0x0:
        case 0x2:
        {
            switch(w2 & 0x7f)
            {
                case 0x00:    mnemonic = "fmove"; break;
                case 0x01:    mnemonic = "fint"; break;
                case 0x02:    mnemonic = "fsinh"; break;
                case 0x03:    mnemonic = "fintrz"; break;
                case 0x04:    mnemonic = "fsqrt"; break;
                case 0x06:    mnemonic = "flognp1"; break;
                case 0x08:    mnemonic = "fetoxm1"; break;
                case 0x09:    mnemonic = "ftanh1"; break;
                case 0x0a:    mnemonic = "fatan"; break;
                case 0x0c:    mnemonic = "fasin"; break;
                case 0x0d:    mnemonic = "fatanh"; break;
                case 0x0e:    mnemonic = "fsin"; break;
                case 0x0f:    mnemonic = "ftan"; break;
                case 0x10:    mnemonic = "fetox"; break;
                case 0x11:    mnemonic = "ftwotox"; break;
                case 0x12:    mnemonic = "ftentox"; break;
                case 0x14:    mnemonic = "flogn"; break;
                case 0x15:    mnemonic = "flog10"; break;
                case 0x16:    mnemonic = "flog2"; break;
                case 0x18:    mnemonic = "fabs"; break;
                case 0x19:    mnemonic = "fcosh"; break;
                case 0x1a:    mnemonic = "fneg"; break;
                case 0x1c:    mnemonic = "facos"; break;
                case 0x1d:    mnemonic = "fcos"; break;
                case 0x1e:    mnemonic = "fgetexp"; break;
                case 0x1f:    mnemonic = "fgetman"; break;
                case 0x20:    mnemonic = "fdiv"; break;
                case 0x21:    mnemonic = "fmod"; break;
                case 0x22:    mnemonic = "fadd"; break;
                case 0x23:    mnemonic = "fmul"; break;
                case 0x24:    mnemonic = "fsgldiv"; break;
                case 0x25:    mnemonic = "frem"; break;
                case 0x26:    mnemonic = "fscale"; break;
                case 0x27:    mnemonic = "fsglmul"; break;
                case 0x28:    mnemonic = "fsub"; break;
                case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
                    mnemonic = "fsincos"; break;
                case 0x38:    mnemonic = "fcmp"; break;
                case 0x3a:    mnemonic = "ftst"; break;
                case 0x41:    mnemonic = "fssqrt"; break;
                case 0x45:    mnemonic = "fdsqrt"; break;
                case 0x58:    mnemonic = "fsabs"; break;
                case 0x5a:    mnemonic = "fsneg"; break;
                case 0x5c:    mnemonic = "fdabs"; break;
                case 0x5e:    mnemonic = "fdneg"; break;
                case 0x60:    mnemonic = "fsdiv"; break;
                case 0x62:    mnemonic = "fsadd"; break;
                case 0x63:    mnemonic = "fsmul"; break;
                case 0x64:    mnemonic = "fddiv"; break;
                case 0x66:    mnemonic = "fdadd"; break;
                case 0x67:    mnemonic = "fdmul"; break;
                case 0x68:    mnemonic = "fssub"; break;
                case 0x6c:    mnemonic = "fdsub"; break;

                default:    mnemonic = "FPU (?)"; break;
            }

            if (w2 & 0x4000) {


                str << mnemonic << float_data_format[src] << Op<M, Long>(reg, addr);

            } else {

                str << mnemonic << tab << "FP" << src << Sep{} << "FP" << src << dst_reg;
                // sprintf(g_dasm_str, "%s.x   FP%d, FP%d", mnemonic, src, dst_reg);
            }
            break;
        }

        case 0x3:
        {
            switch ((w2>>10)&7)
            {
                case 3:        // packed decimal w/fixed k-factor
                    str << "fmove" << float_data_format[(w2>>10)&7] << tab << "FP1" << dst_reg << Sep{} << Op<M, Long>(reg, addr) << "sext_7bit_int(w2&0x7f)";
                    // sprintf(g_dasm_str, "fmove%s   FP%d, %s {#%d}", float_data_format[(w2>>10)&7], dst_reg, get_ea_mode_str_32(g_cpu_ir), sext_7bit_int(w2&0x7f));
                    break;

                case 7:        // packed decimal w/dynamic k-factor (register)
                    str << "fmove" << float_data_format[(w2>>10)&7] << tab << "F" << dst_reg << Sep{} << Op<M, Long>(reg, addr) << "{" << Dn((w2>>4)&7) << "}";
                    // sprintf(g_dasm_str, "fmove%s   FP%d, %s {D%d}", float_data_format[(w2>>10)&7], dst_reg, get_ea_mode_str_32(g_cpu_ir), (w2>>4)&7);
                    break;

                default:
                    str << "fmove" << float_data_format[(w2>>10)&7] << tab << "FP3" << dst_reg << Sep{} << Op<M, Long>(reg, addr);
                    // sprintf(g_dasm_str, "fmove%s   FP%d, %s", float_data_format[(w2>>10)&7], dst_reg, get_ea_mode_str_32(g_cpu_ir));
                    break;
            }
            break;
        }

        case 0x4:    // ea to control
        {
            str << "fmovem.l" << tab << Op<M, Long>(reg, addr);
            if (w2 & 0x1000) str << "fpcr";
            if (w2 & 0x0800) str << "/fpsr";
            if (w2 & 0x0400) str << "/fpiar";
            /* sprintf(g_dasm_str, "fmovem.l   %s, ", get_ea_mode_str_32(g_cpu_ir));
            if (w2 & 0x1000) strcat(g_dasm_str, "fpcr");
            if (w2 & 0x0800) strcat(g_dasm_str, "/fpsr");
            if (w2 & 0x0400) strcat(g_dasm_str, "/fpiar");
            */
            break;
        }

        case 0x5:    // control to ea
        {
            str << "fmovem.l" << tab;
            if (w2 & 0x1000) str << "fpcr";
            if (w2 & 0x0800) str << "/fpsr";
            if (w2 & 0x0400) str << "/fpiar";
            str << Sep{} << Op<M, Long>(reg, addr);
            /*
            strcpy(g_dasm_str, "fmovem.l   ");
            if (w2 & 0x1000) strcat(g_dasm_str, "fpcr");
            if (w2 & 0x0800) strcat(g_dasm_str, "/fpsr");
            if (w2 & 0x0400) strcat(g_dasm_str, "/fpiar");
            strcat(g_dasm_str, ", ");
            strcat(g_dasm_str, get_ea_mode_str_32(g_cpu_ir));
            */
            break;
        }

        case 0x6:    // memory to FPU, list
        {
            if ((w2>>11) & 1)    // dynamic register list
            {
                str << "fmovem.x" << tab << Op<M, Long>(reg, addr) << Sep{} << Dn((w2>>4)&7);
                // sprintf(g_dasm_str, "fmovem.x   %s, D%d", get_ea_mode_str_32(g_cpu_ir), (w2>>4)&7);
            }
            else    // static register list
            {
                int i;

                str << "fmovem.x" << tab << Op<M, Long>(reg, addr) << Sep{};
                // sprintf(g_dasm_str, "fmovem.x   %s, ", get_ea_mode_str_32(g_cpu_ir));

                for (i = 0; i < 8; i++) {

                    if (w2 & (1<<i)) {

                        if ((w2>>12) & 1) {   // postincrement or control

                            str << "FP" << 7 - i;
                            // sprintf(temp, "FP%d ", 7-i);

                        } else {           // predecrement

                            str << "FP" << i;
                            // sprintf(temp, "FP%d ", i);
                        }
                        // strcat(g_dasm_str, temp);
                    }
                }
            }
            break;
        }

        case 0x7:    // FPU to memory, list
        {
            if ((w2>>11) & 1)    // dynamic register list
            {
                str << "fmovem.x" << tab << Dn((w2>>4)&7) << Sep{} << Op<M, Long>(reg, addr);
                // sprintf(g_dasm_str, "fmovem.x   D%d, %s", (w2>>4)&7, get_ea_mode_str_32(g_cpu_ir));
            }
            else    // static register list
            {
                int i;

                str << "fmovem.x" << tab;
                // sprintf(g_dasm_str, "fmovem.x   ");

                for (i = 0; i < 8; i++)
                {
                    if (w2 & (1<<i))
                    {
                        if ((w2>>12) & 1)    // postincrement or control
                        {
                            str << "FP" << 7 - i;
                            // sprintf(temp, "FP%d ", 7-i);
                        }
                        else            // predecrement
                        {
                            str << "FP" << i;
                            // sprintf(temp, "FP%d ", i);
                        }
                        // strcat(g_dasm_str, temp);
                    }
                }

                str << Sep{} << Op<M, Long>(reg, addr);
                // strcat(g_dasm_str, ", ");
                // strcat(g_dasm_str, get_ea_mode_str_32(g_cpu_ir));
            }
            break;
        }

        default:
        {
            str << "FPU (?)";
            // sprintf(g_dasm_str, "FPU (?) ");
            break;
        }
    }
}
