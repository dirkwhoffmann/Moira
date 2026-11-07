// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

// MMU context used while walking the translation tables
struct MmuContext {

    u64 addrBits;
    u64 indexBits;
    u8  shiftCnt;

    u32 lowerLimit;
    u32 upperLimit;

    bool wp;
    bool su;

    // The logical address that triggered the lookup (reported in bus error frames)
    u32 faultAddr;

    u32 getAddrBits() { return u32(addrBits >> 32); }
    u32 getIndexBits() { return u32(indexBits >> 32); }

    u32 nextAddrBits() {

        // Shift the new index into the upper word
        indexBits = (indexBits & 0xFFFFFFFF) << 4;

        // Get the upper word
        auto shift = indexBits >> 32;

        // Remember the total number of shifted bits
        shiftCnt += u8(shift);

        // Shift the new address bits into the upper word
        addrBits = (addrBits & 0xFFFFFFFF) << shift;

        // Return the upper word
        return u32(addrBits >> 32);
    }

    u32 remainingAddrBits() {

        return u32(addrBits) >> shiftCnt;
    }
};


// The following definitions are not templated. They are compiled into the
// main translation unit only (see MoiraCore_cpp.h).
#ifdef MOIRA_MAIN_TU

void
Moira::setTC(u32 value)
{

    bool oldE = mmu.tc & (1 << 31);
    mmu.tc = value;
    bool newE = mmu.tc & (1 << 31);

    if (!oldE && newE) mmuDidEnable();
    if (oldE && !newE) mmuDidDisable();
}

void
Moira::setTT0(u64 value)
{

}

void
Moira::setTT1(u64 value)
{

}

bool
Moira::testTT(u32 ttx, u32 addr, u8 fc, bool rw)
{
    u32 addrBase = ttx >> 24 & 0xFF;
    u32 addrMask = ttx >> 16 & 0xFF;
    auto e       = x_______________ (ttx);
    auto rwb     = ______x_________ (ttx);
    auto rwm     = _______x________ (ttx);
    auto fcBase  = _________xxx____ (ttx);
    auto fcMask  = _____________xxx (ttx);
    
    if (!e) {
        return false;
    }
    if ((addr >> 24 | addrMask) != (addrBase | addrMask)) {
        return false;
    }
    if ((fc | fcMask) != (fcBase | fcMask)) {
        return false;
    }
    if ((rw | rwm) != (rwb | rwm)) {
        return false;
    }
    return true;
}

bool
Moira::isValidExtMMU(Instr I, Mode M, u16 op, u32 ext) const
{
    auto preg  = [ext]() { return ext >> 10 & 0b111;   };
    auto a     = [ext]() { return ext >>  8 & 0b1;     };
    auto mode  = [ext]() { return ext >> 10 & 0b111;   };
    auto mask  = [ext]() { return ext >>  5 & 0b1111;  }; // 68851 mask is 4 bit
    auto reg   = [ext]() { return ext >>  5 & 0b111;   };
    auto fc    = [ext]() { return ext       & 0b11111; };

    auto validFC = [&]() {
        return fc() <= 1 || (fc() >= 8); // Binutils checks M68851
    };

    switch (I) {

        case Instr::PFLUSHA:

            return (op & 0xFF) == 0 && mask() == 0 && fc() == 0;

        case Instr::PFLUSH:

            // Check mode
            if (mode() != 0b100 && mode() != 0b110) return false;

            // Check EA mode
            if (mode() == 0b110) {
                if (M != Mode::AI && M != Mode::DI && M != Mode::IX && M != Mode::AW && M != Mode::AL) {
                    return false;
                }
            }
            return validFC();

        case Instr::PLOAD:

            // Check EA mode
            if (M != Mode::AI && M != Mode::DI && M != Mode::IX && M != Mode::AW && M != Mode::AL) {
                return false;
            }

            return validFC();

        case Instr::PMOVE:

            if ((ext & 0x200)) {
                if (M == Mode::DIPC || M == Mode::IXPC || M == Mode::IM) return false;
            }
            if (M == Mode::IP) return false;

            switch (ext >> 13 & 0b111) {

                case 0b000:

                    // Check register field
                    if (preg() != 0b010 && preg() != 0b011) return false;

                    // If memory is written, flushing is mandatory
                    if ((ext & 0x300) == 0x300) return false;
                    return true;

                case 0b010:

                    // If memory is written, flushing is mandatory
                    if ((ext & 0x300) == 0x300) return false;

                    if ((ext & 0x300) == 0) {
                        if (preg() != 0) {
                            if (M == Mode::PI || M == Mode::PD || M == Mode::IM || M == Mode::IP) { return false;
                            }
                        }
                    }

                    // Check register field (binutils accepts all M68851 registers)
                    if ((ext & 0x100) == 0) {
                        if (preg() != 0) {
                            if (M == Mode::DN || M == Mode::AN) {
                                return false;
                            }
                        }
                    }
                    return true;

                case 0b011:

                    return true;

                default:
                    return false;
            }
            break;

        case Instr::PTEST:

            // When A is 0, reg must be 0
            if (a() == 0 && reg() != 0) return false;

            // Check FC
            if ((fc() & 0b11000) == 0 && (fc() & 0b110) != 0) return false;

            // Check EA mode
            if (M != Mode::AI && M != Mode::DI && M != Mode::IX && M != Mode::AW && M != Mode::AL) return false;

            return true;

        default:
            fatalError;
    }
}

