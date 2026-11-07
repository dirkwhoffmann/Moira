// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

// Thrown to terminate a table walk
struct MmuWalkStop { };

// MMU context used while walking the translation tables
struct MmuContext {

    u64 addrBits;
    u64 indexBits;
    u8  shiftCnt;

    u32 lowerLimit;
    u32 upperLimit;

    bool wp;
    bool su;

    // Properties of the access
    bool super;
    u8 fc;

    // Dry run (PTEST): Walk the tables without side effects. If 'level' is
    // non-zero, the walk ends at the descriptor with that number.
    bool ptest;
    u32 level;

    // Number of descriptors fetched so far and the address of the last one
    u32 depth;
    u32 descrAddr;

    // Status bits collected during the walk
    u32 flags;
    bool modified;

    // Page size mask and the letter of the last indexed table
    u32 pageMask;
    char lastTable;

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

    // Terminates the walk
    [[noreturn]] void stop(u32 bits = 0) { flags |= bits; throw MmuWalkStop(); }

    // Returns the MMU status register value
    u32 status() {

        return flags
        | (wp ? MMUSR_WRITE_PROTECTED : 0)
        | (violation() ? MMUSR_SUPERVISOR_ONLY : 0)
        | (modified ? MMUSR_MODIFIED : 0)
        | (depth & MMUSR_NUMBER_OF_LEVELS);
    }

    // Indicates a supervisor violation (supervisor-only descriptor, user access)
    bool violation() { return su && !super; }

