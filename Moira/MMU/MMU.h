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

// Supported MMU models
enum class MMUModel
{
    NONE,           // No MMU installed (MMU instructions cause a line F exception)
    INTERNAL        // MMU built into the 68030 and 68040
};

// MMU registers addressable by PMOVE
enum class MmuReg { TT0, TT1, MMUSR, CRP, SRP, TC };

// An entry of the address translation cache (ATC)
struct AtcEntry {

    u32 logical;            // Logical page address
    u32 physical;           // Physical page address
    u8  fc;                 // Function code
    bool valid;             //
    bool busError;          // Translation error bit
    bool wp;                // Write protection bit
    bool modified;          // Modification bit
    bool mru;               // History bit used by the replacement algorithm
};

static constexpr int ATC_ENTRIES = 22;

// MMU status register (MMUSR) bits
static constexpr u32 MMUSR_BUS_ERROR          = 1 << 15;
static constexpr u32 MMUSR_LIMIT_VIOLATION    = 1 << 14;
static constexpr u32 MMUSR_SUPERVISOR_ONLY    = 1 << 13;
static constexpr u32 MMUSR_WRITE_PROTECTED    = 1 << 11;
static constexpr u32 MMUSR_INVALID            = 1 << 10;
static constexpr u32 MMUSR_MODIFIED           = 1 << 9;
static constexpr u32 MMUSR_TRANSPARENT_ACCESS = 1 << 6;
static constexpr u32 MMUSR_NUMBER_OF_LEVELS   = 0b111;

class MMU {

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

    MMU() { reset(); }

    // Initializes all registers and invalidates the ATC
    void reset();


    //
    // Configuring
    //

public:

    MMUModel getModel() const { return model; }
    bool isPresent() const { return model != MMUModel::NONE; }
    void setModel(MMUModel newModel) { model = newModel; }


    //
    // Translating
    //

public:

    // Returns the page size mask as defined in the translation control register
    u32 pageMask() const { return (1u << (tc >> 20 & 0xF)) - 1; }

    // Checks whether an access matches a transparent translation register
    static bool testTT(u32 ttx, u32 addr, u8 fc, bool rw);


    //
    // Managing the address translation cache
    //

public:

    // Searches the ATC. Returns the line number of a matching entry.
    std::optional<int> atcFind(u32 addr, u8 fc, bool write);

    // Creates an ATC entry and returns its line number
    int atcInsert(u32 addr, u8 fc, u32 physical, u32 status);

    // Invalidates entries
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
};

}