#endif


//
// Address translation
//

template <Core C, bool write> u32
Moira::translate(u32 addr, u8 fc)
{
    // Only proceed if a MMU capable core is present
    if constexpr (C == Core::C68000 || C == Core::C68010) return addr;

    // Only proceed if the MMU is enabled
    if (!(mmu.tc & 0x80000000)) return addr;

    // Only proceed if the selected CPU model has a MMU
    if (!hasMMU()) return addr;

    //
    // TODO: LOOKUP IN CACHE
    //

    return mmuLookup<C, write>(addr, fc);
}

template <Core C, bool write> u32
Moira::mmuLookup(u32 addr, u8 fc)
{
    MmuContext context { .addrBits = addr, .indexBits = mmu.tc << 12 };
    context.faultAddr = addr;

    // Start with the SRP or CRP
    u64 rp = (reg.sr.s && (mmu.tc & 0x02000000)) ? mmu.srp : mmu.crp;

    // +------------------------------------------------+
    // | ROOT POINTER (CRP/SRP)                         |
    // +-----+--------+---------------------------------+
    // | Bit | Length | Contents                        |
    // +-----+--------+---------------------------------+
    // | 00  |   04   | reserved ( must be 0 )          |
    // | 04  |   28   | TableA Address (upper 28 bits)  |
    // | 32  |   02   | Descriptor Type                 |
    // | 34  |   14   | reserved ( must be 0 )          |
    // | 48  |   15   | Limit                           |
    // | 63  |   01   | L/U                             |
    // +----+---------+---------------------------------+

    u32 taddr = u32(rp >> 0 ) & 0xFFFFFFF0;
    u32 dt    = u32(rp >> 32) & 0x3;
    u32 limit = u32(rp >> 48) & 0x7FFF;

    // Evaluate the limit field
    if (rp & (1LL << 63)) {

        context.lowerLimit = limit;
        context.upperLimit = 0XFFFF;

    } else {

        context.lowerLimit = 0;
        context.upperLimit = limit;
    }

    // Apply the initial shift (ignore some bits)
    (void)context.nextAddrBits();


    // Check the descriptor type
    switch (dt) {

        case 1:
        {
            /* Early termination. No translation table exists, all memory
             * accesses are calculated by adding the TableA address to the
             * logical address specified.
             */
            return taddr + addr;
        }
        case 2:
        {
            bool fcl = mmu.tc & ( 1 << 24);

            if (fcl) {

                u32 offset = readFC();

                return mmuLookupShort<C, write>('A', taddr, offset, context);

            } else {

                u32 offset = context.nextAddrBits();

                return mmuLookupShort<C, write>('A', taddr, offset, context);
            }
        }
        case 3:
        {
            bool fcl = mmu.tc & ( 1 << 24);

            if (fcl) {

                u32 offset = readFC();

                return mmuLookupLong<C, write>('A', taddr, offset, context);

            } else {

                u32 offset = context.nextAddrBits();

                return mmuLookupLong<C, write>('A', taddr, offset, context);
            }
        }
        default:

            // TODO: WHICH EXCEPTION? (MMU CONFIG ERROR? BUS ERROR?)
            throw BusError(makeFrame<write ? AE_WRITE : 0>(context.faultAddr));
    }
}

