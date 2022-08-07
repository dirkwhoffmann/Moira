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
                if ((result.ext1 & 0x47) >= 0x44) ow = 0;

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

template <Instr I, Mode M, Size S> const char *
Moira::availability(u16 opcode, u16 ext)
{
    switch(I) {

        case BKPT:
        case MOVES:
        case MOVEFCCR:
        case RTD:

            return "; (1+)";

        case CMPI:

            return isPrgMode(M) ? "; (1+)" : "";

        case CALLM:
        case RTM:

            return "; (2)";

        case cpGEN:
        case cpRESTORE:
        case cpSAVE:
        case cpScc:
        case cpTRAPcc:
            
            return "; (2-3)";

        case BFCHG:
        case BFCLR:
        case BFEXTS:
        case BFEXTU:
        case BFFFO:
        case BFINS:
        case BFSET:
        case BFTST:
        case CAS:
        case CAS2:
        case CHK2:
        case CMP2:
        case DIVL:
        case EXTB:
        case MULL:
        case PACK:
        case TRAPCC:
        case TRAPCS:
        case TRAPEQ:
        case TRAPGE:
        case TRAPGT:
        case TRAPHI:
        case TRAPLE:
        case TRAPLS:
        case TRAPLT:
        case TRAPMI:
        case TRAPNE:
        case TRAPPL:
        case TRAPVC:
        case TRAPVS:
        case TRAPF:
        case TRAPT:
        case UNPK:
            
            return "; (2+)";

        case CHK:
        case LINK:
        case BRA:
        case BHI:
        case BLS:
        case BCC:
        case BCS:
        case BNE:
        case BEQ:
        case BVC:
        case BVS:
        case BPL:
        case BMI:
        case BGE:
        case BLT:
        case BGT:
        case BLE:
        case BSR:

            return S == Long ? "; (2+)" : "";

        case TST:
            return M == 1 || M >= 9 ? "; (2+)" : "";

        case MOVEC:

            switch (ext & 0x0FFF) {

                case 0x000:
                case 0x001:
                case 0x800:
                case 0x801: return "; (1+)";
                case 0x002:
                case 0x803:
                case 0x804: return "; (2+)";
                case 0x802: return "; (2,3)";
                case 0x003:
                case 0x004:
                case 0x005:
                case 0x006:
                case 0x007:
                case 0x805:
                case 0x806:
                case 0x807: return "; (4+)";

                default:    return "; (?)";
            }

        default:
            return "";
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmIllegal(StrWriter &str, u32 &addr, u16 op)
{
    str << "dc.w " << UInt16{op} << "; ILLEGAL";
}

template <Instr I, Mode M, Size S> void
Moira::dasmLineA(StrWriter &str, u32 &addr, u16 op)
{
    str << "dc.w " << tab << UInt16{op} << "; opcode 1010";
}

template <Instr I, Mode M, Size S> void
Moira::dasmLineF(StrWriter &str, u32 &addr, u16 op)
{
    str << "dc.w " << tab << UInt16{op} << "; opcode 1111";
}

template <Instr I, Mode M, Size S> void
Moira::dasmShiftRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );
    auto src = Dn ( ____xxx_________(op) );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmShiftIm(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imd ( ____xxx_________(op) );
    auto dst = Dn  ( _____________xxx(op) );

    if (src.raw == 0) src.raw = 8;
    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
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

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAbcdEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddEaRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddRgEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAdda(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddiRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims ( SEXT<S>(dasmRead<S>(addr)) );
    auto dst = Dn  ( _____________xxx(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddiEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims      ( SEXT<S>(dasmRead<S>(addr)) );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddqDn(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imd ( ____xxx_________(op) );
    auto dst = Dn  ( _____________xxx(op) );

    if (src.raw == 0) src.raw = 8;
    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddqAn(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imd ( ____xxx_________(op) );
    auto dst = An  ( _____________xxx(op) );

    if (src.raw == 0) src.raw = 8;
    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddqEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imd      ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    if (src.raw == 0) src.raw = 8;
    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddxRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAddxEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndEaRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndRgEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndiRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imu ( dasmRead<S>(addr)    );
    auto dst = Dn  ( _____________xxx(op) );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndiEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imu      ( dasmRead<S>(addr)          );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndiccr(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imu ( dasmRead<S>(addr)         );

    str << Ins<I>{} << tab << src << ", CCR";
}

template <Instr I, Mode M, Size S> void
Moira::dasmAndisr(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Imu ( dasmRead<S>(addr) );

    str << Ins<I>{} << tab << src << ", SR";
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitFieldDn(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab;

    if constexpr (I == BFINS) {
        str << Dn ( _xxx____________(ext) ) << ", ";
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
        str << ", " << Dn ( _xxx____________(ext) );
    }

    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitFieldEa(StrWriter &str, u32 &addr, u16 op)
{
    dasmBitFieldDn<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmBkpt(StrWriter &str, u32 &addr, u16 op)
{
    auto nr = Imd ( _____________xxx(op) );

    str << Ins<I>{} << tab << nr << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmBsr(StrWriter &str, u32 &addr, u16 op)
{
    if constexpr (MIMIC_MUSASHI && S == Byte) {

        if ((u8)op == 0xFF) {

            dasmIllegal <I,M,S> (str, addr, op);
            return;
        }
    }

    u32 dst = addr;
    U32_INC(dst, 2);
    U32_INC(dst, S == Byte ? (i8)op : SEXT<S>(dasmRead<S>(addr)));

    str << Ins<I>{} << tab << UInt(dst);
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmCallm(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims      ( dasmRead <Byte> (addr)     );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src << ", " << dst;
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmCas(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto dc  = Dn ( _____________xxx(ext) );
    auto du  = Dn ( _______xxx______(ext) );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << dc << ", " << du << ", " << dst;
    str << availability <I, M, S> ();
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
    str << dc1 << ":" << dc2 << ", " << du1 << ":" << du2 << ", ";
    str << "(" << rn1 << "):(" << rn2 << ")";
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmChk(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmChkCmp2(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);

    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Rn       ( xxxx____________(ext)      );

    if (ext & 0x0800) {
        str << Ins<CHK2>{} << Sz<S>{} << tab << src << ", " << dst;
    } else {
        str << Ins<CMP2>{} << Sz<S>{} << tab << src << ", " << dst;
    }
    str << availability <I, M, S> ();
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

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpiRg(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims ( SEXT<S>(dasmRead<S>(addr)) );
    auto dst = Dn  ( _____________xxx(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpiEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims      ( SEXT<S>(dasmRead<S>(addr)) );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmCmpm(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Op <M,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpBcc(StrWriter &str, u32 &addr, u16 op)
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
Moira::dasmCpDbcc(StrWriter &str, u32 &addr, u16 op)
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
Moira::dasmCpGen(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = Imu ( dasmRead<Long>(addr) );
    auto id =      ( ____xxx_________(op) );

    str << id << Ins<I>{} << tab << ext;
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpRestore(StrWriter &str, u32 &addr, u16 op)
{
    auto dn = ( _____________xxx(op) );
    auto id = ( ____xxx_________(op) );
    auto ea = Op <M,S> (dn, addr);

    str << id << Ins<I>{} << " " << ea;
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpSave(StrWriter &str, u32 &addr, u16 op)
{
    auto dn = ( _____________xxx(op) );
    auto id = ( ____xxx_________(op) );
    auto ea = Op <M,S> (dn, addr);

    str << id << Ins<I>{} << tab << ea;
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmCpScc(StrWriter &str, u32 &addr, u16 op)
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
Moira::dasmCpTrapcc(StrWriter &str, u32 &addr, u16 op)
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
Moira::dasmBcc(StrWriter &str, u32 &addr, u16 op)
{
    if constexpr (MIMIC_MUSASHI && S == Byte) {

        if ((u8)op == 0xFF) {

            dasmIllegal <I,M,S> (str, addr, op);
            return;
        }
    }

    u32 dst = addr;
    U32_INC(dst, 2);
    U32_INC(dst, S == Byte ? (i8)op : SEXT<S>(dasmRead<S>(addr)));

    str << Ins<I>{} << tab << UInt(dst);
    str << availability <I, M, S> ();
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

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitDxEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn       ( ____xxx_________(op)       );
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitImDy(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << "#" << UInt(src) << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmBitImEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = dasmRead<S>(addr);
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << "#" << UInt(src) << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmDbcc(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( _____________xxx(op) );
    auto dst = addr + 2;

    U32_INC(dst, (i16)dasmRead<Word>(addr));

    str << Ins<I>{} << tab << src << ", " << UInt(dst);
}

template <Instr I, Mode M, Size S> void
Moira::dasmExgDxDy(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( ____xxx_________(op) );
    auto dst = Dn ( _____________xxx(op) );

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmExgAxDy(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn ( ____xxx_________(op) );
    auto dst = An ( _____________xxx(op) );

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmExgAxAy(StrWriter &str, u32 &addr, u16 op)
{
    auto src = An ( ____xxx_________(op) );
    auto dst = An ( _____________xxx(op) );

    str << Ins<I>{} << tab << src << ", " << dst;
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
    str << availability <I, M, S> ();
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

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmLink(StrWriter &str, u32 &addr, u16 op)
{
    auto src = An  ( _____________xxx(op)           );
    auto dsp = Ims ( SEXT <S> (dasmRead <S> (addr)) );

    str << Ins<I>{} << tab << src << ", " << dsp;
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove0(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove2(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_AI,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove3(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_PI,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove4(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_PD,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove5(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_DI,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove6(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_IX,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove7(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_AW,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMove8(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S>       ( _____________xxx(op), addr );
    auto dst = Op <MODE_AL,S> ( ____xxx_________(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovea(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = An       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovecRcRx(StrWriter &str, u32 &addr, u16 op)
{
    auto arg = u16(dasmRead<Word>(addr));
    auto src = Cn(____xxxxxxxxxxxx(arg));
    auto dst = Rn(xxxx____________(arg));

    str << Ins<I>{} << tab << src << ", " << dst;
    str << availability <I, M, S> (op, arg);
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovecRxRc(StrWriter &str, u32 &addr, u16 op)
{
    auto arg = u16(dasmRead<Word>(addr));
    auto dst = Cn(____xxxxxxxxxxxx(arg));
    auto src = Rn(xxxx____________(arg));

    str << Ins<I>{} << tab << src << ", " << dst;
    str << availability <I, M, S> (op, arg);
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovemEaRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = RegRegList ( (u16)dasmRead<Word>(addr)  );
    auto src = Op <M,S>   ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovemRgEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = RegRegList ( (u16)dasmRead<Word>(addr)  );
    auto dst = Op <M,S>   ( _____________xxx(op), addr );

    if constexpr (M == 4) { src.raw = REVERSE_16(src.raw); }
    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovepDxEa(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Dn             ( ____xxx_________(op)       );
    auto dst = Op <MODE_DI,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab;
    str << src << ", (" << UInt(dst.ext1) << "," << An{dst.reg} << ")";
}

template <Instr I, Mode M, Size S> void
Moira::dasmMovepEaDx(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <MODE_DI,S> ( _____________xxx(op), addr );
    auto dst = Dn             ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab;
    str << "(" << UInt(src.ext1) << "," << An{src.reg} << "), " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveq(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Ims ( (i8)op               );
    auto dst = Dn  ( ____xxx_________(op) );

    str << Ins<I>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoves(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = (u16)dasmRead<Word>(addr);
    auto ea = Op <M,S> ( _____________xxx(op), addr );
    auto reg = Rn ( xxxx____________(ext) );

    if (ext & 0x800) {      // Rg -> Ea
        str << Ins<I>{} << Sz<S>{} << tab << reg << ", " << ea;
    } else {                // Ea -> Rg
        str << Ins<I>{} << Sz<S>{} << tab << ea << ", " << reg;
    }
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromCcrRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );

    str << Ins<I>{} << tab << "CCR, " << dst << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromCcrEa(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << "CCR, " << dst << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveToCcr(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,Byte> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src << ", CCR";
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromSrRg(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Dn ( _____________xxx(op) );

    str << Ins<I>{} << tab << "SR, " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveFromSrEa(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << "SR, " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveToSr(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << tab << src << ", SR";
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveUspAn(StrWriter &str, u32 &addr, u16 op)
{
    auto dst = An ( _____________xxx(op) );

    str << Ins<I>{} << tab << "USP, " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMoveAnUsp(StrWriter &str, u32 &addr, u16 op)
{
    auto src = An ( _____________xxx(op) );

    str << Ins<I>{} << tab << src << ", USP";
}

template <Instr I, Mode M, Size S> void
Moira::dasmMul(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmMull(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dl  = Dn       ( _xxx____________(ext)      );
    auto dh  = Dn       ( _____________xxx(ext)      );

    (ext & 1 << 11) ? str << Ins<MULS>{} : str << Ins<MULU>{};
    str << Sz<S>{} << tab << src << ", ";
    (ext & 1 << 10) ? str << dh << ":" << dl : str << dl;
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmDiv(StrWriter &str, u32 &addr, u16 op)
{
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dst = Dn       ( ____xxx_________(op)       );

    str << Ins<I>{} << Sz<S>{} << tab << src << ", " << dst;
}

template <Instr I, Mode M, Size S> void
Moira::dasmDivl(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead <Word> (addr);
    auto src = Op <M,S> ( _____________xxx(op), addr );
    auto dl  = Dn       ( _xxx____________(ext)      );
    auto dh  = Dn       ( _____________xxx(ext)      );

    (ext & 1 << 11) ? str << Ins<DIVS>{} : str << Ins<DIVU>{};

    if (ext & 1 << 10) {
        str << Sz<S>{} << tab << src << ", " << dh << ":" << dl;
    } else if (dl.raw == dh.raw) {
        str << Sz<S>{} << tab << src << ", " << dl;
    } else {
        str << "l" << Sz<S>{} << tab << src << ", " << dh << ":" << dl;
    }
    
    str << availability <I, M, S> ();
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

    str << Ins<I>{} << tab << rx << ", " << ry << ", " << Imu(ext);
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmPackPd(StrWriter &str, u32 &addr, u16 op)
{
    dasmPackDn <I, M, S> (str, addr, op);
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
    auto disp = Ims ( i16(dasmRead<Word>(addr)) );

    str << Ins<I>{} << tab << disp;
    str << availability <I, M, S> ();
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

    str << Ins<I>{} << tab << src << availability <I, M, S> ();
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
    auto src = Ims ( SEXT<S>(dasmRead<S>(addr)) );

    str << Ins<I>{} << tab << src;
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

    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmTst(StrWriter &str, u32 &addr, u16 op)
{
    auto ea = Op <M,S> ( _____________xxx(op), addr );

    str << Ins<I>{} << Sz<S>{} << tab << ea;
    str << availability <I, M, S> ();
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

    str << Ins<I>{} << tab << rx << ", " << ry << ", " << Imu(ext);
    str << availability <I, M, S> ();
}

template <Instr I, Mode M, Size S> void
Moira::dasmUnpkPd(StrWriter &str, u32 &addr, u16 op)
{
    dasmUnpkDn<I, M, S>(str, addr, op);
}
