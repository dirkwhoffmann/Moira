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

    /*

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
     break;
     }

     case 0x5:    // control to ea
     {
     str << "fmovem.l" << tab;
     if (w2 & 0x1000) str << "fpcr";
     if (w2 & 0x0800) str << "/fpsr";
     if (w2 & 0x0400) str << "/fpiar";
     str << Sep{} << Op<M, Long>(reg, addr);
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
     */
}

template <Instr I, Mode M, Size S> void
Moira::dasmFbcc(StrWriter &str, u32 &addr, u16 op)
{
    str << "dasmFbcc";
}

template <Instr I, Mode M, Size S> void
Moira::dasmFdbcc(StrWriter &str, u32 &addr, u16 op)
{
    str << "dasmFdbcc";
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
                    str << Op<MODE_PD, Long>(reg, addr) << Sep{};
                    str << FRegList(ext & 0xFF);
                    break;

                case 0b01: // Dynamic list, predecrement addressing

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Op<MODE_PD, Long>(reg, addr) << Sep{};
                    str << Dn{rrr};
                    break;

                case 0b10: // Static list, postincrement addressing (invalid?!)

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Op<MODE_PI, Long>(reg, addr) << Sep{};
                    str << FRegList(REVERSE_8(ext & 0xFF));
                    break;

                case 0b11: // Dynamic list, postincrement addressing (invalid?!)

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Op<MODE_PI, Long>(reg, addr) << Sep{};
                    str << Dn{rrr};
                    break;
            }
            break;

        case 0b111: // FPU to memory

            switch (mod) {

                case 0b00: // Static list, predecrement addressing  (invalid?!)

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << FRegList(ext & 0xFF) << Sep{};
                    str << Op<MODE_PD, Long>(reg, addr);
                    break;

                case 0b01: // Dynamic list, predecrement addressing  (invalid?!)

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Dn{rrr} << Sep{};
                    str << Op<MODE_PD, Long>(reg, addr);
                    break;

                case 0b10: // Static list, postincrement addressing (invalid?!)

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << FRegList(REVERSE_8(ext & 0xFF)) << Sep{};
                    str << Op<MODE_PI, Long>(reg, addr);
                    break;

                case 0b11: // Dynamic list, postincrement addressing (invalid?!)

                    str << Ins<I>{} << Ffmt{2} << tab;
                    str << Dn{rrr} << Sep{};
                    str << Op<MODE_PI, Long>(reg, addr);
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
