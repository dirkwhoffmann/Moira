// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template <Instr I, Mode M, Size S> void
Moira::dasmFGen(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead<Word>(addr);
    auto cod = xxx_____________(ext);
    auto cmd = _________xxxxxxx(ext);
    addr -= 2;

    if ((ext & 0xFC00) == 0x5C00) {

        dasmFMovecr<FMOVECR, M, S>(str, addr, op);
        return;
    }

    switch (cod) {

        case 0b010:
        case 0b000:

            switch (cmd) {

                case 0x00: dasmFMove<FMOVE, M, S>(str, addr, op); return;
                case 0x01: dasmFGeneric<FINT, M, S>(str, addr, op); return;
                case 0x02: dasmFGeneric<FSINH, M, S>(str, addr, op); return;
                case 0x03: dasmFGeneric<FINTRZ, M, S>(str, addr, op); return;
                case 0x04: dasmFGeneric<FSQRT, M, S>(str, addr, op); return;
                case 0x06: dasmFGeneric<FLOGNP1, M, S>(str, addr, op); return;
                case 0x08: dasmFGeneric<FETOXM1, M, S>(str, addr, op); return;
                case 0x09: dasmFGeneric<FTANH, M, S>(str, addr, op); return;
                case 0x0A: dasmFGeneric<FATAN, M, S>(str, addr, op); return;
                case 0x0C: dasmFGeneric<FASIN, M, S>(str, addr, op); return;
                case 0x0D: dasmFGeneric<FATANH, M, S>(str, addr, op); return;
                case 0x0E: dasmFGeneric<FSIN, M, S>(str, addr, op); return;
                case 0x0F: dasmFGeneric<FTAN, M, S>(str, addr, op); return;
                case 0x10: dasmFGeneric<FETOX, M, S>(str, addr, op); return;
                case 0x11: dasmFGeneric<FTWOTOX, M, S>(str, addr, op); return;
                case 0x12: dasmFGeneric<FTENTOX, M, S>(str, addr, op); return;
                case 0x14: dasmFGeneric<FLOGN, M, S>(str, addr, op); return;
                case 0x15: dasmFGeneric<FLOG10, M, S>(str, addr, op); return;
                case 0x16: dasmFGeneric<FLOG2, M, S>(str, addr, op); return;
                case 0x18: dasmFGeneric<FABS, M, S>(str, addr, op); return;
                case 0x19: dasmFGeneric<FCOSH, M, S>(str, addr, op); return;
                case 0x1A: dasmFGeneric<FNEG, M, S>(str, addr, op); return;
                case 0x1C: dasmFGeneric<FACOS, M, S>(str, addr, op); return;
                case 0x1D: dasmFGeneric<FCOS, M, S>(str, addr, op); return;
                case 0x1E: dasmFGeneric<FGETEXP, M, S>(str, addr, op); return;
                case 0x1F: dasmFGeneric<FGETMAN, M, S>(str, addr, op); return;
                case 0x20: dasmFGeneric<FDIV, M, S>(str, addr, op); return;
                case 0x21: dasmFGeneric<FMOD, M, S>(str, addr, op); return;
                case 0x22: dasmFGeneric<FADD, M, S>(str, addr, op); return;
                case 0x23: dasmFGeneric<FMUL, M, S>(str, addr, op); return;
                case 0x24: dasmFGeneric<FSGLDIV, M, S>(str, addr, op); return;
                case 0x25: dasmFGeneric<FREM, M, S>(str, addr, op); return;
                case 0x26: dasmFGeneric<FSCALE, M, S>(str, addr, op); return;
                case 0x27: dasmFGeneric<FSGLMUL, M, S>(str, addr, op); return;
                case 0x28: dasmFGeneric<FSUB, M, S>(str, addr, op); return;
                case 0x30: dasmFGeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x31: dasmFGeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x32: dasmFGeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x33: dasmFGeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x34: dasmFGeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x35: dasmFGeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x36: dasmFGeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x37: dasmFGeneric<FSINCOS, M, S>(str, addr, op); return;
                case 0x38: dasmFGeneric<FCMP, M, S>(str, addr, op); return;
                case 0x3A: dasmFGeneric<FTST, M, S>(str, addr, op); return;
                case 0x41: dasmFGeneric<FSSQRT, M, S>(str, addr, op); return;
                case 0x45: dasmFGeneric<FDSQRT, M, S>(str, addr, op); return;
                case 0x58: dasmFGeneric<FSABS, M, S>(str, addr, op); return;
                case 0x5A: dasmFGeneric<FSNEG, M, S>(str, addr, op); return;
                case 0x5C: dasmFGeneric<FDABS, M, S>(str, addr, op); return;
                case 0x5E: dasmFGeneric<FDNEG, M, S>(str, addr, op); return;
                case 0x60: dasmFGeneric<FSDIV, M, S>(str, addr, op); return;
                case 0x62: dasmFGeneric<FSADD, M, S>(str, addr, op); return;
                case 0x63: dasmFGeneric<FSMUL, M, S>(str, addr, op); return;
                case 0x64: dasmFGeneric<FDDIV, M, S>(str, addr, op); return;
                case 0x66: dasmFGeneric<FDADD, M, S>(str, addr, op); return;
                case 0x67: dasmFGeneric<FDMUL, M, S>(str, addr, op); return;
                case 0x68: dasmFGeneric<FSSUB, M, S>(str, addr, op); return;
                case 0x6C: dasmFGeneric<FDSUB, M, S>(str, addr, op); return;
            }
            break;

        case 0b011:

            dasmFMove<FMOVE, M, S>(str, addr, op);
            return;

        case 0b100:
        case 0b101:
        case 0b110:
        case 0b111:

            dasmFMovem<FMOVE, M, S>(str, addr, op);
            return;
    }

    dasmLineF<I, M, S>(str, addr, op);
}

