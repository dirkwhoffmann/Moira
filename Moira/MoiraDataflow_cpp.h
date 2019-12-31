// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template<Mode M, Size S> bool
Moira::readOp(int n, u32 &ea, u32 &result)
{
    // Handle non-memory modes
    if (M == MODE_DN) { result = readD<S>(n); return true; }
    if (M == MODE_AN) { result = readA<S>(n); return true; }
    if (M == MODE_IM) { result = readI<S>();  return true; }

    // Compute effective address
    ea = computeEA<M,S>(n);

    // Read from effective address
    bool error; result = readM<S>(ea, error);

    // Exit if an address error has occurred
    if (error) return false;

    // Emulate (An)+ or -(An) register modification
    updateAn<M,S>(n);
    return true;
}

template<Mode M, Size S, bool last> bool
Moira::writeOp(int n, u32 val)
{
    // Handle non-memory modes
    if (M == MODE_DN) { writeD<S>(n, val); return true;  }
    if (M == MODE_AN) { writeA<S>(n, val); return true;  }
    if (M == MODE_IM) { assert(false);     return false; }

    // Compute effective address
    u32 ea = computeEA<M,S>(n);

    // Write to effective address
    bool error; writeM<S,last>(ea, val, error);

    // Early exit in case of an address error
    if (error) return false;

    // Emulate (An)+ or -(An) register modification
    updateAn<M,S>(n);
    return true;
}

template<Mode M, Size S, bool last> void
Moira::writeOp(int n, u32 ea, u32 val)
{
    // Handle non-memory modes
    if (M == MODE_DN) { writeD<S>(n, val); return; }
    if (M == MODE_AN) { writeA<S>(n, val); return; }
    if (M == MODE_IM) { assert(false);     return; }

    writeM<S,last>(ea, val);
}


