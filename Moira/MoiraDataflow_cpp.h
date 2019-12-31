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

template<Size S, bool last> u32
Moira::readM(u32 addr)
{
    u32 result;

    switch (S) {
        case Byte:
        {
            sync(2);
            if (last) pollIrq();
            result = read8(addr & 0xFFFFFF);
            sync(2);
            break;
        }
        case Word:
        {
            sync(2);
            if (last) pollIrq();
            result = read16(addr & 0xFFFFFF);
            sync(2);
            break;
        }
        case Long:
        {
            result = readM<Word>(addr) << 16;
            result |= readM<Word,last>(addr + 2);
            break;
        }
    }
    return result;
}

template<Size S, bool last> u32
Moira::readM(u32 addr, bool &error)
{
    if ((error = addressError<S,2>(addr))) { return 0; }
    return readM<S,last>(addr);
}

template<Size S, bool last> void
Moira::writeM(u32 addr, u32 val)
{
    switch (S) {

        case Byte:
        {
            sync(2);
            if (last) pollIrq();
            write8(addr & 0xFFFFFF, (u8)val);
            sync(2);
            break;
        }
        case Word:
        {
            sync(2);
            if (last) pollIrq();
            write16(addr & 0xFFFFFF, (u16)val);
            sync(2);
            break;
        }
        case Long:
        {
            writeM<Word>     (addr,     val >> 16   );
            writeM<Word,last>(addr + 2, val & 0xFFFF);
            break;
        }
    }
}

template<Size S, bool last> void
Moira::writeM(u32 addr, u32 val, bool &error)
{
    if ((error = addressError<S,2>(addr))) { return; }
    writeM<S,last>(addr, val);
}

template<Size S, bool last> void
Moira::writeMrev(u32 addr, u32 val)
{
    switch (S) {

        case Byte:
        case Word:
        {
            writeM<S,last>(addr, val);
            break;
        }
        case Long:
        {
            writeM<Word>     (addr + 2, val & 0xFFFF);
            writeM<Word,last>(addr,     val >> 16   );
            break;
        }
    }
}

template<Size S, bool last> void
Moira::writeMrev(u32 addr, u32 val, bool &error)
{
    if ((error = addressError<S,2>(addr))) { return; }
    writeMrev<S,last>(addr, val);
}