template <Core C, bool write> u32
Moira::mmuLookupShort(char table, u32 taddr, u32 offset, struct MmuContext &c)
{
    u32 physAddr;

    // Check offset
    if (offset < c.lowerLimit || offset > c.upperLimit) {

        throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));
    }

    // Read table entry
    u32 descriptor = readMMU32(taddr + 4 * offset);

    // Extract descriptor type
    u32 dt = descriptor & 0x3;

    switch (dt) {

        case 0:
        {
            // +------------------------------------------------+
            // | SHORT FORMAT INVALID DESCRIPTOR                |
            // +-----+--------+---------------------------------+
            // | Bit | Length | Contents                        |
            // +-----+--------+---------------------------------+
            // | 00  |   02   | Descriptor Type (DT)            |
            // | 02  |   30   | Unused                          |
            // +-----+--------+---------------------------------+

            throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));
        }
        case 1:
        {
            // +------------------------------------------------+
            // | SHORT FORMAT EARLY TERM. OR PAGE DESCRIPTOR    |
            // +-----+--------+---------------------------------+
            // | Bit | Length | Contents                        |
            // +-----+--------+---------------------------------+
            // | 00  |   02   | Descriptor Type (DT)            |
            // | 02  |   01   | Write Protect (WP)              |
            // | 03  |   01   | Update (U)                      |
            // | 04  |   01   | Modified (M)                    |
            // | 05  |   01   | Reserved ( must be 0 )          |
            // | 06  |   01   | Cache Inhibit (CI)              |
            // | 07  |   01   | Reserved ( must be 0 )          |
            // | 08  |   24   | Page Address                    |
            // +-----+--------+---------------------------------+

            // Record WP
            if constexpr (write) c.wp |= descriptor & 0x4;

            // Set U and M bit
            u64 desc2 = descriptor | (1LL << 3);
            if constexpr (write) { desc2 |= (1LL << 4); }
            write16(taddr + 4 * offset + 2, (u16)desc2);

            physAddr = (descriptor & 0xFFFFFF00) + c.remainingAddrBits();

            if (table == 'D') {


            } else {

            }
            break;
        }
        default:
        {
            if (table == 'D') {

                // +------------------------------------------------+
                // | SHORT FORMAT INDIRECT DESCRIPTOR               |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   30   | Descriptor Address              |
                // +-----+--------+---------------------------------+

                taddr = descriptor & 0xFFFFFFFC;


                physAddr = mmuLookupShort<C, write>(table + 1, taddr, 0, c);

            } else {

                // +------------------------------------------------+
                // | SHORT FORMAT TABLE DESCRIPTOR                  |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   01   | Write Protect (WP)              |
                // | 03  |   01   | Update (U)                      |
                // | 04  |   28   | Table Address                   |
                // +-----+--------+---------------------------------+

                // Record WP
                if constexpr (write) c.wp |= descriptor & 0x4;

                // Set U bit
                u64 desc2 = descriptor | (1LL << 3);
                write16(taddr + 4 * offset + 2, (u16)desc2);

                u32 offset = c.nextAddrBits();

                taddr = descriptor & 0xFFFFFFF0;

                // Short descriptors carry no limit. The limit of the root
                // pointer has already been checked for the current level.
                if constexpr (write) { c.wp |= descriptor & 0x4; }
                c.lowerLimit = 0;
                c.upperLimit = 0xFFFF;

                if (dt == 2) {
                    physAddr = mmuLookupShort<C, write>(table + 1, taddr, offset, c);
                } else {
                    physAddr = mmuLookupLong<C, write>(table + 1, taddr, offset, c);
                }
            }
            break;
        }
    }

    // Check for write protection
    if constexpr (write) {
        if (c.wp) {

            throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));
        }
    }

    // Check for supervisor protection
    if (c.su && !reg.sr.s) throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));

    return physAddr;
}

