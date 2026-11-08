// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "MoiraTypes.h"

namespace MOIRA_NAMESPACE {

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
 */
class Cache {

public:

    enum class Layout { M68020, M68030 };

    static constexpr int maxLines = 64;
    static constexpr int maxLongs = 4;

    struct Line {

        u32 tag;
        u32 data[maxLongs];
        u8  valid;              // One bit per longword
    };

    Line lines[maxLines] = { };

private:

    Layout layout = Layout::M68020;
    int lineCnt = 64;
    int longsPerLine = 1;

public:

    void setLayout(Layout l)
    {
        layout = l;
        lineCnt = (l == Layout::M68020) ? 64 : 16;
        longsPerLine = (l == Layout::M68020) ? 1 : 4;
        flush();
    }

    Layout getLayout() const { return layout; }
    int numLines() const { return lineCnt; }
    int numLongs() const { return longsPerLine; }

    // Maps an address to its line and longword
    int index(u32 addr) const { return int(addr >> (longsPerLine == 4 ? 4 : 2)) & (lineCnt - 1); }
    int longword(u32 addr) const { return longsPerLine == 4 ? int(addr >> 2) & 3 : 0; }
    static u32 tag(u32 addr, bool supervisor) { return (addr & 0xFFFFFF00) | u32(supervisor); }

    // Invalidates all lines
    void flush()
    {
        for (int i = 0; i < maxLines; i++) lines[i].valid = 0;
    }

    // Invalidates the longword that an address maps to
    void invalidate(u32 addr)
    {
        lines[index(addr)].valid &= ~(1 << longword(addr));
    }

    // Returns the line that an address maps to
    Line &line(u32 addr) { return lines[index(addr)]; }

    // Checks if the longword of an address is valid and carries the matching tag
    bool hit(u32 addr, bool supervisor) const
    {
        auto &l = lines[index(addr)];
        return (l.valid & (1 << longword(addr))) && l.tag == tag(addr, supervisor);
    }

    // Reads a longword (the caller has checked for a hit)
    u32 read(u32 addr) const { return lines[index(addr)].data[longword(addr)]; }

    // Stores a longword
    void insert(u32 addr, bool supervisor, u32 data)
    {
        auto &l = lines[index(addr)];
        auto t = tag(addr, supervisor);

        if (l.tag != t) l.valid = 0;
        l.tag = t;
        l.data[longword(addr)] = data;
        l.valid |= 1 << longword(addr);
    }

    // Returns the number of valid longwords in the line an address maps to
    int validCount(u32 addr) const
    {
        auto v = lines[index(addr)].valid;
        return (v & 1) + (v >> 1 & 1) + (v >> 2 & 1) + (v >> 3 & 1);
    }
};

}
