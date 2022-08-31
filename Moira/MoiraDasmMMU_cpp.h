// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template <Instr I, Mode M, Size S> void
Moira::dasmPGen(StrWriter &str, u32 &addr, u16 op)
{
    //  PFLUSH: 001x xx0x xxxx xxxx
    //   PLOAD: 0010 00x0 000x xxxx
    // PVALID1: 0010 1000 0000 0000
    // PVALID2: 0010 1100 0000 0xxx
    //   PMOVE: 010x xxx0 0000 0000 (Format 1) ??
    //          011x xxx0 000x xx00 (Format 2) ??
    //          011x xxx0 0000 0000 (Format 3) ??
    //   PTEST: 100x xxxx xxxx xxxx
    // PFLUSHR: 1010 0000 0000 0000

    auto ext = dasmRead<Word>(addr);
    addr -= 2;

    // printf("dasmPGen(%x) ext = %x\n", op, ext);

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

    // PVALID
    if ((ext & 0xFFFF) == 0x2800 || (ext & 0xFFF8) == 0x2C00) {

        str << "TODO: dasmMgen PVALID";
        // dasmPValid<I, M, S>(str, addr, op);
        return;
    }

    // PTEST
    if ((ext & 0xE000) == 0x8000) {

        dasmPTest<PTEST, M, Long>(str, addr, op);
        return;
    }

    // PMOVE
    dasmPMove<PMOVE, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmPFlush(StrWriter &str, u32 &addr, u16 op)
{
    auto old = addr;
    auto ext = dasmRead<Word>(addr);

    // Catch illegal extension words (TODO: Clean this up)
    bool illegalMode =
    M == MODE_DN ||
    M == MODE_AN ||
    M == MODE_PI ||
    M == MODE_PD ||
    M == MODE_DIPC ||
    M == MODE_IXPC ||
    M == MODE_IM;

    if (str.style == DASM_GNU && (!isValidExt(I, op, ext) || illegalMode)) {

        addr = old;
        dasmIllegal<I, M, S>(str, addr, op);
        return;
    }

    if ((ext & 0xE200) == 0x2000) { // PFLUSH

        if (M == MODE_IP) {

            str << Ins<I>{} << tab << u8(ext & 0x1F) << Sep{} << u8((ext >> 5) & 0xF);
            str << Sep{} << "INVALID " << Int(op & 0x3f);

        } else {

            auto ea = Op <M,S> ( _____________xxx(op), addr );
            str << Ins<I>{} << tab << u8(ext & 0x1F) << Sep{} << u8((ext >> 5) & 0xF);
            str << Sep{} << ea;
        }
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmPLoad(StrWriter &str, u32 &addr, u16 op)
{
    assert(M != MODE_IP);

    auto ea = Op <M,S> ( _____________xxx(op), addr );

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

    auto old  = addr;
    auto ext  = dasmRead<Word>(addr);
    auto reg  = _____________xxx(op);
    auto fmt  = xxx_____________(ext);
    auto preg = ___xxx__________(ext);
    auto nr   = ___________xxx__(ext);

    // Catch illegal extension words
    if (str.style == DASM_GNU && !isValidExt(I, op, ext)) {

        addr = old;
        dasmIllegal<I, M, S>(str, addr, op);
        return;
    }
    
    const char *suffix = (ext & 0x100) ? "fd" : "";

    // printf("dasmPMove ext = %x\n", ext);

    const char *pStr = "";

    switch (fmt) {

        case 0:

            // printf("dasmPMove0\n");
            pStr = mmuregs[preg];
            break;

        case 2:

            // printf("dasmPMove2\n");
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

            // printf("dasmPMove3\n");
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

        str << Ins<I>{} << suffix << tab;

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
    auto old = addr;
    auto ext = dasmRead<Word>(addr);
    auto reg = _____________xxx(op);
    auto lev = ___xxx__________(ext);
    auto rw  = ______x_________(ext);
    auto a   = _______x________(ext);
    auto an  = ________xxx_____(ext);
    auto fc  = ___________xxxxx(ext);
    auto fc1 = ___________xx___(ext);
    auto fc2 = _____________xxx(ext);

    // Catch illegal extension words (TODO: Clean this up)
    bool illegalMode =
    M == MODE_DN ||
    M == MODE_AN ||
    M == MODE_PI ||
    M == MODE_PD ||
    M == MODE_DIPC ||
    M == MODE_IXPC ||
    M == MODE_IM;

    if (str.style == DASM_GNU && (!isValidExt(I, op, ext) || illegalMode)) {

        addr = old;
        dasmIllegal<I, M, S>(str, addr, op);
        return;
    }

    str << Ins<I>{} << (rw ? "r" : "w") << tab;

    // 10XXX — Function code is specified as bits XXX
    // 01DDD — Function code is specified as bits 2–0 of data register DDD
    // 00000 — Function code is specified as source function code register
    // 00001 — Function code is specified as destination function code register

    if (fc == 0b00000) {
        str << "sfc";
    } else if (fc == 0b0001) {
        str << "dfc";
    } else if (fc1 == 0b10) {
        str << Dn{fc2};
    } else {
        str << "code " << fc2;
    }

    str << Sep{} << Op<M>(reg, addr) << Sep{} << lev;
    if (a) { str << Sep{} << An{an}; }
}
