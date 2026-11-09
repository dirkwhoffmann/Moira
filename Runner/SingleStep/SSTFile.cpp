// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "SSTFile.h"
#include <cstdio>

namespace sst {

namespace {

constexpr uint32_t MAGIC_FILE         = 0x1A3F5D71;
constexpr uint32_t MAGIC_TEST         = 0xABC12367;
constexpr uint32_t MAGIC_NAME         = 0x89ABCDEF;
constexpr uint32_t MAGIC_STATE        = 0x01234567;
constexpr uint32_t MAGIC_TRANSACTIONS = 0x456789AB;

// Cursor over a block of memory. All reads are bounds checked.
struct Cursor {

    const uint8_t *p;
    const uint8_t *end;
    bool ok = true;

    uint32_t u32() {
        if (end - p < 4) { ok = false; return 0; }
        uint32_t v = p[0] | p[1] << 8 | p[2] << 16 | uint32_t(p[3]) << 24;
        p += 4;
        return v;
    }
    uint16_t u16() {
        if (end - p < 2) { ok = false; return 0; }
        uint16_t v = uint16_t(p[0] | p[1] << 8);
        p += 2;
        return v;
    }

    // Opens a block, i.e., checks its magic number and returns a cursor for its payload
    Cursor block(uint32_t magic) {

        const uint8_t *start = p;
        uint32_t size = u32();
        if (u32() != magic || size < 8 || size_t(end - start) < size) ok = false;
        if (!ok) return { p, p, false };

        p = start + size;
        return { start + 8, start + size };
    }
};

bool readState(Cursor &parent, State &s)
{
    Cursor c = parent.block(MAGIC_STATE);

    for (auto &v : s.d) v = c.u32();
    for (auto &v : s.a) v = c.u32();
    s.usp = c.u32();
    s.ssp = c.u32();
    s.sr = c.u32();
    s.pc = c.u32();
    s.prefetch[0] = c.u32();
    s.prefetch[1] = c.u32();

    // The RAM is stored as 16 bit words (big endian byte order within the word)
    s.ram.clear();
    for (uint32_t n = c.u32(); n && c.ok; n--) {

        uint32_t addr = c.u32();
        uint16_t word = c.u16();
        s.ram.emplace_back(addr, uint8_t(word >> 8));
        s.ram.emplace_back(addr | 1, uint8_t(word));
    }
    return c.ok;
}

}

bool
File::open(const std::string &path)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    data.resize(size_t(size));
    bool ok = size >= 8 && fread(data.data(), 1, data.size(), f) == data.size();
    fclose(f);
    if (!ok) return false;

    Cursor c { data.data(), data.data() + data.size() };
    if (c.u32() != MAGIC_FILE) return false;
    remaining = c.u32();
    pos = 8;
    return true;
}

bool
File::next(Test &test)
{
    if (remaining == 0) return false;

    Cursor file { data.data() + pos, data.data() + data.size() };
    Cursor c = file.block(MAGIC_TEST);
    if (!file.ok) return false;

    // Name
    Cursor name = c.block(MAGIC_NAME);
    uint32_t len = name.u32();
    if (!name.ok || size_t(name.end - name.p) < len) return false;
    test.name.assign(reinterpret_cast<const char *>(name.p), len);

    // States
    if (!readState(c, test.initial) || !readState(c, test.final)) return false;

    // Transactions (only the total cycle count is used)
    Cursor transactions = c.block(MAGIC_TRANSACTIONS);
    test.cycles = transactions.u32();
    if (!transactions.ok || !c.ok) return false;

    pos = size_t(file.p - data.data());
    remaining--;
    return true;
}

}
