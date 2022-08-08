// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template <> u32
Moira::dasmRead<Byte>(u32 &addr)
{
    U32_INC(addr, 2);
    return read16Dasm(addr) & 0xFF;
}

template <> u32
Moira::dasmRead<Word>(u32 &addr)
{
    U32_INC(addr, 2);
    return read16Dasm(addr);
}

template <> u32
Moira::dasmRead<Long>(u32 &addr)
{
    u32 result = dasmRead<Word>(addr) << 16;
    result |= dasmRead<Word>(addr);
    return result;
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

                int dw = baseDispWords((u16)result.ext1);
                if (dw == 1) result.ext2 = dasmRead<Word>(pc);
                if (dw == 2) result.ext2 = dasmRead<Long>(pc);

                int ow = outerDispWords((u16)result.ext1);

                // Compensate Musashi bug (?)
                // if ((result.ext1 & 0x47) >= 0x44) ow = 0;

                if (ow == 1) result.ext3 = dasmRead<Word>(pc);
                if (ow == 2) result.ext3 = dasmRead<Long>(pc);
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
Moira::dasmIllegal(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << "dc.w " << UInt16{op} << "; ILLEGAL";
}

template <Instr I, Mode M, Size S> void
Moira::dasmLineA(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << "dc.w " << tab << UInt16{op} << "; opcode 1010";
}

template <Instr I, Mode M, Size S> void
Moira::dasmLineF(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << "dc.w " << tab << UInt16{op} << "; opcode 1111";
}

template <Instr I, Mode M, Size S> void
Moira::dasmShiftRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );
    auto src = Dn ( ____xxx_________(op) );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmShiftIm(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imd ( ____xxx_________(op) );
    auto dst = Dn  ( _____________xxx(op) );

    if (src.raw == 0) src.raw = 8;
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmShiftEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAbcdRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAbcdEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddEaRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddRgEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAdda(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddiRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims ( SEXT<S>(dasmRead<S>(addr)) );
    auto dst = Dn  ( _____________xxx(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddiEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims      ( SEXT<S>(dasmRead<S>(addr)) );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddqDn(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imd ( ____xxx_________(op) );
    auto dst = Dn  ( _____________xxx(op) );

    if (src.raw == 0) src.raw = 8;
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddqAn(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imd ( ____xxx_________(op) );
    auto dst = An  ( _____________xxx(op) );

    if (src.raw == 0) src.raw = 8;
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddqEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imd      ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    if (src.raw == 0) src.raw = 8;
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddxRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddxEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndEaRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndRgEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndiRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = _____________xxx(op);

    if (dasmStyle == DASM_MUSASHI) {
        str << Ins<I>{} << Sz<S>{} << tab << Imu{src} << Sep{} << Dn{dst};
    } else {
        str << Ins<I>{} << Sz<S>{} << tab << Ims{SEXT<S>(src)} << Sep{} << Dn{dst};
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndiEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    if (dasmStyle == DASM_MUSASHI) {
        str << Ins<I>{} << Sz<S>{} << tab << Imu{src} << Sep{} << dst;
    } else {
        str << Ins<I>{} << Sz<S>{} << tab << Ims{SEXT<S>(src)} << "," << dst;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndiccr(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);

    if (str.style == DASM_MUSASHI) {
        str << Ins<I>{} << tab << Imu{src} << ", CCR";
    } else {
        str << Ins<I>{} << Sz<S>{} << tab << Ims{SEXT<S>(src)} << ",ccr";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndisr(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);

    if (str.style == DASM_MUSASHI) {
        str << Ins<I>{} << tab << Imu{src} << ", SR";
    } else {
        str << Ins<I>{} << Sz<S>{} << tab << Ims{SEXT<S>(src)} << ",sr";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitFieldDn(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab;

    if constexpr (I == BFINS) {
        str << Dn ( _xxx____________(ext) ) << Sep{};
    }

    str << dst;

    if (ext & 0x0800) {
        str << " {" << Dn ( _______xxx______(ext) ) << ":";
    } else {
        str << " {" << _____xxxxx______(ext) << ":";
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
Moira::dasmBitFieldEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    dasmBitFieldDn<I, M, S>(style, str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmBkpt(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto nr = Imd ( _____________xxx(op) );

    str << Ins<I>{} << tab << nr;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmBsr(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    if constexpr (MIMIC_MUSASHI && S == Byte) {

        if ((u8)op == 0xFF) {

            dasmIllegal<I, M, S>(style, str, addr, op);
            return;
        }
    }

    u32 dst = addr;
    U32_INC(dst, 2);
    U32_INC(dst, S == Byte ? (i8)op : SEXT<S>(dasmRead<S>(addr)));

    str << Ins<I>{} << tab << UInt(dst);
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCallm(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims     ( dasmRead<Byte>(addr)       );
    auto dst = Op<M, S>( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src << Sep{} << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCas(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto dc  = Dn ( _____________xxx(ext) );
    auto du  = Dn ( _______xxx______(ext) );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << dc << Sep{} << du << Sep{} << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCas2(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
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
    str << "(" << rn1 << "):(" << rn2 << ")";
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmChk(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmChkCmp2(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
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
Moira::dasmClr(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmp(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    Ea<M,S> src = Op <M,S> ( _____________xxx(op), addr );
    Dn      dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpiRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims ( SEXT<S>(dasmRead<S>(addr)) );
    auto dst = Dn  ( _____________xxx(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpiEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims      ( SEXT<S>(dasmRead<S>(addr)) );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpm(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpBcc(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto pc   = addr + 2;
    auto ext1 = dasmRead<Word>(addr);
    auto disp = dasmRead<S>(addr);
    auto ext2 = dasmRead<Word>(addr);
    auto id   = ( ____xxx_________(op) );
    auto cnd  = ( __________xxxxxx(op) );

    pc += SEXT<S>(disp);

    str << id << Ins<I>{} << Cpcc{cnd} << tab << Ims{ i16(ext2) };
    str << "; " << UInt(pc) << " (extension = " << Int(ext1) << ") (2-3)";
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpDbcc(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto pc   = addr + 2;
    auto ext1 = dasmRead<Word>(addr);
    auto ext2 = dasmRead<Word>(addr);
    auto ext3 = dasmRead<Word>(addr);
    auto ext4 = dasmRead<Word>(addr);
    auto dn   = ( _____________xxx(op)   );
    auto id   = ( ____xxx_________(op)   );
    auto cnd  = ( __________xxxxxx(ext1) );

    pc += i16(ext3);

    str << id << Ins<I>{} << Cpcc{cnd} << tab << Dn{dn} << "," << Ims{i16(ext4)};
    str << "; " << UInt(pc) << " (extension = " << Int(ext2) << ") (2-3)";
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpGen(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext = Imu ( dasmRead<Long>(addr) );
    auto id =      ( ____xxx_________(op) );

    str << id << Ins<I>{} << tab << ext;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpRestore(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dn = ( _____________xxx(op) );
    auto id = ( ____xxx_________(op) );
    auto ea = Op <M,S> (dn, addr);

    str << id << Ins<I>{} << " " << ea;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpSave(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dn = ( _____________xxx(op) );
    auto id = ( ____xxx_________(op) );
    auto ea = Op <M,S> (dn, addr);

    str << id << Ins<I>{} << tab << ea;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpScc(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext1 = dasmRead<Word>(addr);
    auto ext2 = dasmRead<Word>(addr);
    auto dn   = ( _____________xxx(op)   );
    auto id   = ( ____xxx_________(op)   );
    auto cnd  = ( __________xxxxxx(ext1) );
    auto ea   = Op <M,S> (dn, addr);

    str << id << Ins<I>{} << Cpcc{cnd} << tab << ea;
    str << "; (extension = " << Int(ext2) << ") (2-3)";
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpTrapcc(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext1 = dasmRead<Word>(addr);
    auto ext2 = dasmRead<Word>(addr);
    auto id   = ( ____xxx_________(op)   );
    auto cnd  = ( __________xxxxxx(ext1) );

    str << id << Ins<I>{} << Cpcc{cnd} << Align{9};
    
    switch (op & 0b111) {

        case 0b010:
        {
            auto ext = dasmRead <Word> (addr);
            str << Align{10} << Imu(ext);
            break;
        }
        case 0b011:
        {
            auto ext = dasmRead <Long> (addr);
            str << Align{10} << Imu(ext);
            break;
        }
    }

    str << "; (extension = " << Int(ext2) << ") (2-3)";
}

template <Instr I, Mode M, Size S> void
Moira::dasmBcc(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    if constexpr (MIMIC_MUSASHI && S == Byte) {

        if ((u8)op == 0xFF) {

            dasmIllegal<I, M, S>(style, str, addr, op);
            return;
        }
    }

    u32 dst = addr;
    U32_INC(dst, 2);
    U32_INC(dst, S == Byte ? (i8)op : SEXT<S>(dasmRead<S>(addr)));

    str << Ins<I>{} << tab << UInt(dst);
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmBra(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    dasmBcc<I, M, S>(style, str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitDxDy(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitDxEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitImDy(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << "#" << UInt(src) << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitImEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << "#" << UInt(src) << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmDbcc(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( _____________xxx(op) );
    auto dst = addr + 2;

    U32_INC(dst, (i16)dasmRead<Word>(addr));

    str << Ins<I>{} << tab << src << Sep{} << UInt(dst);
}

template <Instr I, Mode M, Size S> void
Moira::dasmExgDxDy(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( ____xxx_________(op) );
    auto dst = Dn ( _____________xxx(op) );

    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmExgAxDy(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( ____xxx_________(op) );
    auto dst = An ( _____________xxx(op) );

    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmExgAxAy(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = An ( ____xxx_________(op) );
    auto dst = An ( _____________xxx(op) );

    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmExt(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( _____________xxx(op) );

    str << Ins<I>{} << Sz<S>{} << tab << Dn{src};
}

template <Instr I, Mode M, Size S> void
Moira::dasmExtb(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( _____________xxx(op) );

    str << Ins<I>{} << Sz<S>{} << tab << Dn{src};
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmJmp(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmJsr(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> (_____________xxx(op), addr);

    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmLea(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );

    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmLink(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = An  ( _____________xxx(op)           );
    auto dsp = Ims ( SEXT <S> (dasmRead <S> (addr)) );

    str << Ins<I>{} << tab << src << Sep{} << dsp;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove0(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove2(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_AI,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove3(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_PI,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove4(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_PD,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove5(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_DI,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove6(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_IX,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove7(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_AW,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove8(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_AL,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovea(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovecRcRx(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto arg = u16(dasmRead<Word>(addr));
    auto src = Cn(____xxxxxxxxxxxx(arg));
    auto dst = Rn(xxxx____________(arg));

    str << Ins<I>{} << tab << src << Sep{} << dst;
    str << Av<I, M, S>{arg};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovecRxRc(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto arg = u16(dasmRead<Word>(addr));
    auto dst = Cn(____xxxxxxxxxxxx(arg));
    auto src = Rn(xxxx____________(arg));

    str << Ins<I>{} << tab << src << Sep{} << dst;
    str << Av<I, M, S>{arg};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovemEaRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = RegRegList ( (u16)dasmRead<Word>(addr)  );
    auto src = Op <M,S>   ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovemRgEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = RegRegList ( (u16)dasmRead<Word>(addr)  );
    auto dst = Op <M,S>   ( _____________xxx(op), addr );

    if constexpr (M == 4) { src.raw = REVERSE_16(src.raw); }
    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovepDxEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn             ( ____xxx_________(op)       );
    auto dst = Op <MODE_DI,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;

}

template <Instr I, Mode M, Size S> void
Moira::dasmMovepEaDx(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <MODE_DI,S> ( _____________xxx(op), addr );
    auto dst = Dn             ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveq(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims ( (i8)op               );
    auto dst = Dn  ( ____xxx_________(op) );

    str << Ins<I>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoves(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext = (u16)dasmRead<Word>(addr);
    auto ea = Op <M,S> ( _____________xxx(op), addr );
    auto reg = Rn ( xxxx____________(ext) );

    if (ext & 0x800) {      // Rg -> Ea
        str << Ins<I>{} << Sz<S>{} << tab << reg << Sep{} << ea;
    } else {                // Ea -> Rg
        str << Ins<I>{} << Sz<S>{} << tab << ea << Sep{} << reg;
    }
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromCcrRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );

    str << Ins<I>{} << tab << "CCR, " << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromCcrEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << "CCR, " << dst;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveToCcr(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,Byte> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src << ", CCR";
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromSrRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );

    str << Ins<I>{} << tab << "SR, " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromSrEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << "SR, " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveToSr(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src << ", SR";
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveUspAn(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = An ( _____________xxx(op) );

    str << Ins<I>{} << tab << "USP, " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveAnUsp(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = An ( _____________xxx(op) );

    str << Ins<I>{} << tab << src << ", USP";
}

template <Instr I, Mode M, Size S> void
Moira::dasmMul(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMull(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dl  = Dn       ( _xxx____________(ext)      );
    auto dh  = Dn       ( _____________xxx(ext)      );

    (ext & 1 << 11) ? str << Ins<MULS>{} : str << Ins<MULU>{};
    str << Sz<S>{} << tab << src << Sep{};
    (ext & 1 << 10) ? str << dh << ":" << dl : str << dl;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmDiv(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << Sep{} << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmDivl(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dl  = Dn       ( _xxx____________(ext)      );
    auto dh  = Dn       ( _____________xxx(ext)      );

    (ext & 1 << 11) ? str << Ins<DIVS>{} : str << Ins<DIVU>{};

    if (ext & 1 << 10) {
        str << Sz<S>{} << tab << src << Sep{} << dh << ":" << dl;
    } else if (dl.raw == dh.raw) {
        str << Sz<S>{} << tab << src << Sep{} << dl;
    } else {
        str << "l" << Sz<S>{} << tab << src << Sep{} << dh << ":" << dl;
    }
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmNbcdRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<NBCD>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmNbcdEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<NBCD>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmNop(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmPackDn(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto rx = Op <M,S> ( _____________xxx(op), addr );
    auto ry = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << tab << rx << Sep{} << ry << Sep{} << Imu(ext);
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmPackPd(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    dasmPackDn<I, M, S>(style, str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmPea(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmReset(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRtd(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto disp = Ims ( i16(dasmRead<Word>(addr)) );

    str << Ins<I>{} << tab << disp;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRte(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRtm(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Rn ( ____________xxxx(op) );

    str << Ins<I>{} << tab << src;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRtr(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmRts(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmSccRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( _____________xxx(op) );

    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmSccEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmStop(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims ( SEXT<S>(dasmRead<S>(addr)) );

    str << Ins<I>{} << tab << src;
}

template <Instr I, Mode M, Size S> void
Moira::dasmNegRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmNegEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmSwap(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    Dn reg = Dn ( _____________xxx(op) );

    str << Ins<I>{} << tab << reg;
}

template <Instr I, Mode M, Size S> void
Moira::dasmTasRg(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );

    str << Ins<I>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmTasEa(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmTrap(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto nr = Imu ( ____________xxxx(op) );

    str << Ins<I>{} << tab << nr;
}

template <Instr I, Mode M, Size S> void
Moira::dasmTrapv(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmTrapcc(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    str << Ins<I>{} << tab;

    switch (op & 0b111) {

        case 0b010:
        {
            auto ext = dasmRead <Word> (addr);
            str << Imu(ext);
            break;
        }
        case 0b011:
        {
            auto ext = dasmRead <Long> (addr);
            str << Imu(ext);
            break;
        }
    }
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmTst(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ea = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << ea;
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmUnlk(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto reg = An ( _____________xxx(op) );

    str << Ins<I>{} << tab << reg;
}

template <Instr I, Mode M, Size S> void
Moira::dasmUnpkDn(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto rx = Op <M,S> ( _____________xxx(op), addr );
    auto ry = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << tab << rx << Sep{} << ry << Sep{} << Imu(ext);
    str << Av<I, M, S>{};
}

template <Instr I, Mode M, Size S> void
Moira::dasmUnpkPd(DasmStyle style, StrWriter &str, u32 &addr, u16 op)
{
    dasmUnpkDn<I, M, S>(style, str, addr, op);
}
