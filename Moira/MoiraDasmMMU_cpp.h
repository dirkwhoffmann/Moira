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
    auto old   = addr;
    auto ext   = dasmRead<Word>(addr);
    auto reg   = _____________xxx(op);
    auto mode  = ___xxx__________(ext);
    auto mask  = _______xxxx_____(ext);
    auto fc    = ___________xxxxx(ext);
    
    // Catch illegal extension words
    if (str.style == DASM_GNU && !isValidExt(I, M, op, ext)) {

        addr = old;
        dasmIllegal<I, M, S>(str, addr, op);
        return;
    }

    if (str.style == DASM_MOIRA_MOT || str.style == DASM_MOIRA_MIT) {

        // Only the MC68851 has four mask bits. The 68030 only has three.
        mask &= 0b111;
    }

    if (mode == 1) {

        str << "pflusha";
        return;
    }

    str << Ins<I>{} << tab;
    str << Fc{fc} << Sep{} << Imu{mask};
    if (mode == 6) str << Sep{} << Op<M>(reg, addr);
}

template <Instr I, Mode M, Size S> void
Moira::dasmPLoad(StrWriter &str, u32 &addr, u16 op)
{
    auto old = addr;
    auto ea  = Op <M,S> ( _____________xxx(op), addr );
    auto ext = dasmRead<Word>(addr);

    // Catch illegal extension words
    if (str.style == DASM_GNU && !isValidExt(I, M, op, ext)) {

        addr = old;
        dasmIllegal<I, M, S>(str, addr, op);
        return;
    }

    str << "pload" << ((ext & 0x200) ? "r" : "w") << tab;
    str << Fc(ext & 0b11111) << Sep{} << ea;
}

template <Instr I, Mode M, Size S> void
Moira::dasmPMove(StrWriter &str, u32 &addr, u16 op)
{
    const char *const mmuregs[8] =
    { "tc", "drp", "tt0", "tt1", "cal", "val", "sccr", "acr" };

    auto old  = addr;
    auto ext  = dasmRead<Word>(addr);
    auto reg  = _____________xxx(op);
    auto fmt  = xxx_____________(ext);
    auto preg = ___xxx__________(ext);
    auto nr   = ___________xxx__(ext);

    // Catch illegal extension words
    if (str.style == DASM_GNU && !isValidExt(I, M, op, ext)) {

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

                case 0:     pStr = "psr"; break;
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
            str << pStr << Sep{} << Op<M, S>(reg, addr);
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

    // Catch illegal extension words
    if (str.style == DASM_GNU && !isValidExt(I, M, op, ext)) {

        addr = old;
        dasmIllegal<I, M, S>(str, addr, op);
        return;
    }

    str << Ins<I>{} << (rw ? "r" : "w") << tab;
    str << Fc{fc} << Sep{} << Op<M>(reg, addr) << Sep{} << lev;
    if (a) { str << Sep{} << An{an}; }
}