template <Instr I, Mode M, Size S> void
Moira::dasmFBcc(StrWriter &str, u32 &addr, u16 op)
{
    auto cnd = __________xxxxxx (op);

    auto dst = addr + 2;
    U32_INC(dst, SEXT<S>(dasmRead<S>(addr)));

    str << "fb" << Fcc{cnd} << Sz<S>{} << tab << Imu(dst);
}

template <Instr I, Mode M, Size S> void
Moira::dasmFDbcc(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead(addr);
    auto src = _____________xxx (op);
    auto cnd = __________xxxxxx (ext);

    auto dst = addr + 2;
    U32_INC(dst, SEXT<S>(dasmRead<S>(addr)));

    str << "fdb" << Fcc{cnd} << tab << Dn{src} << Sep{} << UInt(dst);
}

template <Instr I, Mode M, Size S> void
Moira::dasmFRestore(StrWriter &str, u32 &addr, u16 op)
{
    auto dn = _____________xxx (op);

    str << Ins<I>{} << tab << Op<M, S>(dn, addr);
}

template <Instr I, Mode M, Size S> void
Moira::dasmFSave(StrWriter &str, u32 &addr, u16 op)
{
    auto dn = _____________xxx (op);

    str << Ins<I>{} << tab << Op<M, S>(dn, addr);
}

template <Instr I, Mode M, Size S> void
Moira::dasmFScc(StrWriter &str, u32 &addr, u16 op)
{
    printf("dasmFscc(%x): %d %d %d\n", op, I, M, S);
    auto ext = dasmRead(addr);
    auto reg = _____________xxx (op);
    auto cnd = __________xxxxxx (ext);

    str << "fs" << Fcc{cnd} << tab << Op<M, S>(reg, addr);
}

template <Instr I, Mode M, Size S> void
Moira::dasmFTrapcc(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead(addr);
    auto cnd = __________xxxxxx (ext);

    switch (S) {

        case Unsized:

            str << "ftrap" << Fcc{cnd};
            break;

        case Word:
        case Long:

            auto ext = dasmRead<S>(addr);
            str << "ftrap" << Fcc{cnd} << tab << Imu(ext);
            break;
    }
}

template <Instr I, Mode M, Size S> void
Moira::dasmFGeneric(StrWriter &str, u32 &addr, u16 op)
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

template <Instr I, Mode M, Size S> void
Moira::dasmFMove(StrWriter &str, u32 &addr, u16 op)
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
Moira::dasmFMovecr(StrWriter &str, u32 &addr, u16 op)
{
    auto ext = dasmRead(addr);
    auto dst = ______xxx_______ (ext);
    auto ofs = _________xxxxxxx (ext);

    str << Ins<I>{} << Ffmt{2} << tab << Imu{ofs} << Sep{} << Fp{dst};
}

template <Instr I, Mode M, Size S> void
Moira::dasmFMovem(StrWriter &str, u32 &addr, u16 op)
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
