// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "../MoiraTypes.h"
#include <optional>

namespace MOIRA_NAMESPACE {

struct MmuContext;

enum class MMUModel
{
    NONE,                   // No MMU installed (MMU instructions cause a line F exception)
    INTERNAL                // MMU built into the 68030 and 68040
};

enum class MmuReg {
    
    TT0,                    // Transparent translation register 0
    TT1,                    // Transparent translation register 1
    MMUSR,                  // MMU Status register
    CRP,                    // CPU Root pointer
    SRP,                    // Supervisor Root pointer
    TC                      // Translation control register
};

struct AtcEntry {

    u32 logical;            // Logical page address
    u32 physical;           // Physical page address
    u8  fc;                 // Function code
    bool valid;             //
    bool busError;          // Translation error bit
    bool wp;                // Write protection bit
    bool modified;          // Modification bit
    bool history;           // Set if the entry was used recently (guides the replacement policy)
};

// Cache size
static constexpr int ATC_ENTRIES                = 22;

// Status register bits
static constexpr u32 MMUSR_BUS_ERROR            = 1 << 15;
static constexpr u32 MMUSR_LIMIT_VIOLATION      = 1 << 14;
static constexpr u32 MMUSR_SUPERVISOR_ONLY      = 1 << 13;
static constexpr u32 MMUSR_WRITE_PROTECTED      = 1 << 11;
static constexpr u32 MMUSR_INVALID              = 1 << 10;
static constexpr u32 MMUSR_MODIFIED             = 1 << 9;
static constexpr u32 MMUSR_TRANSPARENT_ACCESS   = 1 << 6;
static constexpr u32 MMUSR_NUMBER_OF_LEVELS     = 0b111;

class MMU {

    // The CPU this MMU belongs to
    class Moira &moira;

public:

    // Emulated MMU model
    MMUModel model = MMUModel::NONE;

    // Root pointers (CPU and Supervisor)
    u64 crp = 0;
    u64 srp = 0;
    
    // Translation control register
    u32 tc = 0;
    
    // Transparent translation register
    u32 tt0 = 0;
    u32 tt1 = 0;
    
    // MMU Status register
    u32 mmusr = 0;

    // Address translation cache
    AtcEntry atc[ATC_ENTRIES] = { };


    //
    // Constructing
    //

public:

    MMU(Moira &ref) : moira(ref) { reset(); }

    // Initializes all registers and invalidates the ATC
    void reset();


    //
    // Configuring
    //

public:

    MMUModel getModel() const { return model; }
    bool isPresent() const { return model != MMUModel::NONE; }
    void setModel(MMUModel newModel) { model = newModel; }

    // Indicates whether address translation is active
    bool isEnabled() const { return (tc & 0x80000000) && isPresent(); }

    // Setters
    void setTC(u32 value);
    void setTT0(u32 value);
    void setTT1(u32 value);


    //
    // Translating
    //

public:

    // Returns the page size mask as defined in the translation control register
    u32 pageMask() const { return (1u << (tc >> 20 & 0xF)) - 1; }

    // Returns the number of tables indexed by the logical address
    int numTables() const;

    // Checks whether an access matches one of the transparent translation registers
    bool testTT(u32 addr, u8 fc, bool rw) const;

    // Translates a logical address to a physical address
    std::optional<u32> translate(u32 addr, u8 fc, bool write);

    // Creates an ATC entry for a logical address (PLOAD)
    void load(u32 addr, u8 fc, bool write);

    // Returns the MMUSR value and the address of the last descriptor (PTEST)
    u32 test(u32 addr, u8 fc, bool write, u32 level, u32 &descrAddr);


    //
    // Managing the address translation cache
    //

public:

    // Searches the ATC. Returns the line number of a matching entry.
    std::optional<int> atcFind(u32 addr, u8 fc, bool write);

    // Creates an ATC entry and returns its line number
    int atcInsert(u32 addr, u8 fc, u32 physical, u32 status);

    // Invalidates one or more ATC entries
    void atcFlush();
    void atcFlush(u8 fcBase, u8 fcMask);
    void atcFlushPage(u32 addr);
    void atcFlushPage(u32 addr, u8 fcBase, u8 fcMask);

private:

    // Returns the first unused line, if any
    std::optional<int> atcFindFree();

    // Returns the first line without history bit, if any
    std::optional<int> atcFindUnused();

    // Marks an entry as recently used
    void atcTouch(int line);

    // Checks whether an access matches a single transparent translation register
    static bool testTT(u32 ttx, u32 addr, u8 fc, bool rw);

    // Walks the translation tables
    u32 walk(u32 addr, u8 fc, bool write, MmuContext &c);

    // Walks the translation tables without side effects (PTEST mode)
    u32 walk(u32 addr, u8 fc, bool write, MmuContext &c, bool ptest, u32 level);

    // Helpers called inside the walk function
    u32 lookupShort(char table, u32 taddr, u32 offset, bool write, MmuContext &c);
    u32 lookupLong(char table, u32 taddr, u32 offset, bool write, MmuContext &c);
};

}