template <Core C, bool write> u32
Moira::mmuLookupLong(char table, u32 taddr, u32 offset, struct MmuContext &c)
{
    u32 physAddr;

    // Check offset
    if (offset < c.lowerLimit || offset > c.upperLimit) {

        throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));
    }

    // Read table entry
    u64 descriptor = readMMU64(taddr + 8 * offset);

    // Extract descriptor type
    u32 dt = u32(descriptor >> 32) & 0x3;

    // Evaluate the limit field
    if (descriptor & (1LL << 63)) {
        c.lowerLimit = u32(descriptor >> 48) & 0x7FFF;
        c.upperLimit = 0xFFFF;
    } else {
        c.lowerLimit = 0;
        c.upperLimit = u32(descriptor >> 48) & 0x7FFF;
    }

    switch (dt) {

        case 0:
        {
            // +------------------------------------------------+
            // | LONG FORMAT INVALID DESCRIPTOR                 |
            // +-----+--------+---------------------------------+
            // | Bit | Length | Contents                        |
            // +-----+--------+---------------------------------+
            // | LongWord0                                      |
            // +-----+--------+---------------------------------+
            // | 00  |   02   | Descriptor Type (DT)            |
            // | 02  |   30   | Unused                          |
            // +-----+--------+---------------------------------+
            // | LongWord1                                      |
            // +-----+--------+---------------------------------+
            // | 00  |   32   | Unused                          |
            // +-----+--------+---------------------------------+

            throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));
        }
        case 1:
        {
            // Record WP and S bit
            if constexpr (write) c.wp |= (descriptor >> 32) & 0x4;
            c.su |= (descriptor >> 32) & 0x100;

            // Set U and M bit
            u64 desc2 = (descriptor >> 32) | (1LL << 3);
            if constexpr (write) { desc2 |= (1LL << 4); }
            write16(taddr + 8 * offset + 2, (u16)desc2);

            physAddr = (descriptor & 0xFFFFFF00) + c.remainingAddrBits();

            if (table == 'D') {

                // +------------------------------------------------+
                // | LONG FORMAT PAGE DESCRIPTOR                    |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | LongWord0                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   01   | Write Protect (WP)              |
                // | 03  |   01   | Update (U)                      |
                // | 04  |   01   | Modified (M)                    |
                // | 05  |   01   | Reserved ( must be 0 )          |
                // | 06  |   01   | Cache Inhibit (CI)              |
                // | 07  |   01   | Reserved ( must be 0 )          |
                // | 08  |   01   | Supervisor (S)                  |
                // | 09  |   07   | Reserved ( must be 1111110 )    |
                // | 16  |   16   | Unused                          |
                // +-----+--------+---------------------------------+
                // | LongWord1                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   07   | Unused                          |
                // | 08  |   24   | Page Address                    |
                // +-----+--------+---------------------------------+


            } else {

                // +------------------------------------------------+
                // | LONG FORMAT EARLY TERMINATION PAGE DESCRIPTOR  |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | LongWord0                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   01   | Write Protect (WP)              |
                // | 03  |   01   | Update (U)                      |
                // | 04  |   01   | Modified (M)                    |
                // | 05  |   01   | Reserved ( must be 0 )          |
                // | 06  |   01   | Cache Inhibit (CI)              |
                // | 07  |   01   | Reserved ( must be 0 )          |
                // | 08  |   01   | Supervisor (S)                  |
                // | 09  |   07   | Reserved ( must be 1111110 )    |
                // | 16  |   15   | Limit                           |
                // | 31  |   01   | L/U                             |
                // +-----+--------+---------------------------------+
                // | LongWord1                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   07   | Unused                          |
                // | 08  |   24   | Page Address                    |
                // +-----+--------+---------------------------------+

            }
            break;
        }
        default:
        {
            if (table == 'D') {

                // +------------------------------------------------+
                // | LONG FORMAT INDIRECT DESCRIPTOR                |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | LongWord0                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   30   | Unused                          |
                // +-----+--------+---------------------------------+
                // | LongWord1                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Unused                          |
                // | 02  |   30   | Descriptor Address              |
                // +-----+--------+---------------------------------+

                taddr = descriptor & 0xFFFFFFFC;


                physAddr = mmuLookupLong<C, write>(table, taddr, 0, c);

            } else {

                // +------------------------------------------------+
                // | LONG FORMAT TABLE DESCRIPTOR                   |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | LongWord0                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   01   | Write Protect (WP)              |
                // | 03  |   01   | Update (U)                      |
                // | 04  |   04   | Reserved ( must be 0000 )       |
                // | 08  |   01   | Supervisor (S)                  |
                // | 09  |   07   | Reserved ( must be 1111110 )    |
                // | 16  |   15   | Limit                           |
                // | 31  |   01   | L/U                             |
                // +-----+--------+---------------------------------+
                // | LongWord1                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   04   | Unused                          |
                // | 04  |   28   | Table Address                   |
                // +-----+--------+---------------------------------+

                // Record WP and S bit
                if constexpr (write) c.wp |= (descriptor >> 32) & 0x4;
                c.su |= (descriptor >> 32) & 0x100;

                // Set U bit
                u64 desc2 = (descriptor >> 32) | (1LL << 3);
                write16(taddr + 8 * offset + 2, (u16)desc2);

                u32 offset = c.nextAddrBits();

                taddr = descriptor & 0xFFFFFFF0;


                // Check offset range
                if (offset < c.lowerLimit || offset > c.upperLimit) {

                    throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));
                }

                c.lowerLimit = 0;
                c.upperLimit = 0xFFFF;

                if (dt == 2) {
                    physAddr = mmuLookupShort<C, write>(table + 1, taddr, offset, c);
                } else {
                    physAddr = mmuLookupLong<C, write>(table + 1, taddr, offset, c);
                }
            }
            break;
        }
    }

    // Check for write protection
    if constexpr (write) {
        if (c.wp) {

            throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));
        }
    }

    // Check for supervisor protection
    // TODO: THIS CAN PROBABLY BE DELETET. NO S BIT IN SHORT DESCRIPTORS
    if (c.su && !reg.sr.s) throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));

    return physAddr;
}

template <Core C, bool write> u32
Moira::mmuDryRun(u32 addr, u8 fc, u32 *mmusr)
{
    MmuContext context { .addrBits = addr, .indexBits = mmu.tc << 12 };
    context.faultAddr = addr;
    *mmusr = 0;

    // Start with the SRP or CRP
    u64 rp = (reg.sr.s && (mmu.tc & 0x02000000)) ? mmu.srp : mmu.crp;

    // +------------------------------------------------+
    // | ROOT POINTER (CRP/SRP)                         |
    // +-----+--------+---------------------------------+
    // | Bit | Length | Contents                        |
    // +-----+--------+---------------------------------+
    // | 00  |   04   | reserved ( must be 0 )          |
    // | 04  |   28   | TableA Address (upper 28 bits)  |
    // | 32  |   02   | Descriptor Type                 |
    // | 34  |   14   | reserved ( must be 0 )          |
    // | 48  |   15   | Limit                           |
    // | 63  |   01   | L/U                             |
    // +----+---------+---------------------------------+

    u32 taddr = u32(rp >> 0 ) & 0xFFFFFFF0;
    u32 dt    = u32(rp >> 32) & 0x3;
    u32 limit = u32(rp >> 48) & 0x7FFF;

    // Evaluate the limit field
    if (rp & (1LL << 63)) {

        context.lowerLimit = limit;
        context.upperLimit = 0XFFFF;

    } else {

        context.lowerLimit = 0;
        context.upperLimit = limit;
    }

    // Apply the initial shift (ignore some bits)
    (void)context.nextAddrBits();


    // Check the descriptor type
    switch (dt) {

        case 1:
        {
            /* Early termination. No translation table exists, all memory
             * accesses are calculated by adding the TableA address to the
             * logical address specified.
             */
            return taddr + addr;
        }
        case 2:
        {
            *mmusr = 1;
            bool fcl = mmu.tc & ( 1 << 24);

            if (fcl) {

                u32 offset = readFC();

                return mmuDryRunShort<C, write>(taddr, offset, context, mmusr);

            } else {

                u32 offset = context.nextAddrBits();

                return mmuDryRunShort<C, write>(taddr, offset, context, mmusr);
            }
        }
        case 3:
        {
            *mmusr = 1;
            bool fcl = mmu.tc & ( 1 << 24);

            if (fcl) {

                u32 offset = readFC();

                return mmuDryRunLong<C, write>(taddr, offset, context, mmusr);

            } else {

                u32 offset = context.nextAddrBits();

                return mmuDryRunLong<C, write>(taddr, offset, context, mmusr);
            }
        }
        default:

            // TODO: WHICH EXCEPTION? (MMU CONFIG ERROR? BUS ERROR?)
            throw BusError(makeFrame<write ? AE_WRITE : 0>(context.faultAddr));
    }
}

