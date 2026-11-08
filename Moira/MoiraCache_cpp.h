// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#if MOIRA_EMULATE_ICACHE

void
Moira::flushInstructionCache()
{
    flushInstructionLatch();
    iCache.flush();
}

void
Moira::flushInstructionLatch()
{
    iLatch.addr = 0xffffffff;
    iLatch.data = 0xffffffff;
}

void
Moira::invalidateCacheEntry(u32 addr)
{
    iCache.invalidate(addr);
}

void
Moira::flushDataCache()
{
    dCache.flush();
}

void
Moira::invalidateDataCacheEntry(u32 addr)
{
    dCache.invalidate(addr);
}

bool
Moira::hasDataCache() const
{
    return cpuModel == Model::M68030 || cpuModel == Model::M68EC030;
}

void
Moira::updateCaches()
{
    // The 68020 has a cache with one longword per line, all other models with a cache have four
    iCache.setLayout(cpuModel >= Model::M68EC030 ? Cache::Layout::M68030 : Cache::Layout::M68020);
    dCache.setLayout(Cache::Layout::M68030);

    flushInstructionLatch();
    dCacheOn = hasDataCache() && (reg.cacr & CACR::ED);
}

u32
Moira::busReadLong(u32 addr)
{
    // Filling a cache entry is a longword fetch, so it is split into as many
    // bus cycles as the addressed port provides (see dsack). The cycle penalty
    // for a narrow port is accounted for by the caller.

    switch (portSize(dsack(addr))) {

        case 4:  return read32(addr);
        case 2:  return u32(read16(addr)) << 16 | read16(addr + 2);
        default: return u32(read8(addr))  << 24 | u32(read8(addr + 1)) << 16 |
                        u32(read8(addr + 2)) << 8 | u32(read8(addr + 3));
    }
}

void
Moira::burstFill(Cache &cache, u32 logical, u32 physical, bool supervisor)
{
    /* A burst fill loads the remaining longwords of the line right behind the
     * one that has just been loaded. The MMU only translates the first
     * longword. The others follow in the same physical line. The longword
     * index wraps around within the line.
     */
    auto lbase = logical & ~15;
    auto pbase = physical & ~15;
    auto lw = cache.longword(logical);

    for (int i = 0; i < 3; i++) {

        lw = (lw + 1) & 3;
        cache.insert(lbase + 4 * lw, supervisor, read32((pbase + 4 * lw) & addrMask()));
    }
}

bool
Moira::fillInstructionCache(u32 base, u32 &phys)
{
    bool supervisor = reg.sr.s;
    phys = base;

    // The cache is only consulted while it is enabled. Bit 0 (E) gates the
    // lookup, not just the fill: clearing it has to stop valid lines from
    // being served, or the CPU would keep executing stale opcodes after a
    // program switches the cache off. Bit 1 (F) is deliberately not part of
    // this condition. Freezing only prevents new entries from being
    // allocated; entries that are already valid keep hitting.

    if ((reg.cacr & CACR::E) && iCache.hit(base, supervisor)) {

        //
        // Cache hit
        //

        iLatch.data = iCache.read(base);
        iLatch.addr = base;
        return false;
    }

    //
    // Cache miss
    //

    // The logical address is translated now. The MMU provides the physical
    // address and tells if the page is cache-inhibited.
    bool ci;
    phys = translate<Core::C68020, false, Word>(base, readFC(), &ci) & addrMask() & ~3;

    u32 data = busReadLong(phys);

    if ((reg.cacr & CACR::E) && !(reg.cacr & CACR::F) && !ci) {

        iCache.insert(base, supervisor, data);

        // Burst fills are a 68030 feature
        if (hasDataCache() && (reg.cacr & CACR::IBE) && iCache.validCount(base) == 1 && portSize(dsack(phys)) == 4) {
            burstFill(iCache, base, phys, supervisor);
        }
    }

    // Updated last, so that a read that throws leaves the latch
    // untouched instead of marking stale data as valid.
    iLatch.data = data;
    iLatch.addr = base;

    return true;
}

u16
Moira::readInstructionCache(u32 addr, bool &busAccess, u32 &phys)
{
    auto base = addr & addrMask() & ~3;
    phys = addr;

    if (iLatch.addr == base) {

        // Direct hit
        busAccess = false;

    } else {

        // Query the cache
        busAccess = fillInstructionCache(base, phys);
    }

    return (addr & 2) ? u16(iLatch.data & 0xFFFF) : u16(iLatch.data >> 16);
}

#else

// Dummy implementations used when the instruction cache is disabled at
// compile time. Every fetch behaves like a permanent cache miss: it reads
// straight from the bus and reports that a bus access was made.

void Moira::flushInstructionCache() { }
void Moira::flushInstructionLatch() { }
void Moira::invalidateCacheEntry(u32) { }
void Moira::flushDataCache() { }
void Moira::invalidateDataCacheEntry(u32) { }
bool Moira::hasDataCache() const { return false; }
void Moira::updateCaches() { dCacheOn = false; }
u32 Moira::busReadLong(u32 addr) { return read32(addr); }
void Moira::burstFill(Cache &, u32, u32, bool) { }
bool Moira::fillInstructionCache(u32 addr, u32 &phys) { phys = addr; return true; }

u16
Moira::readInstructionCache(u32 addr, bool &busAccess, u32 &phys)
{
    busAccess = true;
    phys = translate<Core::C68020, false, Word>(addr, readFC());
    return read16(phys);
}

#endif
