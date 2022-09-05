// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

bool
Moira::isValidExtFPU(Instr I, Mode M, u16 op, u32 ext)
{
    auto cod  = xxx_____________ (ext);
    auto mode = ___xx___________ (ext);
    auto fmt  = ___xxx__________ (ext);
    auto lst  = ___xxx__________ (ext);
    auto cmd  = _________xxxxxxx (ext);

    switch (I) {

        case FDBcc:
        case FScc:
        case FTRAPcc:
            
            return (ext & 0xFFE0) == 0;

        case FMOVECR:

            return (op & 0x3F) == 0;
            
        case FMOVE:
        case FMOVEM:

            switch (cod) {

                case 0b010: // FMOVE (??)

                    if (M == MODE_IP) break;
                    return true;

                case 0b000: // FMOVE

                    if (cmd == 0 && cod == 0 && (op & 0x3F)) break;
                    return true;

                case 0b011: // FMOVE

                    // Check for k-factor formats
                    if (M == MODE_DN || M == MODE_AN) { if (fmt == 0b011 || fmt == 0b111) break; }
                    if (M == MODE_AI) { if (fmt == 0b111 && (ext & 0xF)) break; }
                    if (M == MODE_PI) { if (fmt == 0b111 && (ext & 0xF)) break; }
                    if (M == MODE_PD) { if (fmt == 0b111 && (ext & 0xF)) break; }
                    if (M == MODE_DI) { if (fmt == 0b111 && (ext & 0xF)) break; }
                    if (M == MODE_IX) { if (fmt == 0b111 && (ext & 0xF)) break; }
                    if (M == MODE_AW) { if (fmt == 0b111 && (ext & 0xF)) break; }
                    if (M == MODE_AL) { if (fmt == 0b111 && (ext & 0xF)) break; }
                    if (M == MODE_DIPC) break;
                    if (M == MODE_IXPC) break;
                    if (M == MODE_IM) break;
                    if (M == MODE_IP) break;

                    if (fmt != 0b011 && fmt != 0b111 && (ext & 0x7F)) break;

                    if (M == MODE_DN && (fmt != 0 && fmt != 1 && fmt != 4 && fmt != 6)) {
                        break;
                    }
                    return true;

                case 0b101: // FMOVEM
                {

                    if (ext & 0x3FF) break;

                    if (M == MODE_DN || M == MODE_AN) {
                        if (lst != 0b000 && lst != 0b001 && lst != 0b010 && lst != 0b100) break;
                    }
                    if (M == MODE_DIPC) break;
                    if (M == MODE_IXPC) break;
                    if (M == MODE_IM) break;
                    if (M == MODE_IP) break;
                    return true;
                }
                case 0b100: // FMOVEM

                    if (ext & 0x3FF) break;
                    if (M == MODE_IP) break;
                    return true;

                case 0b110: // FMOVEM
                case 0b111:

                    if (M == MODE_DN || M == MODE_AN) break;
                    if (M == MODE_AI) { if (mode == 0 || mode == 1) break; }
                    if (M == MODE_PI) { if (mode == 0 || mode == 1) break; if (cod == 0b111) break; }
                    if (M == MODE_PD && cod == 0b110) { break; }
                    if (M == MODE_PD && cod == 0b111 && (mode == 1) && (ext & 0x8F)) { break; }
                    if (M == MODE_PD && cod == 0b111 && (mode == 2 || mode == 3)) { break; }
                    if (M == MODE_DI) { if (mode == 0 || mode == 1) break; }
                    if (M == MODE_IX) { if (mode == 0 || mode == 1) break; }
                    if (M == MODE_AW) { if (mode == 0 || mode == 1) break; }
                    if (M == MODE_AL) { if (mode == 0 || mode == 1) break; }
                    if (M == MODE_DIPC) break;
                    if (M == MODE_IXPC) break;
                    if (M == MODE_IM) break;
                    if (M == MODE_IP) break;

                    if (ext & 0x0700) break;
                    if (mode == 3 && (ext & 0x8F)) break;
                    return true;
            }
            return false;

        default:
            fatalError;
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execFBcc(u16 opcode)
{
    execLineF<C, I, M, S>(opcode);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execFDbcc(u16 opcode)
{
    execLineF<C, I, M, S>(opcode);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execFGen(u16 opcode)
{
    execLineF<C, I, M, S>(opcode);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execFNop(u16 opcode)
{
    execLineF<C, I, M, S>(opcode);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execFRestore(u16 opcode)
{
    execLineF<C, I, M, S>(opcode);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execFSave(u16 opcode)
{
    execLineF<C, I, M, S>(opcode);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execFScc(u16 opcode)
{
    execLineF<C, I, M, S>(opcode);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execFTrapcc(u16 opcode)
{
    execLineF<C, I, M, S>(opcode);
}