    // Returns the next index without consuming it
    u32 peekAddrBits() {

        MmuContext copy = *this;
        return copy.nextAddrBits();
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

void
Moira::atcTouch(int line)
{
    // Mark the entry as recently used. If all history bits are set, clear all but this one.
    mmu.atc[line].mru = true;

    for (auto &e : mmu.atc) if (!e.mru) return;
    for (auto &e : mmu.atc) e.mru = false;
    mmu.atc[line].mru = true;
}

int
Moira::atcFind(u32 addr, u8 fc, bool write)
{
    u32 page = addr & ~pageMask();

    for (int i = 0; i < ATC_ENTRIES; i++) {

        auto &e = mmu.atc[i];
        if (!e.valid || e.logical != page || e.fc != fc) continue;

        // A write access to a page that hasn't been marked as modified yet
        // needs a table walk to set the M bit. The entry is invalidated.
        if (!write || e.modified || e.writeProtect || e.busError) {

            atcTouch(i);
            return i;
        }
        e.valid = false;
    }
    return -1;
}

int
Moira::atcInsert(u32 addr, u8 fc, u32 physical, u32 status)
{
    // Take the first free entry. If there is none, replace the first entry without history bit.
    int line = 0;
    for (; line < ATC_ENTRIES && mmu.atc[line].valid; line++) { }
    if (line == ATC_ENTRIES) {
        for (line = 0; line < ATC_ENTRIES && mmu.atc[line].mru; line++) { }
        if (line == ATC_ENTRIES) line = 0;
    }
    atcTouch(line);

    auto &e = mmu.atc[line];
    e.logical = addr & ~pageMask();
    e.physical = physical & ~pageMask();
    e.fc = fc;
    e.valid = true;
    e.busError = status & (MMUSR_INVALID | MMUSR_SUPERVISOR_ONLY);
    e.writeProtect = status & MMUSR_WRITE_PROTECTED;
    e.modified = status & MMUSR_MODIFIED;
    return line;
}

void
Moira::atcFlush()
{
    for (auto &e : mmu.atc) e.valid = false;
}

void
Moira::atcFlush(u8 fcBase, u8 fcMask)
{
    for (auto &e : mmu.atc) if ((fcBase & fcMask) == (e.fc & fcMask)) e.valid = false;
}

void
Moira::atcFlushPage(u32 addr)
{
    for (auto &e : mmu.atc) if (e.logical == (addr & ~pageMask())) e.valid = false;
}

void
Moira::atcFlushPage(u32 addr, u8 fcBase, u8 fcMask)
{
    for (auto &e : mmu.atc) {
        if (e.logical == (addr & ~pageMask()) && (fcBase & fcMask) == (e.fc & fcMask)) e.valid = false;
    }
}

u8
Moira::mmuFC(u16 ext) const
{
    switch (ext & 0x18) {

        case 0x10: return ext & 7;
        case 0x08: return reg.d[ext & 7] & 7;
        default:   return (ext & 1) ? u8(reg.dfc) : u8(reg.sfc);
    }
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

    // Accesses to the CPU space and accesses matching a transparent translation
    // register are not translated
    if (fc == 7 || testTT(mmu.tt0, addr, fc, !write) || testTT(mmu.tt1, addr, fc, !write)) return addr;

    //
    // TODO: LOOKUP IN CACHE
    //

    return mmuLookup<C, write>(addr, fc);
}

template <Core C> bool
Moira::translateAddress(u32 addr, u8 fc, bool write, u32 &physical)
{
    try {
        physical = write ? translate<C, true>(addr, fc) : translate<C, false>(addr, fc);
        return true;
    } catch (const BusError &) {
        return false;
    }
}

template <Core C, bool write> u32
Moira::mmuLookup(u32 addr, u8 fc)
{
    int line = atcFind(addr, fc, write);

    if (line < 0) {

        // The translation is not cached. Walk the tables and create an ATC entry.
        MmuContext context;
        u32 physAddr = mmuWalk<C, write>(addr, fc, context, false, 0);
        line = atcInsert(addr, fc, physAddr, context.status());
    }
    auto &e = mmu.atc[line];

    // Invalid descriptors and supervisor violations cause a bus error. So do
    // write accesses to write-protected pages.
    if (e.busError || (write && e.writeProtect)) {

        throw BusError(makeFrame<write ? AE_WRITE : 0>(addr));
    }
    return e.physical | (addr & pageMask());
}

template <Core C> void
Moira::mmuLoad(u32 addr, u8 fc, bool write)
{
    atcFlushPage(addr);

    MmuContext context;
    u32 physAddr = write ? mmuWalk<C, true>(addr, fc, context, false, 0)
                         : mmuWalk<C, false>(addr, fc, context, false, 0);
    atcInsert(addr, fc, physAddr, context.status());
}

template <Core C> u32
Moira::testAddress(u32 addr, u8 fc, bool write, u32 level, u32 &descrAddr)
{
    return mmuPTest<C>(addr, fc, write, level, &descrAddr);
}

template <Core C> u32
Moira::mmuPTest(u32 addr, u8 fc, bool write, u32 level, u32 *descrAddr)
{
    *descrAddr = 0;

    // Level 0 searches the ATC
    if (level == 0) {

        if (testTT(mmu.tt0, addr, fc, !write) || testTT(mmu.tt1, addr, fc, !write)) {
            return MMUSR_TRANSPARENT_ACCESS;
        }

        u32 page = addr & ~pageMask();
        for (auto &e : mmu.atc) {

            if (!e.valid || e.logical != page || e.fc != fc) continue;

            return (e.busError ? MMUSR_BUS_ERROR | MMUSR_INVALID : 0)
                 | (e.writeProtect ? MMUSR_WRITE_PROTECTED : 0)
                 | (e.modified ? MMUSR_MODIFIED : 0);
        }
        return MMUSR_INVALID;
    }

    MmuContext context;
    (void)mmuWalk<C, false>(addr, fc, context, true, level);

    if (context.depth) *descrAddr = context.descrAddr;
    return context.status();
}

template <Core C, bool write> u32
Moira::mmuWalk(u32 addr, u8 fc, MmuContext &c, bool ptest, u32 level)
{
    c = MmuContext { .addrBits = addr, .indexBits = mmu.tc << 12 };
    c.faultAddr = addr;
    c.super = fc & 4;
    c.fc = fc & 7;
    c.pageMask = (1u << (mmu.tc >> 20 & 0xF)) - 1;
    c.ptest = ptest;
    c.level = level;

    bool fcl = mmu.tc & (1 << 24);

    // Determine the last table. A table is indexed by each non-zero TIx field.
    // If function code lookup is enabled, there is an additional table.
    int tables = 0;
    while (tables < 4 && (mmu.tc >> (12 - 4 * tables) & 0xF)) tables++;
    c.lastTable = char('A' + tables - 1 + (fcl ? 1 : 0));

    // Start with the SRP or CRP
    u64 rp = (c.super && (mmu.tc & 0x02000000)) ? mmu.srp : mmu.crp;

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

        c.lowerLimit = limit;
        c.upperLimit = 0XFFFF;

    } else {

        c.lowerLimit = 0;
        c.upperLimit = limit;
    }

    // Apply the initial shift (ignore some bits)
    (void)c.nextAddrBits();

    try {

        // Check the descriptor type
        switch (dt) {

            case 1:
            {
                /* Early termination. No translation table exists. The physical
                 * address is the sum of the TableA address and the logical address
                 * (without the ignored bits). The limit applies to the first index
                 * unless function code lookup is enabled.
                 */
                if (!fcl) {

                    u32 index = c.peekAddrBits();
                    if (index < c.lowerLimit || index > c.upperLimit) {
                        c.stop(MMUSR_LIMIT_VIOLATION | MMUSR_INVALID);
                    }
                }
                return (u32(rp) & 0xFFFFFF00 & ~c.pageMask) + c.remainingAddrBits();
            }
            case 2:
            case 3:
            {
                u32 offset;

                if (fcl) {

                    // The function code is used as the first index (no limit check)
                    offset = c.fc;
                    c.lowerLimit = 0;
                    c.upperLimit = 0xFFFF;

                } else {

                    offset = c.nextAddrBits();
                }

                return dt == 2 ? mmuLookupShort<C, write>('A', taddr, offset, c)
                               : mmuLookupLong<C, write>('A', taddr, offset, c);
            }
            default:

                // TODO: WHICH EXCEPTION? (MMU CONFIG ERROR? BUS ERROR?)
                c.stop(MMUSR_INVALID);
        }

    } catch (const MmuWalkStop &) { }

    return 0;
}

template <Core C, bool write> u32
Moira::mmuLookupShort(char table, u32 taddr, u32 offset, struct MmuContext &c)
{
    u32 physAddr;

    // Check offset
    if (offset < c.lowerLimit || offset > c.upperLimit) {

        c.stop(MMUSR_LIMIT_VIOLATION | MMUSR_INVALID);
    }

    // Read table entry
    u32 descrAddr = taddr + 4 * offset;
    u32 descriptor = readMMU32(descrAddr);
    c.depth++;
    c.descrAddr = descrAddr;

    // Extract descriptor type
    u32 dt = descriptor & 0x3;

    switch (dt) {

        case 0:
        {
            // +------------------------------------------------+
            // | SHORT FORMAT INVALID DESCRIPTOR                |
            // +-----+--------+---------------------------------+
            // | 00  |   02   | Descriptor Type (DT)            |
            // | 02  |   30   | Unused                          |
            // +-----+--------+---------------------------------+

            c.stop(MMUSR_INVALID);
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
            // | 03  |   01   | Used (U)                        |
            // | 04  |   01   | Modified (M)                    |
            // | 05  |   01   | Reserved                        |
            // | 06  |   01   | Cache Inhibit (CI)              |
            // | 07  |   01   | Reserved                        |
            // | 08  |   24   | Page Address                    |
            // +-----+--------+---------------------------------+

            // Record WP
            c.wp |= descriptor & 0x4;

            // Update the U bit and the M bit (the latter only if the page is writable)
            if (!c.ptest && !c.violation()) {

                u32 desc2 = descriptor | 0x8;
                if constexpr (write) { if (!c.wp) desc2 |= 0x10; }
                if (desc2 != descriptor) write16(descrAddr + 2, (u16)desc2);
                descriptor = desc2;
            }
            if (descriptor & 0x10) c.modified = true;

            // The page offset consists of the unused index bits and the page offset
            physAddr = (descriptor & 0xFFFFFF00 & ~c.pageMask) + c.remainingAddrBits();
            break;
        }
        default:
        {
            // +------------------------------------------------+
            // | SHORT FORMAT TABLE DESCRIPTOR                  |
            // +-----+--------+---------------------------------+
            // | Bit | Length | Contents                        |
            // +-----+--------+---------------------------------+
            // | 00  |   02   | Descriptor Type (DT)            |
            // | 02  |   01   | Write Protect (WP)              |
            // | 03  |   01   | Used (U)                        |
            // | 04  |   28   | Table Address                   |
            // +-----+--------+---------------------------------+

            if (table > c.lastTable) {

                // An indirect descriptor must point to a page descriptor
                c.stop(MMUSR_INVALID);
            }

            if (table == c.lastTable) {

                // Indirect descriptor. The pointer is used as is. Neither
                // the protection bits nor the U bit are evaluated.
                if (c.level == c.depth) c.stop();

                c.lowerLimit = 0;
                c.upperLimit = 0xFFFF;
                taddr = descriptor & 0xFFFFFFFC;

                physAddr = dt == 2 ? mmuLookupShort<C, write>(table + 1, taddr, 0, c)
                                   : mmuLookupLong<C, write>(table + 1, taddr, 0, c);

            } else {

                // Record WP
                c.wp |= descriptor & 0x4;

                // Update the U bit
                if (!c.ptest && !c.violation() && !(descriptor & 0x8)) write16(descrAddr + 2, (u16)(descriptor | 0x8));

                // Stop here if PTEST asked for this level
                if (c.level == c.depth) c.stop();

                offset = c.nextAddrBits();
                taddr = descriptor & 0xFFFFFFF0;

                // Short descriptors don't carry a limit
                c.lowerLimit = 0;
                c.upperLimit = 0xFFFF;

                physAddr = dt == 2 ? mmuLookupShort<C, write>(table + 1, taddr, offset, c)
                                   : mmuLookupLong<C, write>(table + 1, taddr, offset, c);
            }
            break;
        }
    }

    return physAddr;
}

template <Core C, bool write> u32
Moira::mmuLookupLong(char table, u32 taddr, u32 offset, struct MmuContext &c)
{
    u32 physAddr;

    // Check offset
    if (offset < c.lowerLimit || offset > c.upperLimit) {

        c.stop(MMUSR_LIMIT_VIOLATION | MMUSR_INVALID);
    }

    // Read table entry
    u32 descrAddr = taddr + 8 * offset;
    u64 descriptor = readMMU64(descrAddr);
    c.depth++;
    c.descrAddr = descrAddr;

    // The upper word contains the flags, the lower word the address
    u32 flags = u32(descriptor >> 32);
    u32 addr  = u32(descriptor);

    // Extract descriptor type
    u32 dt = flags & 0x3;

    // +------------------------------------------------+
    // | LONG FORMAT DESCRIPTORS                        |
    // +-----+--------+---------------------------------+
    // | Bit | Length | Contents                        |
    // +-----+--------+---------------------------------+
    // | 00  |   02   | Descriptor Type (DT)            |
    // | 02  |   01   | Write Protect (WP)              |
    // | 03  |   01   | Used (U)                        |
    // | 04  |   01   | Modified (M)  (page only)       |
    // | 06  |   01   | Cache Inhibit (CI) (page only)  |
    // | 08  |   01   | Supervisor Only (S)             |
    // | 16  |   15   | Limit                           |
    // | 31  |   01   | L/U                             |
    // | 32  |   32   | Table or page address           |
    // +-----+--------+---------------------------------+

    if (dt == 0) c.stop(MMUSR_INVALID);

    if (dt > 1 && table > c.lastTable) {

        // An indirect descriptor must point to a page descriptor
        c.stop(MMUSR_INVALID);
    }

    if (dt > 1 && table == c.lastTable) {

        // Indirect descriptor. The pointer is used as is. Neither
        // the protection bits nor the U bit are evaluated.
        if (c.level == c.depth) c.stop();

        c.lowerLimit = 0;
        c.upperLimit = 0xFFFF;
        taddr = addr & 0xFFFFFFFC;

        physAddr = dt == 2 ? mmuLookupShort<C, write>(table + 1, taddr, 0, c)
                           : mmuLookupLong<C, write>(table + 1, taddr, 0, c);

    } else {

        // Record WP and S bit
        c.wp |= flags & 0x4;
        c.su |= flags & 0x100;

        if (dt == 1) {

            // Page descriptor (or early termination)
            if (!c.ptest && !c.violation()) {

                u32 flags2 = flags | 0x8;
                if constexpr (write) { if (!c.wp) flags2 |= 0x10; }
                if (flags2 != flags) write16(descrAddr + 2, (u16)flags2);
                flags = flags2;
            }
            if (flags & 0x10) c.modified = true;

            // In case of early termination, the limit applies to the next index
            if (table < c.lastTable) {

                u32 index = c.peekAddrBits();
                u32 limit = flags >> 16 & 0x7FFF;
                if ((flags & 0x80000000) ? index < limit : index > limit) {
                    c.stop(MMUSR_LIMIT_VIOLATION | MMUSR_INVALID);
                }
            }

            physAddr = (addr & 0xFFFFFF00 & ~c.pageMask) + c.remainingAddrBits();

        } else {

            // Table descriptor. Update the U bit.
            if (!c.ptest && !c.violation() && !(flags & 0x8)) write16(descrAddr + 2, (u16)(flags | 0x8));

            // Stop here if PTEST asked for this level
            if (c.level == c.depth) c.stop();

            offset = c.nextAddrBits();
            taddr = addr & 0xFFFFFFF0;

            // The limit applies to the next index
            u32 limit = flags >> 16 & 0x7FFF;
            if (flags & 0x80000000) { c.lowerLimit = limit; c.upperLimit = 0xFFFF; }
            else                    { c.lowerLimit = 0; c.upperLimit = limit; }

            physAddr = dt == 2 ? mmuLookupShort<C, write>(table + 1, taddr, offset, c)
                               : mmuLookupLong<C, write>(table + 1, taddr, offset, c);
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
    auto mask = ________xxx_____ (ext);

    // Catch illegal extension words
    if (!isValidExtMMU(I, M, opcode, ext)) {

        execIllegal<C, Instr::ILLEGAL, M, S>(opcode);
        return;
    }

    (void)readI<C, Word>();

    u8 fcBase = mmuFC(ext);

    if (mode == 0b110) {

        // Flush by function code and effective address
        u32 ea = computeEA<C, M, Long>(reg);
        atcFlushPage(ea, fcBase, mask);

    } else {

        // Flush by function code
        atcFlush(fcBase, mask);
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

    atcFlush();

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

    // Catch illegal extension words
    if (!isValidExtMMU(I, M, opcode, ext)) {

        execIllegal<C, Instr::ILLEGAL, M, S>(opcode);
        return;
    }

    (void)readI<C, Word>();

    u32 ea = computeEA<C, M, Long>(reg);
    mmuLoad<C>(ea, mmuFC(ext), !rw);

    prefetch<C, POLL>();

    CYCLES_68020(8);
    FINALIZE
}

// Checks if a PMOVE instruction can be executed (stricter than the disassembler check)
template <Mode M> static bool
isExecutablePMove(u16 ext)
{
    auto fmt  = ext >> 13 & 0b111;
    auto preg = ext >> 10 & 0b111;
    bool rw   = ext & 0x200;
    bool fd   = ext & 0x100;

    // Only memory and control alterable addressing modes are allowed
    if (M == Mode::DN || M == Mode::AN || M == Mode::PI || M == Mode::PD ||
        M == Mode::DIPC || M == Mode::IXPC || M == Mode::IM || M == Mode::IP) return false;

    // The lower byte must be zero, and reading a register can't flush the ATC
    if ((ext & 0xFF) || (rw && fd)) return false;

    switch (fmt) {

        case 0:  return preg == 2 || preg == 3;
        case 2:  return preg == 0 || preg == 2 || preg == 3;
        case 3:  return preg == 0 && !fd;
        default: return false;
    }
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
    if (!isValidExtMMU(I, M, opcode, ext) || !isExecutablePMove<M>(ext)) {

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

    // Writing a translation register flushes the ATC unless the FD bit is set
    if (!rw && fmt != 3 && !(ext & 0x100)) atcFlush();

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
    
    u16 ext   = queue.irc;
    auto rg   = _____________xxx (opcode);
    auto fc   = ___________xxxxx (ext);
    auto rw   = ______x_________ (ext);
    auto a    = _______x________ (ext);
    auto areg = ________xxx_____ (ext);
    auto lvl  = ___xxx__________ (ext);

    // Catch illegal extension words. Level 0 doesn't support the A bit and
    // function code field 11xxx is reserved.
    if (!isValidExtMMU(I, M, opcode, ext) || (lvl == 0 && a) || (fc & 0b11000) == 0b11000) {

        execIllegal<C, Instr::ILLEGAL, M, S>(opcode);
        return;
    }

    (void)readI<C, Word>();

    // Compute effective address
    u32 ea = computeEA<C, M, Long>(rg);

    // u8 fcode = mmuFC(ext);

    u32 descrAddr;
    mmu.mmusr = mmuPTest<C>(ea, mmuFC(ext), !rw, lvl, &descrAddr);
    if (a) writeA(areg, descrAddr);

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
