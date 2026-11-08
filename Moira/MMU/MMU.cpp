// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "../MoiraConfig.h"
#include "MMU.h"
#include "../MoiraMacros.h"

namespace MOIRA_NAMESPACE {

void
MMU::reset()
{
    // Reset registers
    crp = srp = 0;
    tc = tt0 = tt1 = mmusr = 0;
    
    // Reset cache
    for (auto &e : atc) e = { };
}

bool
MMU::testTT(u32 ttx, u32 addr, u8 fc, bool rw)
{
    auto addrBase = ________xxxxxxxx (ttx >> 24);
    auto addrMask = ________xxxxxxxx (ttx >> 16);
    auto e        = x_______________ (ttx);
    auto rwb      = ______x_________ (ttx);
    auto rwm      = _______x________ (ttx);
    auto fcBase   = _________xxx____ (ttx);
    auto fcMask   = _____________xxx (ttx);

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

void
MMU::atcTouch(int line)
{
    // Mark the entry as recently used
    atc[line].mru = true;

    // If all history bits are set, clear all but this one
    for (auto &e : atc) if (!e.mru) return;
    for (auto &e : atc) e.mru = false;
    atc[line].mru = true;
}

std::optional<int>
MMU::atcFind(u32 addr, u8 fc, bool write)
{
    u32 page = addr & ~pageMask();

    for (int i = 0; i < ATC_ENTRIES; i++) {

        auto &e = atc[i];
        
        // Check if the entry is valid
        if (!e.valid) continue;
        
        // Check if the logica page and the fc pins match
        if (e.logical != page || e.fc != fc) continue;

        // A write access to a page that hasn't been marked as modified yet
        // needs a table walk to set the M bit. The entry is invalidated.
        // Note: An address is mapped by at most one entry, so the search ends here.
        if (!write || e.modified || e.wp || e.busError) {

            atcTouch(i);
            return i;
        }
        
        e.valid = false;
        break;
    }
    
    return std::nullopt;
}

std::optional<int>
MMU::atcFindFree()
{
    for (int i = 0; i < ATC_ENTRIES; i++) if (!atc[i].valid) return i;
    return std::nullopt;
}

std::optional<int>
MMU::atcFindUnused()
{
    for (int i = 0; i < ATC_ENTRIES; i++) if (!atc[i].mru) return i;
    return std::nullopt;
}

int
MMU::atcInsert(u32 addr, u8 fc, u32 physical, u32 status)
{
    // Find the first free entry
    auto line = atcFindFree();

    // If there is none, use the first entry wit no history bit
    if (!line) line = atcFindUnused();

    // If there is none, fallback to the first entry
    if (!line) line = 0;

    // Mark the picked entry as used
    atcTouch(*line);

    auto &e = atc[*line];
    
    e.logical   = addr & ~pageMask();
    e.physical  = physical & ~pageMask();
    e.fc        = fc;
    e.valid     = true;
    e.busError  = status & (MMUSR_INVALID | MMUSR_SUPERVISOR_ONLY);
    e.wp        = status & MMUSR_WRITE_PROTECTED;
    e.modified  = status & MMUSR_MODIFIED;

    return *line;
}

void
MMU::atcFlush()
{
    for (auto &e : atc) e.valid = false;
}

void
MMU::atcFlush(u8 fcBase, u8 fcMask)
{
    for (auto &e : atc) if ((fcBase & fcMask) == (e.fc & fcMask)) e.valid = false;
}

void
MMU::atcFlushPage(u32 addr)
{
    for (auto &e : atc) if (e.logical == (addr & ~pageMask())) e.valid = false;
}

void
MMU::atcFlushPage(u32 addr, u8 fcBase, u8 fcMask)
{
    for (auto &e : atc) {
        if (e.logical == (addr & ~pageMask()) && (fcBase & fcMask) == (e.fc & fcMask)) e.valid = false;
    }
}

}
