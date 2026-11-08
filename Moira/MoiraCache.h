// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* On-chip caches
 *
 * The 68020 has an instruction cache of 64 longwords. The 68030 has an
 * instruction cache and a data cache of 256 bytes each, organized as 16 lines
 * of four longwords. The 68040 models use the 68030 layout for the instruction
 * cache. A data cache is only emulated for the 68030. See Cache.h for details
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

// Invalidates all lines of the instruction cache and clears the latch (CACR.CI)
void flushInstructionCache();

// Clears the latch only, forcing the next fetch to consult the cache again
void flushInstructionLatch();

// Invalidates the longword of the instruction cache that an address maps to (CACR.CEI)
void invalidateCacheEntry(u32 addr);

// Invalidates all lines of the data cache (CACR.CD)
void flushDataCache();

// Invalidates the longword of the data cache that an address maps to (CACR.CED)
void invalidateDataCacheEntry(u32 addr);

// Selects the cache layout of the emulated CPU and updates the cache state
void updateCaches();

// Indicates whether the emulated CPU model has a data cache
bool hasDataCache() const;

// Serves a longword from the instruction cache or refills the line from the bus; returns true on a miss
bool fillInstructionCache(u32 addr, u32 &phys);

// Reads one instruction word via the latch/cache, reporting whether a bus access was made
u16 readInstructionCache(u32 addr, bool &busAccess, u32 &phys);

// Reads an aligned longword from the bus, split according to the width of the addressed port
u32 busReadLong(u32 addr);

// Fetches the missing longwords of a line right behind the longword that has just been loaded
void burstFill(Cache &cache, u32 logical, u32 physical, bool supervisor);

// Reads data via the data cache
template <Core C, Size S, Flags F> u32 readDataCache(u32 addr);
template <Core C> bool readDataLong(u32 addr, u8 fc, u32 &result);

// Writes data via the data cache
template <Core C, Size S, Flags F> void writeDataCache(u32 addr, u32 val);