template <Core C, bool write> u32
Moira::mmuDryRunShort(u32 taddr, u32 offset, struct MmuContext &c, u32 *mmusr)
{
    u32 physAddr;
    char table = (*mmusr & 0b111) + 'A' - 1;

    // Check offset
    if (offset < c.lowerLimit || offset > c.upperLimit) {

        *mmusr |= MMUSR_LIMIT_VIOLATION;
        return 0;
    }

    // Read table entry
    u32 descriptor = readMMU32(taddr + 4 * offset);

    // Extract descriptor type
    u32 dt = descriptor & 0x3;

    switch (dt) {

        case 0:
        {
            // +------------------------------------------------+
            // | SHORT FORMAT INVALID DESCRIPTOR                |
            // +-----+--------+---------------------------------+
            // | Bit | Length | Contents                        |
            // +-----+--------+---------------------------------+
            // | 00  |   02   | Descriptor Type (DT)            |
            // | 02  |   30   | Unused                          |
            // +-----+--------+---------------------------------+

            *mmusr |= MMUSR_BUS_ERROR;
            return 0;
        }
        case 1:
        {
            // +------------------------------------------------+
            // | SHORT FORMAT EARLY TERM. OR PAGE DESCRIPTOR    |
            // +-----+--------+---------------------------------+
            // | Bit | Length | Contents                        |
            // +-----+--------+---------------------------------+
            // | 00  |   02   | Descriptor Type (DT)            |
            // | 02  |   01   | Write Protect (WP)              |
            // | 03  |   01   | Update (U)                      |
            // | 04  |   01   | Modified (M)                    |
            // | 05  |   01   | Reserved ( must be 0 )          |
            // | 06  |   01   | Cache Inhibit (CI)              |
            // | 07  |   01   | Reserved ( must be 0 )          |
            // | 08  |   24   | Page Address                    |
            // +-----+--------+---------------------------------+

            // Record WP and M
            if  (descriptor & (1 << 2)) *mmusr |= MMUSR_WRITE_PROTECTED;
            if  (descriptor & (1 << 4)) *mmusr |= MMUSR_MODIFIED;

            physAddr = (descriptor & 0xFFFFFF00) + c.remainingAddrBits();

            if (table == 'D') {


            } else {

            }
            break;
        }
        default:
        {
            if (table == 'D') {

                // +------------------------------------------------+
                // | SHORT FORMAT INDIRECT DESCRIPTOR               |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   30   | Descriptor Address              |
                // +-----+--------+---------------------------------+

                taddr = descriptor & 0xFFFFFFFC;


                // Increment 'number of levels' field
                *mmusr += 1;

                physAddr = mmuDryRunShort<C, write>(taddr, 0, c, mmusr);

            } else {

                // +------------------------------------------------+
                // | SHORT FORMAT TABLE DESCRIPTOR                  |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   01   | Write Protect (WP)              |
                // | 03  |   01   | Update (U)                      |
                // | 04  |   28   | Table Address                   |
                // +-----+--------+---------------------------------+

                // Record WP
                if  (descriptor & (1 << 2)) *mmusr |= MMUSR_WRITE_PROTECTED;

                u32 offset = c.nextAddrBits();

                taddr = descriptor & 0xFFFFFFF0;


                // Check offset range
                if (offset < c.lowerLimit || offset > c.upperLimit) {

                    *mmusr |= MMUSR_LIMIT_VIOLATION;
                    return 0;
                }

                c.lowerLimit = 0;
                c.upperLimit = 0xFFFF;

                // Increment 'number of levels' field
                *mmusr += 1;

                if (dt == 2) {
                    physAddr = mmuDryRunShort<C, write>(taddr, offset, c, mmusr);
                } else {
                    physAddr = mmuDryRunLong<C, write>(taddr, offset, c, mmusr);
                }
            }
            break;
        }
    }

    return physAddr;
}

