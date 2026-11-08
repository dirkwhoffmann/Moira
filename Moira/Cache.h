// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "MoiraTypes.h"

namespace MOIRA_NAMESPACE {

/* On-chip caches
 *
 * The 68020 has an instruction cache of 64 longwords. The 68030 has an
 * instruction cache and a data cache of 256 bytes each, organized as 16 lines
 * of four longwords. The 68040 models use the 68030 layout for the instruction
 * cache. A data cache is only emulated for the 68030. See below for details
 * on the organization.
 *
 * Both caches are addressed logically. A hit is served without involving the
 * MMU, which means that it can neither fault nor update the translation tables.
 * Only a miss translates the address, which also reveals the cache-inhibit
 * attribute of the page. Inhibited accesses bypass the caches.
 *
 * The Cache Control Register (CACR) controls the caches:
 *
 *   68020                              68030
 *   Bit 0 (E)  Enable                  Bit 0 (EI)  Enable instruction cache
 *   Bit 1 (F)  Freeze                  Bit 1 (FI)  Freeze instruction cache
 *   Bit 2 (CE) Clear entry             Bit 2 (CEI) Clear instruction cache entry
 *   Bit 3 (C)  Clear                   Bit 3 (CI)  Clear instruction cache
 *                                      Bit 4 (IBE) Enable instruction burst fills
 *                                      Bit 8 (ED)  Enable data cache
 *                                      Bit 9 (FD)  Freeze data cache
 *                                      Bit 10 (CED) Clear data cache entry
 *                                      Bit 11 (CD) Clear data cache
 *                                      Bit 12 (DBE) Enable data burst fills
 *                                      Bit 13 (WA) Write allocate
 *
 * Clearing the enable bit bypasses the cache entirely. Lookups never hit and
 * fills never happen. This has to be checked on every lookup, not just on fill,
 * or the CPU would keep executing stale opcodes after a program disables the
 * cache mid-flight. Freezing stops new entries from being allocated, but entries
 * that are already valid keep being served as hits (and are updated by writes).
 * The clear-entry bits invalidate the longword selected by the CAAR register.
 *
 * A miss reads the missing longword. If burst fills are enabled, the cache is
 * not frozen and the port is 32 bits wide, the remaining longwords of the
 * line are fetched right behind it (in wrap-around order). A burst fill is
 * only started if the missed longword is the line's only valid entry. A narrower
 * port needs two or four smaller transactions per longword and doesn't burst.
 *
 * The data cache is write-through. Every write reaches the bus. A write hit
 * updates the cached bytes. With write allocate (WA), an aligned longword write
 * allocates the entry. Writes never allocate in an inhibited page.
 *
 * Moira additionally keeps a one-entry "latch" (CacheLatch) in front of the
 * instruction cache. It records the longword last fetched, keyed by its base
 * address; a hit on the latch needs no cache lookup and no bus cycle at all,
 * modeling the CPU re-reading the same longword for the high and low
 * instruction word of a single fetch, or for consecutive fetches that
 * happen to land in the same longword.
 */

/* On-chip cache of the 68020, 68030 and 68040 models
 *
 * The caches are direct-mapped. An address selects a line by its index bits and
 * a longword inside the line by its longword bits. The tag contains the upper
 * 24 address bits and the supervisor bit, which keeps user and supervisor code
 * from aliasing the same line:
 *
 *   tag = xxxxxxxx xxxxxxxx xxxxxxxx -------S     ( (addr & ~0xFF) | S )
 *
 * Tags are logical addresses. A lookup never involves the MMU.
 *
 * Two layouts exist. They differ in the line size only:
 *
 *   68020:       64 lines of one longword    index = addr[7:2]
 *   68030/040:   16 lines of four longwords  index = addr[7:4], longword = addr[3:2]
 *
 * Each longword has its own valid bit, so a line can be partially filled, e.g.,
 * after a miss that is served by a single longword transfer. Allocating a
 * longword in a line with a different tag invalidates the line first.
 *
 * Both layouts store 64 longwords. Because the longword number is addr[7:2] in
 * either case, the data and valid arrays are shared and only the tag index and
 * the grouping of valid bits into lines differ.
 */
class Cache {

public:

    // Selects the cache role: Both kinds are controlled by different CACR bits
    enum class Kind { INSTRUCTION, DATA };

    // The 68020 has one longword per line, the 68030 and the 68040 models have four
    enum class Layout { M68020, M68030 };

    static constexpr int numLongwords = 64;

private:

    // The CPU this cache belongs to
    class Moira &moira;

    Kind kind;
    Layout layout = Layout::M68020;

