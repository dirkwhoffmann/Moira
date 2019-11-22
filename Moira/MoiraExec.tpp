// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template<int size, int mode> void
Moira::execRegShift(uint16_t opcode)
{
    (void)shift<(Size)1,(Instr)2>(42,42);
}

template<int size, int mode> void
Moira::execImmShift(uint16_t opcode)
{

}

