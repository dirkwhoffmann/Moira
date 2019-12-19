// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

int
Moira::syncLineA(u16 opcode, int i)
{
    return 0;
}

int
Moira::syncLineF(u16 opcode, int i)
{
    return 0;
}

int
Moira::syncIllegal(u16 opcode, int i)
{
    return 0;
}

template<Instr I, Size S> int Moira::syncRegShift(u16 opcode, int i) {
    return i;
}

template<Instr I, Size S> int Moira::syncImmShift(u16 opcode, int i) {
    return i;
}

template<Instr I, Mode M> int Moira::syncEaShift(u16 opcode, int i) {
    return i;
}

template<Mode M> int Moira::syncLea(u16 opcode, int i) {
    return i;
}
