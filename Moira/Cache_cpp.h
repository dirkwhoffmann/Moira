// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

void
Cache::reset(Layout l)
{
    layout = l;
    flush();
}

void
Cache::reset(CPUModel model)
{
    // The 68020 has a cache with one longword per line, all other models with a cache have four
    reset(model >= CPUModel::M68EC030 ? Layout::M68030 : Layout::M68020);
}

bool
Cache::burstCapable() const
{
    return moira.cpuModel == CPUModel::M68030 || moira.cpuModel == CPUModel::M68EC030;
}

void
Cache::flush()
{
    for (auto &v : valid) v = false;
}

void
Cache::invalidate(u32 addr)
{
    valid[slot(addr)] = false;
}

u32
Cache::busReadLong(u32 addr)
{
    // Filling a cache entry is a longword fetch, so it is split into as many
    // bus cycles as the addressed port provides (see dsack). The cycle penalty
    // for a narrow port is accounted for by the caller.

    switch (portSize(moira.dsack(addr))) {

        case 4:  return moira.read32(addr);
        case 2:  return u32(moira.read16(addr)) << 16 | moira.read16(addr + 2);
        default: return u32(moira.read8(addr))  << 24 | u32(moira.read8(addr + 1)) << 16 |
                        u32(moira.read8(addr + 2)) << 8 | u32(moira.read8(addr + 3));
    }
}

void
Cache::burstFill(u32 logical, u32 physical, bool supervisor)
{
    /* A burst fill loads the remaining longwords of the line right behind the
     * one that has just been loaded. The MMU only translates the first
     * longword. The others follow in the same physical line. The longword
     * index wraps around within the line.
     */
    auto lbase = logical & ~15;
    auto pbase = physical & ~15;
    auto lw = longword(logical);

    for (int i = 0; i < 3; i++) {

        lw = (lw + 1) & 3;
        insert(lbase + 4 * lw, supervisor, moira.read32((pbase + 4 * lw) & moira.addrMask()));
    }
}

bool
ICache::enabled() const
{
    return moira.reg.cacr & CACR::E;
}

bool
ICache::frozen() const
{
    return moira.reg.cacr & CACR::F;
}

bool
ICache::burstEnabled() const
{
    return moira.reg.cacr & CACR::IBE;
}

void
ICache::flush()
{
    Cache::flush();
    flushLatch();
}

void
ICache::cacrDidChange(u32 value)
{
    // Setting the CE bit invalidates the entry for the address in CAAR
    if (value & CACR::CE) invalidate(moira.reg.caar);

    // Setting the C bit invalidates all entries
    if (value & CACR::C) flush();
}

bool
ICache::fill(u32 base, u32 &phys)
{
    bool supervisor = moira.reg.sr.s;
    phys = base;

    // The cache is only consulted while it is enabled. Bit 0 (E) gates the
    // lookup, not just the fill: clearing it has to stop valid lines from
    // being served, or the CPU would keep executing stale opcodes after a
    // program switches the cache off. Bit 1 (F) is deliberately not part of
    // this condition. Freezing only prevents new entries from being
    // allocated; entries that are already valid keep hitting.

    if (enabled() && hit(base, supervisor)) {

        //
        // Cache hit
        //

        latch = { base, read(base) };
        return false;
    }

    //
    // Cache miss
    //

    // The logical address is translated now. The MMU provides the physical
    // address and tells if the page is cache-inhibited.
    bool ci;
    phys = moira.translate<Core::C68020, false, Word>(base, moira.readFC(), &ci) & moira.addrMask() & ~3;

    u32 value = busReadLong(phys);

    if (enabled() && !frozen() && !ci) {

        insert(base, supervisor, value);

        if (burstEnabled() && burstCapable() && validCount(base) == 1 && portSize(moira.dsack(phys)) == 4) {
            burstFill(base, phys, supervisor);
        }
    }

    // Updated last, so that a read that throws leaves the latch
    // untouched instead of marking stale data as valid.
    latch = { base, value };

    return true;
}

u16
ICache::fetch(u32 addr, bool &busAccess, u32 &phys)
{
#if MOIRA_EMULATE_ICACHE

    auto base = addr & moira.addrMask() & ~3;
    phys = addr;

    if (latch.addr == base) {

        // Direct hit
        busAccess = false;

    } else {

        // Query the cache
        busAccess = fill(base, phys);
    }

    return (addr & 2) ? u16(latch.data & 0xFFFF) : u16(latch.data >> 16);

#else

    // Every fetch behaves like a permanent cache miss
    busAccess = true;
    phys = moira.translate<Core::C68020, false, Word>(addr, moira.readFC());
    return moira.read16(phys);

#endif
}

bool
DCache::present() const
{
    // A data cache is only emulated for the 68030
    return burstCapable();
}

bool
DCache::enabled() const
{
    return present() && (moira.reg.cacr & CACR::ED);
}

bool
DCache::frozen() const
{
    return moira.reg.cacr & CACR::FD;
}

bool
DCache::burstEnabled() const
{
    return moira.reg.cacr & CACR::DBE;
}

void
DCache::cacrDidChange(u32 value)
{
    if (!present()) return;

    // Setting the CED bit invalidates the entry for the address in CAAR
    if (value & CACR::CED) invalidate(moira.reg.caar);

    // Setting the CD bit invalidates all entries
    if (value & CACR::CD) flush();
}

bool
DCache::readLong(u32 addr, u8 fc, u32 &result)
{
    bool supervisor = fc & 4;

    // Serve the longword from the cache if possible
    if (hit(addr, supervisor)) {
        result = read(addr);
        return true;
    }

    // On a miss, the MMU provides the physical address and the cache-inhibit attribute
    bool ci;
    u32 phys = moira.translate<Core::C68020, false, Long>(addr, fc, &ci) & moira.addrMask() & ~3;
    if (ci) return false;

    // Load the entire longword, regardless of the size of the access
    result = busReadLong(phys);

    if (!frozen()) {

        insert(addr, supervisor, result);

        if (burstEnabled() && burstCapable() && validCount(addr) == 1 && portSize(moira.dsack(phys)) == 4) {
            burstFill(addr, phys, supervisor);
        }
    }
    return true;
}

void
DCache::write(u32 addr, bool supervisor, u32 mask, u32 value, bool ci)
{
    bool writeAllocate = moira.reg.cacr & CACR::WA;
    bool full = mask == 0xFFFFFFFF;

    if (hit(addr, supervisor)) {

        // A full write to an inhibited page drops the entry if write allocation is on
        if (ci && full && writeAllocate) {
            invalidate(addr);
        } else {
            update(addr, mask, value);
        }

    } else if (writeAllocate && !frozen() && !ci && full) {

        // An aligned longword write creates an entry
        insert(addr, supervisor, value);
    }
}
