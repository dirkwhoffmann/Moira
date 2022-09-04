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
    auto ext = dasmRead<Word>(addr);
    addr -= 2;

    // PLOAD: 0010 00x0 000x xxxx
    if ((ext & 0xFDE0) == 0x2000) {

        dasmPLoad<PLOAD, M, Long>(str, addr, op);
        return;
    }

    // PFLUSH: 001x xx0x xxxx xxxx
    if ((ext & 0xE200) == 0x2000) {

        dasmPFlush<PFLUSH, M, Long>(str, addr, op);
        return;
    }

    // PTEST: 100x xxxx xxxx xxxx
    if ((ext & 0xE000) == 0x8000) {

        dasmPTest<PTEST, M, Long>(str, addr, op);
        return;
    }

    // PMOVE: 010x xxxx 0000 0000 | 0110 00x0 0000 0000 | 000x xxxx 0000 0000
    if ((ext & 0xE0FF) == 0x4000 || (ext & 0xFDFF) == 0x6000 || (ext & 0xE0FF) == 0x0000) {

        dasmPMove<PMOVE, M, S>(str, addr, op);
        return;
    }

    dasmIllegal<I, M, S>(str, addr, op);
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
Moira::dasmPFlush40(StrWriter &str, u32 &addr, u16 op)
{
    auto reg  = _____________xxx(op);
    auto mode = ___________xx___(op);

    switch (mode) {

        case 0: str << "pflushn" << tab << Op<MODE_AI>(reg, addr); break;
        case 1: str << "pflush" << tab << Op<MODE_AI>(reg, addr); break;
        case 2: str << "pflushan"; break;
        case 3: str << "pflusha"; break;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmPLoad(StrWriter &str, u32 &addr, u16 op)
{
    auto old = addr;
    auto ext = dasmRead<Word>(addr);
    auto ea  = Op <M,S> ( _____________xxx(op), addr );

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

    const char *pStr = "";

    switch (fmt) {

        case 0:

            // printf("dasmPMove0\n");
            pStr = mmuregs[preg];

            if (!(ext & 0x200)) {

                str << Ins<I>{} << suffix << tab;
                str << Op<M, Long>(reg, addr) << Sep{} << pStr;
                if (fmt == 3 && preg > 1) str << Int(nr);

            } else {

                str << Ins<I>{} << suffix << tab;
                if (fmt == 3 && preg > 1) str << Int(nr);
                str << pStr << Sep{} << Op<M, Long>(reg, addr);
            }
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

            if (!(ext & 0x200)) {

                str << Ins<I>{} << suffix << tab;
                str << Op<M, Long>(reg, addr) << Sep{} << pStr;
                if (fmt == 3 && preg > 1) str << Int(nr);

            } else {

                str << Ins<I>{} << suffix << tab;
                if (fmt == 3 && preg > 1) str << Int(nr);
                str << pStr << Sep{} << Op<M, Long>(reg, addr);
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

            if (!(ext & 0x200)) {

                str << Ins<I>{} << suffix << tab;
                str << Op<M, Word>(reg, addr) << Sep{} << pStr;
                if (fmt == 3 && preg > 1) str << Int(nr);

            } else {

                str << Ins<I>{} << suffix << tab;
                if (fmt == 3 && preg > 1) str << Int(nr);
                str << pStr << Sep{} << Op<M, Word>(reg, addr);
            }
            break;
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

template <Instr I, Mode M, Size S> void
Moira::dasmPTest40(StrWriter &str, u32 &addr, u16 op)
{
    auto reg = _____________xxx(op);
    auto rw  = __________x_____(op);

    str << Ins<I>{} << (rw ? "r" : "w") << tab;
    str << Op<MODE_AI>(reg, addr);
}