    u32 tags[numLongwords] = { };       // One per line (the 68030 layout uses the first 16)
    u32 data[numLongwords] = { };       // Indexed by addr[7:2]
    bool valid[numLongwords] = { };     // One flag per longword, indexed by addr[7:2]

    // Instruction cache only: The longword that has been fetched last
    CacheLatch latch = { 0xffffffff, 0xffffffff };

    // The CACR bits that control this cache
    u32 enableBit, freezeBit, burstBit, clearEntryBit, clearBit;


    //
    // Constructing
    //

public:

    Cache(Moira &ref, Kind k);

    // Selects the layout of the emulated CPU model and invalidates all entries
    void update();


    //
    // Querying the cache state
    //

public:

    Layout getLayout() const { return layout; }
    int numLongs() const { return layout == Layout::M68030 ? 4 : 1; }
    int numLines() const { return numLongwords / numLongs(); }

    // Indicates whether the emulated CPU model has this cache
    bool present() const;

    // Indicates whether the cache is present and enabled in the CACR
    bool enabled() const;

    // Indicates whether the CACR freezes the cache (no new entries are allocated)
    bool frozen() const;

    // Indicates whether burst fills are possible
    bool bursting() const;


    //
    // Invalidating entries
    //

public:

    // Invalidates all lines (CACR.CI, CACR.CD)
    void flush();

    // Invalidates the longword that an address maps to (CACR.CEI, CACR.CED)
    void invalidate(u32 addr);

    // Forgets the longword that has been fetched last (instruction cache)
    void flushLatch() { latch = { 0xffffffff, 0xffffffff }; }

    // Performs the actions that are triggered by writing to the CACR
    void cacrDidChange(u32 value);


    //
    // Accessing the cache
    //

public:

    /* Instruction cache: Reads an instruction word via the latch and the
     * cache. On a miss, the line is filled from the bus. The function reports
     * whether a bus access was made and returns the physical address.
     */
    u16 fetch(u32 addr, bool &busAccess, u32 &phys);

    /* Data cache: Reads an aligned longword. Returns false if the access has
     * to bypass the cache (the page is cache-inhibited). May throw a bus error.
     */
    bool readLong(u32 addr, u8 fc, u32 &result);

    /* Data cache: Reflects a write access (the cache is write-through, so the
     * caller has already written to the bus). The mask selects the bytes of
     * the aligned longword that have been written. The ci flag tells whether
     * the page is cache-inhibited.
     */
    void write(u32 addr, bool supervisor, u32 mask, u32 value, bool ci);

private:

    // Maps an address to the position of its longword in the data array
    static int slot(u32 addr) { return int(addr >> 2) & 63; }

    // Maps an address to the line it belongs to
    int index(u32 addr) const { return slot(addr) / numLongs(); }

    // Returns the position of the longword inside its line
    int longword(u32 addr) const { return slot(addr) & (numLongs() - 1); }

    // Returns the slot of the first longword in the line an address maps to
    int first(u32 addr) const { return slot(addr) & ~(numLongs() - 1); }

    // Computes the tag of an address
    static u32 tag(u32 addr, bool supervisor) { return (addr & 0xFFFFFF00) | u32(supervisor); }

    // Checks if the longword of an address is valid and carries the matching tag
    bool hit(u32 addr, bool supervisor) const
    {
        return valid[slot(addr)] && tags[index(addr)] == tag(addr, supervisor);
    }

    // Reads a longword (the caller has checked for a hit)
    u32 read(u32 addr) const { return data[slot(addr)]; }

    // Replaces the bits selected by a mask in a cached longword (the caller has checked for a hit)
    void update(u32 addr, u32 mask, u32 value)
    {
        auto &d = data[slot(addr)];
        d = (d & ~mask) | (value & mask);
    }

    // Stores a longword
    void insert(u32 addr, bool supervisor, u32 value)
    {
        auto t = tag(addr, supervisor);

        if (tags[index(addr)] != t) for (int i = 0; i < numLongs(); i++) valid[first(addr) + i] = false;
        tags[index(addr)] = t;
        data[slot(addr)] = value;
        valid[slot(addr)] = true;
    }

    // Returns the number of valid longwords in the line an address maps to
    int validCount(u32 addr) const
    {
        int count = 0;
        for (int i = 0; i < numLongs(); i++) count += valid[first(addr) + i];
        return count;
    }

    // Serves an instruction fetch that is not covered by the latch; returns true on a miss
    bool fill(u32 base, u32 &phys);

    // Reads an aligned longword from the bus, split according to the width of the addressed port
    u32 busReadLong(u32 addr);

    // Fetches the missing longwords of a line right behind the longword that has just been loaded
    void burstFill(u32 logical, u32 physical, bool supervisor);
};

}
