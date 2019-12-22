// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template <Size S, int delay> bool
Moira::addressError(u32 addr)
{
    if (MOIRA_EMULATE_ADDRESS_ERROR) {

        if ((addr & 1) && S != Byte) {
            sync(delay);
            execAddressError(addr);
            return true;
        }
    }
    return false;
}

template<Size S, bool last> u32
Moira::readM(u32 addr)
{
    u32 result;

    sync(2);

    switch (S) {
        case Byte:
        {
            result = memory->moiraRead8(addr & 0xFFFFFF);
            break;
        }
        case Word:
        {
            result = memory->moiraRead16(addr & 0xFFFFFF);
            break;
        }
        case Long:
        {
            u32 hi = memory->moiraRead16(addr & 0xFFFFFF);
            sync(4);
            u32 lo = memory->moiraRead16((addr + 2) & 0xFFFFFF);
            result = hi << 16 | lo;
            break;
        }
    }

    sync(2);

    return result;
}

template<Size S, bool last> u32
Moira::readM(u32 addr, bool &error)
{
    if ((error = addressError<S,2>(addr))) { return 0; }
    return readM<S,last>(addr);
}

template<Size S, bool last> void
Moira::writeM(u32 addr, u32 value)
{
    sync(2);

    switch (S) {
        case Byte:
        {
            memory->moiraWrite8(addr & 0xFFFFFF, (u8)value);
            break;
        }
        case Word:
        {
            memory->moiraWrite16(addr & 0xFFFFFF, (u16)value);
            break;
        }
        case Long:
        {
            memory->moiraWrite16(addr & 0xFFFFFF, value >> 16);
            sync(4);
            memory->moiraWrite16((addr + 2) & 0xFFFFFF, value & 0xFFFF);
            break;
        }
    }

    sync(2);
}

template<Size S, bool last> void
Moira::writeM(u32 addr, u32 value, bool &error)
{
    if ((error = addressError<S,2>(addr))) { return; }
    writeM<S,last>(addr, value);
}

u32
Moira::readOnReset(u32 addr)
{
    sync(2);
    u32 result = memory->moiraReadAfterReset16(addr & 0xFFFFFF);
    sync(2);

    return result;
}

template<Size S> void
Moira::push(u32 value)
{
    reg.sp -= S;
    writeM<S>(reg.sp, value);
}

template<Mode M, Size S, u8 flags> u32
Moira::computeEA(u32 n) {

    assert(n < 8);

    u32 result;

    switch (M) {

        case 0:  // Dn
        case 1:  // An
        {
            result = n;
            break;
        }
        case 2:  // (An)
        {
            result = readA(n);
            break;
        }
        case 3:  // (An)+
        {
            result = readA(n);
            break;
        }
        case 4:  // -(An)
        {
            sync(2);
            result = readA(n) - ((n == 7 && S == Byte) ? 2 : S);
            break;
        }
        case 5: // (d,An)
        {
            u32 an = readA(n);
            i16  d = (i16)irc;

            result = d + an;
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 6: // (d,An,Xi)
        {
            i8   d = (i8)irc;
            i32 an = readA(n);
            i32 xi = readR((irc >> 12) & 0b1111);

            result = d + an + ((irc & 0x800) ? xi : (i16)xi);

            sync(2);
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 7: // ABS.W
        {
            result = irc;
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 8: // ABS.L
        {
            result = irc << 16;
            readExtensionWord();
            result |= irc;
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 9: // (d,PC)
        {
            i16  d = (i16)irc;

            result = reg.pc + d;
            if (!(flags & SKIP_LAST_READ)) readExtensionWord();
            break;
        }
        case 10: // (d,PC,Xi)
        {
            i8   d = (i8)irc;
            i32 xi = readR((irc >> 12) & 0b1111);

            result = d + reg.pc + ((irc & 0x800) ? xi : (i16)xi);

            sync(2);
            readExtensionWord();
            break;
        }
        case 11: // Im
        {
            result = readImm<S>();
            break;
        }
        default:
        {
            assert(false);
        }
    }
    return result;
}

template<Mode M, Size S> void
Moira::updateAn(int n)
{
    // (An)+
    if (M == 3) reg.a[n] += (n == 7 && S == Byte) ? 2 : S;

    // -(An)
    if (M == 4) reg.a[n] -= (n == 7 && S == Byte) ? 2 : S;
}

template<Mode M, Size S> bool
Moira::readOperand(int n, u32 &ea, u32 &result)
{
    switch (M) {

        case 0:  // Dn
        {
            result = readD<S>(n);
            break;
        }
        case 1:  // An
        {
            result = readA<S>(n);
            break;
        }
        case 11: // Im
        {
            result = readImm<S>();
            break;
        }
        default: // Ea
        {
            // Compute effective address
            ea = computeEA<M,S>(n);

            // Read from effective address
            bool error; result = readM<S>(ea, error);

            // Exit if an address error has occurred
            if (error) return false;

            // Emulate (An)+ or -(An) register modification
            updateAn<M,S>(n);
        }
    }

   return true;
}

template<Mode M, Size S> bool
Moira::writeOperand(int n, u32 value)
{
    switch (M) {

        case 0:  // Dn
        {
            writeD<S>(n, value);
            break;
        }
        case 1:  // An
        {
            writeA<S>(n, value);
            break;
        }
        case 11: // Im
        {
            assert(false);
            break;
        }
        default:
        {
            // Compute effective address
            u32 ea = computeEA<M,S>(n);

            // Write to effective address
            bool error; writeM<S>(ea, value, error);

            // Early exit in case of an address error
            if (error) return false;

            // Emulate (An)+ or -(An) register modification
            updateAn<M,S>(n);
        }
    }

    return true;
}

template<Mode M, Size S> void
Moira::writeOperand(int n, u32 ea, u32 value)
{
    assert(M < 11);

    switch (M) {

        case 0:  // Dn
        {
            writeD<S>(n, value);
            break;
        }
        case 1:  // An
        {
            writeA<S>(n, value);
            break;
        }
        case 11: // Im
        {
            assert(false);
            break;
        }
        default:
        {
            writeM<S>(ea, value);
            break;
        }
    }
}

template<Size S> u32
Moira::readImm()
{
    u32 result;

    switch (S) {
        case Byte:
            result = (u8)irc;
            readExtensionWord();
            break;
        case Word:
            result = irc;
            readExtensionWord();
            break;
        case Long:
            result = irc << 16;
            readExtensionWord();
            result |= irc;
            readExtensionWord();
            break;
    }

    return result;
}
