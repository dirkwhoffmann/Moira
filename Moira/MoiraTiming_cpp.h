// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

void
Moira::sync(int cycles)
{
    clock += cycles;
}

template <Instr I> int
Moira::cyclesBit(u8 bit)
{
    switch (I)
    {
        case BTST: return 2;
        case BCLR: return (bit > 15 ? 6 : 4);
        case BSET:
        case BCHG: return (bit > 15 ? 4 : 2);
    }

    assert(false);
    return 0;
}

int
Moira::cyclesMULU(u16 data)
{
    int mcycles = 17;

    for (; data; data >>= 1) if (data & 1) mcycles++;
    return 2 * mcycles;
}

int
Moira::cyclesMULS(u16 data)
{
    int mcycles = 17;

    data = ((data << 1) ^ data) & 0xFFFF;
    for (; data; data >>= 1) if (data & 1) mcycles++;
    return 2 * mcycles;
}

int
Moira::cyclesDIVU(u32 dividend, u16 divisor)
{
    int mcycles = 38;

    // Check if quotient is larger than 16 bit
    if ((dividend >> 16) >= divisor) return 10;
    u32 hdivisor = divisor << 16;

    for (int i = 0; i < 15; i++) {
        if ((i32)dividend < 0) {
            dividend <<= 1;
            dividend -= hdivisor;
        } else {
            dividend <<= 1;
            if (dividend >= hdivisor) {
                dividend -= hdivisor;
                mcycles += 1;
            } else {
                mcycles += 2;
            }
        }
    }
    return 2 * mcycles;
}

int
Moira::cyclesDIVS(i32 dividend, i16 divisor)
{
    int mcycles = (dividend < 0) ? 7 : 6;

    // Check if quotient is larger than 16 bit
    if ((abs(dividend) >> 16) >= abs(divisor))
        return (mcycles + 2) * 2;

    mcycles += 55;

    if (divisor >= 0) {
        mcycles += (dividend < 0) ? 1 : -1;
    }

    u32 aquot = abs(dividend) / abs(divisor);
    for (int i = 0; i < 15; i++) {
        if ( (i16)aquot >= 0) mcycles++;
        aquot <<= 1;
    }
    return 2 * mcycles;
}