template <Core C, bool write> u32
Moira::mmuDryRunLong(u32 taddr, u32 offset, struct MmuContext &c, u32 *mmusr)
{
    u32 physAddr;
    char table = (*mmusr & 0b111) + 'A' - 1;

    // Check offset
    if (offset < c.lowerLimit || offset > c.upperLimit) {


        throw BusError(makeFrame<write ? AE_WRITE : 0>(c.faultAddr));
    }

    // Read table entry
    u64 descriptor = readMMU64(taddr + 8 * offset);

    // Extract descriptor type
    u32 dt = u32(descriptor >> 32) & 0x3;

    // Evaluate the limit field
    if (descriptor & (1LL << 63)) {
        c.lowerLimit = u32(descriptor >> 48) & 0x7FFF;
        c.upperLimit = 0xFFFF;
    } else {
        c.lowerLimit = 0;
        c.upperLimit = u32(descriptor >> 48) & 0x7FFF;
    }

    switch (dt) {

        case 0:
        {
            // +------------------------------------------------+
            // | LONG FORMAT INVALID DESCRIPTOR                 |
            // +-----+--------+---------------------------------+
            // | Bit | Length | Contents                        |
            // +-----+--------+---------------------------------+
            // | LongWord0                                      |
            // +-----+--------+---------------------------------+
            // | 00  |   02   | Descriptor Type (DT)            |
            // | 02  |   30   | Unused                          |
            // +-----+--------+---------------------------------+
            // | LongWord1                                      |
            // +-----+--------+---------------------------------+
            // | 00  |   32   | Unused                          |
            // +-----+--------+---------------------------------+

            *mmusr |= MMUSR_BUS_ERROR;
            return 0;
        }
        case 1:
        {
            // Record WP, M and S
            if  (descriptor & (1 << 2)) *mmusr |= MMUSR_WRITE_PROTECTED;
            if  (descriptor & (1 << 4)) *mmusr |= MMUSR_MODIFIED;
            if  (descriptor & (1 << 8)) *mmusr |= MMUSR_SUPERVISOR_ONLY;

            physAddr = (descriptor & 0xFFFFFF00) + c.remainingAddrBits();

            if (table == 'D') {

                // +------------------------------------------------+
                // | LONG FORMAT PAGE DESCRIPTOR                    |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | LongWord0                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   01   | Write Protect (WP)              |
                // | 03  |   01   | Update (U)                      |
                // | 04  |   01   | Modified (M)                    |
                // | 05  |   01   | Reserved ( must be 0 )          |
                // | 06  |   01   | Cache Inhibit (CI)              |
                // | 07  |   01   | Reserved ( must be 0 )          |
                // | 08  |   01   | Supervisor (S)                  |
                // | 09  |   07   | Reserved ( must be 1111110 )    |
                // | 16  |   16   | Unused                          |
                // +-----+--------+---------------------------------+
                // | LongWord1                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   07   | Unused                          |
                // | 08  |   24   | Page Address                    |
                // +-----+--------+---------------------------------+


            } else {

                // +------------------------------------------------+
                // | LONG FORMAT EARLY TERMINATION PAGE DESCRIPTOR  |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | LongWord0                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   01   | Write Protect (WP)              |
                // | 03  |   01   | Update (U)                      |
                // | 04  |   01   | Modified (M)                    |
                // | 05  |   01   | Reserved ( must be 0 )          |
                // | 06  |   01   | Cache Inhibit (CI)              |
                // | 07  |   01   | Reserved ( must be 0 )          |
                // | 08  |   01   | Supervisor (S)                  |
                // | 09  |   07   | Reserved ( must be 1111110 )    |
                // | 16  |   15   | Limit                           |
                // | 31  |   01   | L/U                             |
                // +-----+--------+---------------------------------+
                // | LongWord1                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   07   | Unused                          |
                // | 08  |   24   | Page Address                    |
                // +-----+--------+---------------------------------+

            }
            break;
        }
        default:
        {
            if (table == 'D') {

                // +------------------------------------------------+
                // | LONG FORMAT INDIRECT DESCRIPTOR                |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | LongWord0                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   30   | Unused                          |
                // +-----+--------+---------------------------------+
                // | LongWord1                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Unused                          |
                // | 02  |   30   | Descriptor Address              |
                // +-----+--------+---------------------------------+

                taddr = descriptor & 0xFFFFFFFC;


                // Increment 'number of levels' field
                *mmusr += 1;

                physAddr = mmuDryRunLong<C, write>(taddr, 0, c, mmusr);

            } else {

                // +------------------------------------------------+
                // | LONG FORMAT TABLE DESCRIPTOR                   |
                // +-----+--------+---------------------------------+
                // | Bit | Length | Contents                        |
                // +-----+--------+---------------------------------+
                // | LongWord0                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   02   | Descriptor Type (DT)            |
                // | 02  |   01   | Write Protect (WP)              |
                // | 03  |   01   | Update (U)                      |
                // | 04  |   04   | Reserved ( must be 0000 )       |
                // | 08  |   01   | Supervisor (S)                  |
                // | 09  |   07   | Reserved ( must be 1111110 )    |
                // | 16  |   15   | Limit                           |
                // | 31  |   01   | L/U                             |
                // +-----+--------+---------------------------------+
                // | LongWord1                                      |
                // +-----+--------+---------------------------------+
                // | 00  |   04   | Unused                          |
                // | 04  |   28   | Table Address                   |
                // +-----+--------+---------------------------------+

                // Record WP and S
                if  (descriptor & (1 << 2)) *mmusr |= MMUSR_WRITE_PROTECTED;
                if  (descriptor & (1 << 8)) *mmusr |= MMUSR_SUPERVISOR_ONLY;

                u32 offset = c.nextAddrBits();

                taddr = descriptor & 0xFFFFFFF0;


                // Check offset range
                if (offset < c.lowerLimit || offset > c.upperLimit) {

                    *mmusr |= MMUSR_LIMIT_VIOLATION;
                    return 0;
                }

                if constexpr (write) { c.wp |= (descriptor >> 32) & 0x4; }
                c.lowerLimit = 0;
                c.upperLimit = 0xFFFF;

                // Increment 'number of levels' field
                *mmusr += 1;

                if (dt == 2) {
                    physAddr = mmuDryRunShort<C, write>(taddr, offset, c);
                } else {
                    physAddr = mmuLookupLong<C, write>(taddr, offset, c);
                }
            }
            break;
        }
    }

    return physAddr;
}


