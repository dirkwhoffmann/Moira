// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template <Instr I, Mode M, Size S> void
Moira::dasmFgen(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead<Word>(addr);
    auto cod = xxx_____________(ext);
    auto cmd = _________xxxxxxx(ext);
    addr -= 2;

    if ((ext & 0xFC00) == 0x5C00) {

        dasmFmovecr<FMOVECR, M, S>(str, addr, op);
        return;
    }

    switch (cod) {

        case 0b010:
        case 0b000:

            switch (cmd) {

                case 0x00: dasmFmove<FMOVE, M, S>(str, addr, op); return;
                case 0x01: dasmFgeneric<FINT, M, S>(str, addr, op); return;
                case 0x02: dasmFgeneric<FSINH, M, S>(str, addr, op); return;
                case 0x03: dasmFgeneric<FINTRZ, M, S>(str, addr, op); return;
                case 0x04: dasmFgeneric<FSQRT, M, S>(str, addr, op); return;
                case 0x06: dasmFgeneric<FLOGNP1, M, S>(str, addr, op); return;
                case 0x08: dasmFgeneric<FETOXM1, M, S>(str, addr, op); return;
                case 0x09: dasmFgeneric<FTANH, M, S>(str, addr, op); return;
                case 0x0A: dasmFgeneric<FATAN, M, S>(str, addr, op); return;
                case 0x0C: dasmFgeneric<FASIN, M, S>(str, addr, op); return;
                case 0x0D: dasmFgeneric<FATANH, M, S>(str, addr, op); return;
                case 0x0E: dasmFgeneric<FSIN, M, S>(str, addr, op); return;
                case 0x0F: dasmFgeneric<FTAN, M, S>(str, addr, op); return;
                case 0x10: dasmFgeneric<FETOX, M, S>(str, addr, op); return;
                case 0x11: dasmFgeneric<FTWOTOX, M, S>(str, addr, op); return;
                case 0x12: dasmFgeneric<FTENTOX, M, S>(str, addr, op); return;
                case 0x14: dasmFgeneric<FLOGN, M, S>(str, addr, op); return;
                case 0x15: dasmFgeneric<FLOG10, M, S>(str, addr, op); return;
                case 0x16: dasmFgeneric<FLOG2, M, S>(str, addr, op); return;
                case 0x18: dasmFgeneric<FABS, M, S>(str, addr, op); return;
                case 0x19: dasmFgeneric<FCOSH, M, S>(str, addr, op); return;
                case 0x1A: dasmFgeneric<FNEG, M, S>(str, addr, op); return;
                case 0x1C: dasmFgeneric<FACOS, M, S>(str, addr, op); return;
                case 0x1D: dasmFgeneric<FCOS, M, S>(str, addr, op); return;
                case 0x1E: dasmFgeneric<FGETEXP, M, S>(str, addr, op); return;
                case 0x1F: dasmFgeneric<FGETMAN, M, S>(str, addr, op); return;
                case 0x20: dasmFgeneric<FDIV, M, S>(str, addr, op); return;
                case 0x21: dasmFgeneric<FMOD, M, S>(str, addr, op); return;
                case 0x22: dasmFgeneric<FADD, M, S>(str, addr, op); return;
                case 0x23: dasmFgeneric<FMUL, M, S>(str, addr, op); return;
                case 0x24: dasmFgeneric<FSGLDIV, M, S>(str, addr, op); return;
                case 0x25: dasmFgeneric<FREM, M, S>(str, addr, op); return;
                case 0x26: dasmFgeneric<FSCALE, M, S>(str, addr, op); return;
                case 0x27: dasmFgeneric<FSGLMUL, M, S>(str, addr, op); return;
                case 0x28: dasmFgeneric<FSUB, M, S>(str, addr, op); return;
                case 0x30: dasmFgeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x31: dasmFgeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x32: dasmFgeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x33: dasmFgeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x34: dasmFgeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x35: dasmFgeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x36: dasmFgeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x37: dasmFgeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x38: dasmFgeneric<FCMP, M, S>(str, addr, op); return;
                case 0x3A: dasmFgeneric<FTST, M, S>(str, addr, op); return;
                case 0x41: dasmFgeneric<FSSQRT, M, S>(str, addr, op); return;
                case 0x45: dasmFgeneric<FDSQRT, M, S>(str, addr, op); return;
                case 0x58: dasmFgeneric<FSABS, M, S>(str, addr, op); return;
                case 0x5A: dasmFgeneric<FSNEG, M, S>(str, addr, op); return;
                case 0x5C: dasmFgeneric<FDABS, M, S>(str, addr, op); return;
                case 0x5E: dasmFgeneric<FDNEG, M, S>(str, addr, op); return;
                case 0x60: dasmFgeneric<FSDIV, M, S>(str, addr, op); return;
                case 0x62: dasmFgeneric<FSADD, M, S>(str, addr, op); return;
                case 0x63: dasmFgeneric<FSMUL, M, S>(str, addr, op); return;
                case 0x64: dasmFgeneric<FDDIV, M, S>(str, addr, op); return;
                case 0x66: dasmFgeneric<FDADD, M, S>(str, addr, op); return;
                case 0x67: dasmFgeneric<FDMUL, M, S>(str, addr, op); return;
                case 0x68: dasmFgeneric<FSSUB, M, S>(str, addr, op); return;
                case 0x6C: dasmFgeneric<FDSUB, M, S>(str, addr, op); return;
            }
            break;

        case 0b011:

            dasmFmove<FMOVE, M, S>(str, addr, op);
            return;

        case 0b100:
        case 0b101:
        case 0b110:
        case 0b111:

            dasmFmovem<FMOVE, M, S>(str, addr, op);
            return;
    }

    dasmLineF<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmFbcc(StrWriter &str, u32 &addr, u16 op)
{
    auto cnd = __________xxxxxx (op);

    auto dst = addr + 2;
    U32_INC(dst, SEXT<S>(dasmRead<S>(addr)));

    str << "fb" << Cpcc{cnd} << tab << Ims<Word>(dst);
}

template <Instr I, Mode M, Size S> void
Moira::dasmFdbcc(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead(addr);
    auto src = _____________xxx (op);
    auto cnd = __________xxxxxx (ext);

    auto dst = addr + 2;
    U32_INC(dst, SEXT<S>(dasmRead<S>(addr)));

    str << "fdb" << Cpcc{cnd} << tab << Dn{src} << Sep{} << UInt(dst);
}

template <Instr I, Mode M, Size S> void
Moira::dasmFmove(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead(addr);
    auto reg = _____________xxx (op);
    auto cod = xxx_____________ (ext);
    auto src = ___xxx__________ (ext);
    auto dst = ______xxx_______ (ext);
    auto fac = _________xxxxxxx (ext);

    if (ext == 0) {

        str << Ins<FNOP>{};
        return;
    }

    switch (cod) {

        case 0b000:

            str << Ins<I>{} << Ffmt{src} << tab << Fp(src) << src << Sep{} << Fp(dst);
            break;

        case 0b010:

            str << Ins<I>{} << Ffmt{src} << tab << Op<M, Long>(reg, addr) << Sep{} << Fp(dst);
            break;

        case 0b011:

            switch (src) {

                case 0b011:

                    str << Ins<I>{} << Ffmt{src} << tab << Fp(dst) << Sep{} << Op<M, Long>(reg, addr);
                    str << " {" << Ims<Byte>(i8(fac << 1) >> 1) << "}";
                    break;

                case 0b111:

                    str << Ins<I>{} << Ffmt{3} << tab << Fp{dst} << Sep{} << Op<M, Long>(reg, addr);
                    str << " {" << Dn(fac >> 4) << "}";
                    break;

                default:

                    str << Ins<I>{} << Ffmt{src} << tab << Fp{dst} << Sep{} << Op<M, Long>(reg, addr);
                    break;
            }
            break;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmFmovecr(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead(addr);
    auto dst = ______xxx_______ (ext);
    auto ofs = _________xxxxxxx (ext);

    str << Ins<I>{} << Ffmt{2} << tab << Imu{ofs} << Sep{} << Fp{dst};
}

template <Instr I, Mode M, Size S> void
Moira::dasmFmovem(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead(addr);
    auto reg = _____________xxx (op);
    auto cod = xxx_____________ (ext);
    auto mod = ___xx___________ (ext);
    auto rrr = _________xxx____ (ext);

    const char *delim = "";

    switch (cod) {

        case 0b100: // Ea to Cntrl

            str << "1:";
            str << Ins<I>{} << Ffmt{0} << tab;
            str << Op<M, Long>(reg, addr) << Sep{};
            if ((ext & 0x1C00) == 0) { str << "{}"; }
            if (ext & 0x0400) { str << delim << "fpiar"; delim = "/"; }
            if (ext & 0x0800) { str << delim << "fpsr";  delim = "/"; }
            if (ext & 0x1000) { str << delim << "fpcr";  delim = "/"; }
            break;

        case 0b101: // Cntrl to Ea

            str << "2:";
            str << Ins<I>{} << Ffmt{0} << tab;
            if ((ext & 0x1C00) == 0) { str << "{}"; }
            if (ext & 0x0400) { str << delim << "fpiar"; delim = "/"; }
            if (ext & 0x0800) { str << delim << "fpsr";  delim = "/"; }
            if (ext & 0x1000) { str << delim << "fpcr";  delim = "/"; }
            str << Sep{} << Op<M, Long>(reg, addr);
            break;

        case 0b110: // Memory to FPU

            switch (mod) {

                case 0b00: // Static list, predecrement addressing

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Op<M, Long>(reg, addr) << Sep{};
                    str << FRegList(ext & 0xFF);
                    break;

                case 0b01: // Dynamic list, predecrement addressing

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Op<M, Long>(reg, addr) << Sep{};
                    str << Dn{rrr};
                    break;

                case 0b10: // Static list, postincrement addressing

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Op<M, Long>(reg, addr) << Sep{};
                    str << FRegList(REVERSE_8(ext & 0xFF));
                    break;

                case 0b11: // Dynamic list, postincrement addressing

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Op<MODE_PI, Long>(reg, addr) << Sep{};
                    str << Dn{rrr};
                    break;
            }
            break;

        case 0b111: // FPU to memory

            switch (mod) {

                case 0b00: // Static list, predecrement addressing

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << FRegList(ext & 0xFF) << Sep{};
                    str << Op<M, Long>(reg, addr);
                    break;

                case 0b01: // Dynamic list, predecrement addressing

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Dn{rrr} << Sep{};
                    str << Op<M, Long>(reg, addr);
                    break;

                case 0b10: // Static list, postincrement addressing

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << FRegList(REVERSE_8(ext & 0xFF)) << Sep{};
                    str << Op<M, Long>(reg, addr);
                    break;

                case 0b11: // Dynamic list, postincrement addressing

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Dn{rrr} << Sep{};
                    str << Op<M, Long>(reg, addr);
                    break;
            }
            break;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmFrestore(StrWriter &str, u32 &addr, u16 op)
{
    str << "dasmFrestore";
}

template <Instr I, Mode M, Size S> void
Moira::dasmFsave(StrWriter &str, u32 &addr, u16 op)
{
    str << "dasmFsave";
}

template <Instr I, Mode M, Size S> void
Moira::dasmFscc(StrWriter &str, u32 &addr, u16 op)
{
    str << "dasmFscc";
}

template <Instr I, Mode M, Size S> void
Moira::dasmFtrapcc(StrWriter &str, u32 &addr, u16 op)
{
    str << "dasmTrapcc";
}

template <Instr I, Mode M, Size S> void
Moira::dasmFgeneric(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead(addr);
    auto reg = _____________xxx (op);
    auto src = ___xxx__________ (ext);
    auto dst = ______xxx_______ (ext);

    if (ext & 0x4000) {
        str << Ins<I>{} << Ffmt{src} << tab << Op<M, Long>(reg, addr) << Sep{} << Fp{dst};
    } else {
        str << Ins<I>{} << Ffmt{2} << tab << Fp{src} << Sep{} << Fp{dst};
    }
}
