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

u16
Moira::readOnReset(u32 addr)
{
    sync(2);
    u16 result = read16Reset(addr & 0xFFFFFF);
    sync(2);

    return result;
}

template<Size S, bool last> void
Moira::push(u32 val)
{
    reg.sp -= S;
    writeM<S,last>(reg.sp, val);
}

template<Mode M, Size S, bool skip> u32
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
            i16  d = (i16)queue.irc;

            result = d + an;
            readExtensionWord<skip>();
            break;
        }
        case 6: // (d,An,Xi)
        {
            i8   d = (i8)queue.irc;
            u32 an = readA(n);
            u32 xi = readR((queue.irc >> 12) & 0b1111);

            result = d + an + ((queue.irc & 0x800) ? xi : SEXT<Word>(xi));

            sync(2);
            readExtensionWord<skip>();
            break;
        }
        case 7: // ABS.W
        {
            result = (i16)queue.irc;
            readExtensionWord<skip>();
            break;
        }
        case 8: // ABS.L
        {
            result = queue.irc << 16;
            readExtensionWord();
            result |= queue.irc;
            readExtensionWord<skip>();
            break;
        }
        case 9: // (d,PC)
        {
            i16  d = (i16)queue.irc;

            result = reg.pc + d;
            readExtensionWord<skip>();
            break;
        }
        case 10: // (d,PC,Xi)
        {
            i8   d = (i8)queue.irc;
            u32 xi = readR((queue.irc >> 12) & 0b1111);

            result = d + reg.pc + ((queue.irc & 0x800) ? xi : SEXT<Word>(xi));
            sync(2);
            readExtensionWord<skip>();
            break;
        }
        case 11: // Im
        {
            result = readI<S>();
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

template<Size S> u32
Moira::readI()
{
    u32 result;

    switch (S) {
        case Byte:
            result = (u8)queue.irc;
            readExtensionWord();
            break;
        case Word:
            result = queue.irc;
            readExtensionWord();
            break;
        case Long:
            result = queue.irc << 16;
            readExtensionWord();
            result |= queue.irc;
            readExtensionWord();
            break;
    }

    return result;
}