//
// Instruction handlers
//

template <Core C, Instr I, Mode M, Size S> void
Moira::execPGen(u16 opcode)
{
    auto ext = queue.irc;

    // PLOAD: 0010 00x0 000x xxxx
    if ((ext & 0xFDE0) == 0x2000) {

        execPLoad<C, Instr::PLOAD, M, S>(opcode);
        return;
    }

    // PFLUSHA: 0010 010x xxxx xxxx
    if ((ext & 0xFE00) == 0x2400) {

        execPFlusha<C, Instr::PFLUSHA, M, S>(opcode);
        return;
    }

    // PFLUSH: 001x xx0x xxxx xxxx
    if ((ext & 0xE200) == 0x2000) {

        execPFlush<C, Instr::PFLUSH, M, S>(opcode);
        return;
    }

    // PTEST: 100x xxxx xxxx xxxx
    if ((ext & 0xE000) == 0x8000) {

        execPTest<C, Instr::PTEST, M, S>(opcode);
        return;
    }

    // PMOVE: 010x xxxx 0000 0000 || 0110 00x0 0000 0000 || 000x xxxx 0000 0000
    if ((ext & 0xE0FF) == 0x4000 || (ext & 0xFDFF) == 0x6000 || (ext & 0xE0FF) == 0x0000) {

        execPMove<C, Instr::PMOVE, M, S>(opcode);
        return;
    }

    execIllegal<C, I, M, S>(opcode);
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPFlush(u16 opcode)
{
    AVAILABILITY(Core::C68020)


    u16 ext   = queue.irc;
    auto reg  = _____________xxx (opcode);
    auto mode = ___xxx__________ (ext);

    // Catch illegal extension words
    if (!isValidExtMMU(I, M, opcode, ext)) {

        execIllegal<C, Instr::ILLEGAL, M, S>(opcode);
        return;
    }

    (void)readI<C, Word>();

    if (mode == 0b110) {
        
        u32 ea; u32 data;
        readOp<C, M>(reg, &ea, &data);
    }
    
    prefetch<C, POLL>();

    CYCLES_68020(8);
    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPFlusha(u16 opcode)
{
    AVAILABILITY(Core::C68020)


    u16 ext = queue.irc;

    // Catch illegal extension words
    if (!isValidExtMMU(I, M, opcode, ext)) {

        execIllegal<C, Instr::ILLEGAL, M, S>(opcode);
        return;
    }

    (void)readI<C, Word>();
    
    prefetch<C, POLL>();

    CYCLES_68020(8);
    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPFlush40(u16 opcode)
{
    AVAILABILITY(Core::C68020)


    auto reg  = _____________xxx (opcode);
    auto mode = ___________xx___ (opcode);
    

    switch (mode) {

        case 0:
        case 1:
                        
            u32 ea; u32 data;
            readOp<C, M>(reg, &ea, &data);
            break;
            
        case 2:
        case 3:

            break;
    }

    prefetch<C, POLL>();

    CYCLES_68020(8);
    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPLoad(u16 opcode)
{
    AVAILABILITY(Core::C68020)

    u16 ext   = queue.irc;
    auto reg  = _____________xxx (opcode);
    auto rw   = ______x_________ (ext);

    if (rw) {
    } else {
    }
    
    // Catch illegal extension words
    if (!isValidExtMMU(I, M, opcode, ext)) {

        execIllegal<C, Instr::ILLEGAL, M, S>(opcode);
        return;
    }

    (void)readI<C, Word>();
        
    u32 ea; u32 data;
    readOp<C, M>(reg, &ea, &data);
    
    prefetch<C, POLL>();
    
    CYCLES_68020(8);
    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPMove(u16 opcode)
{
    AVAILABILITY(Core::C68020)

    u16 ext = queue.irc;
    auto fmt  = xxx_____________ (ext);
    auto preg = ___xxx__________ (ext);
    bool rw   = ______x_________ (ext);

    // Catch illegal extension words
    if (!isValidExtMMU(I, M, opcode, ext)) {

        execIllegal<C, Instr::ILLEGAL, M, S>(opcode);
        return;
    }

    (void)readI<C, Word>();

    switch (fmt) {

        case 0:

            switch (preg) {

                case 0b010: execPMove<C, M>(opcode, MmuReg::TT0, rw); break;
                case 0b011: execPMove<C, M>(opcode, MmuReg::TT1, rw); break;
            }
            break;

        case 2:

            switch (preg) {

                case 0b000: execPMove<C, M>(opcode, MmuReg::TC, rw); break;
                case 0b010: execPMove<C, M>(opcode, MmuReg::SRP, rw); break;
                case 0b011: execPMove<C, M>(opcode, MmuReg::CRP, rw); break;
            }
            break;

        case 3:

            switch (preg) {

                case 0b000: execPMove<C, M>(opcode, MmuReg::MMUSR, rw); break;
            }
            break;
    }

    prefetch<C, POLL>();

    CYCLES_68020(8);
    FINALIZE
}

template <Core C, Mode M> void
Moira::execPMove(u16 opcode, MmuReg mmuReg, bool rw)
{
    u32 ea;
    u32 data32;
    u64 data64;

    auto reg  = _____________xxx (opcode);

    if (rw) {

        switch (mmuReg) {

            case MmuReg::MMUSR:

                writeOp<C, M, Word>(reg, mmu.mmusr);
                break;

            case MmuReg::TT0:

                writeOp<C, M, Long>(reg, mmu.tt0);
                break;

            case MmuReg::TT1:

                writeOp<C, M, Long>(reg, mmu.tt1);
                break;

            case MmuReg::TC:

                writeOp<C, M, Long>(reg, mmu.tc);
                break;

            case MmuReg::CRP:

                writeOp64<C, M>(reg, mmu.crp);
                break;

            case MmuReg::SRP:

                writeOp64<C, M>(reg, mmu.srp);
                break;

            default:
                fatalError;
        }

    } else {

        switch (mmuReg) {

            case MmuReg::MMUSR:

                readOp<C, M, Word>(reg, &ea, &data32);
                mmu.mmusr = u16(data32);
                break;

            case MmuReg::TT0:

                readOp<C, M, Long>(reg, &ea, &data32);
                mmu.tt0 = data32;
                break;

            case MmuReg::TT1:

                readOp<C, M, Long>(reg, &ea, &data32);
                mmu.tt1 = data32;
                break;

            case MmuReg::TC:

                readOp<C, M, Long>(reg, &ea, &data32);
                setTC(data32);
                break;

            case MmuReg::CRP:

                readOp64<C, M>(reg, &ea, &data64);
                mmu.crp = data64;
                break;

            case MmuReg::SRP:

                readOp64<C, M>(reg, &ea, &data64);
                mmu.srp = data64;
                break;

            default:
                fatalError;
        }
    }
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPTest(u16 opcode)
{

    AVAILABILITY(Core::C68020)
    
    u16 ext  = queue.irc;
    auto rg  = _____________xxx (opcode);
    auto fc  = ___________xxxxx (ext);
    auto rw  = ______x_________ (ext);

    // Catch illegal extension words
    if (!isValidExtMMU(I, M, opcode, ext)) {

        execIllegal<C, Instr::ILLEGAL, M, S>(opcode);
        return;
    }

    (void)readI<C, Word>();

    u32 ea = 0;

    // Compute effective address
    ea = computeEA<C, M, Long>(rg);


    u8 fcode;
    if (fc == 0) { fcode = (u8)reg.sfc; }
    else if (fc == 1) { fcode = (u8)reg.dfc; }
    else if (fc & 0b01000) { fcode = readD<Long>(fc & 0b111) & 0b111; }
    else { fcode = fc & 0b111; }
    
    if (testTT(mmu.tt0, ea, fcode, rw) || testTT(mmu.tt1, ea, fcode, rw)) {
        mmu.mmusr |= 0x40;
    } else {
        mmu.mmusr &= ~0x40;
    }
    
    prefetch<C, POLL>();

    CYCLES_68020(8);
    FINALIZE
}

template <Core C, Instr I, Mode M, Size S> void
Moira::execPTest40(u16 opcode)
{
    AVAILABILITY(Core::C68020)
    
    auto rg  = _____________xxx (opcode);
    auto rw  = __________x_____ (opcode);
        
    if (rw) {
    } else {
    }

    u32 ea = 0; u32 data;
    readOp<C, M, Long>(rg, &ea, &data);
    
    u8 fcode = u8(reg.dfc);
    if (testTT(mmu.tt0, ea, fcode, rw) || testTT(mmu.tt1, ea, fcode, rw)) {
        mmu.mmusr |= 0x40;
    } else {
        mmu.mmusr &= ~0x40;
    }
    
    prefetch<C, POLL>();

    CYCLES_68020(8);
    FINALIZE
}
