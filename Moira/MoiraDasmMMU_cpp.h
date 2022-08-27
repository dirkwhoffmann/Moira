// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

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
    addr -= 2;

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
