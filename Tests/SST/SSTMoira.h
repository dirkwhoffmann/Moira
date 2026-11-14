// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "SSTTypes.h"

namespace sstrunner {

//
// Moira
//

class TestCPU : public Moira {

public:

    Memory mem;
    Fault fault;
    mutable std::vector<Access> log;
    bool logging = false;
    bool wideBus = false;               // Model a 32-bit port (a long access is a single bus cycle)?
    size_t faultLogIndex = 0;           // Position of the failed access in the log
    bool lastFaultWrite = false;        // Was the last injected bus error a write access?
    int exceptions = 0;                 // Exceptions taken since load()

    using Moira::willExecute;
    void willExecute(M68kException, u16) override { exceptions++; }

    TestCPU() { setModel(CPUModel::M68000); }

    void record(bool write, u32 addr, int size, u16 data) const
    {
        u16 mask = size == 2 ? 0xFFFF : (addr & 1) ? 0x00FF : 0xFF00;
        u16 word = size == 2 ? data : (addr & 1) ? data & 0xFF : u16(data << 8);
        if (logging) log.push_back({ getClock() - 2, write, addr & ADDR_MASK & ~1u, mask, word, readFC() });
    }

    // The 68020 and above count data accesses only (as one access per operand)
    bool wide() const { return cpuModel >= CPUModel::M68EC020; }
    bool counts() const { return !wide() || (readFC() & 3) != 2; }

    /* The core splits a long word that is not aligned to 4 into two word accesses. Both halves are
     * one operand access for the reference (except at a 64K boundary). Hence, the 68020 and above
     * count operands, which the core announces via willAccessBus().
     */
    bool opPending = false;             // The next bus cycle is the first one of an operand access
    bool opCrossing = false;            // The operand is split by the reference (long word at the end of a 64K bank)

    void willAccessBus(u32 addr, Size size, bool write) override
    {
        opPending = true;
        opCrossing = size == Long && (addr & 3) != 0 && (addr & 0xFFFF) == 0xFFFE;
    }

    /* Counts a data access and tells if it has to fail */
    bool failAccess(bool write, u32 addr, int size) const
    {
        if (!logging || !counts()) return false;
        auto self = const_cast<TestCPU *>(this);

        if (wide()) {
            if (opPending) { self->opPending = false; return self->fault.hit(); }
            if (opCrossing) { self->opCrossing = false; return self->fault.hit(); }
            return false;               // Second half of a split operand
        }
        return self->fault.hit();
    }

    u8 read8(u32 addr) const override
    {
        auto self = const_cast<TestCPU *>(this);
        u8 v = mem.peek(addr);
        if (failAccess(false, addr, 1)) { self->lastFaultWrite = false; self->faultLogIndex = self->log.size(); record(false, addr, 1, 0); throw BusError(wide() ? addr : addr & ADDR_MASK); }
        record(false, addr, 1, v);
        return v;
    }
    u16 read16(u32 addr) const override
    {
        auto self = const_cast<TestCPU *>(this);
        u16 v = mem.peek16(addr);
        if (failAccess(false, addr, 2)) { self->lastFaultWrite = false; self->faultLogIndex = self->log.size(); record(false, addr, 2, 0); throw BusError(wide() ? addr : addr & ADDR_MASK); }
        record(false, addr, 2, v);
        return v;
    }
    u32 read32(u32 addr) const override
    {
        if (!wide()) return u32(read16(addr) << 16 | read16(addr + 2));
        auto self = const_cast<TestCPU *>(this);
        u32 v = u32(mem.peek16(addr) << 16 | mem.peek16(addr + 2));
        if (failAccess(false, addr, 4)) { self->lastFaultWrite = false; self->faultLogIndex = self->log.size(); record(false, addr, 2, 0); throw BusError(wide() ? addr : addr & ADDR_MASK); }
        record(false, addr, 2, u16(v >> 16));
        return v;
    }

    void write8(u32 addr, u8 val) const override
    {
        auto self = const_cast<TestCPU *>(this);
        if (failAccess(true, addr, 1)) { self->lastFaultWrite = true; self->faultLogIndex = self->log.size(); record(true, addr, 1, val); throw BusError(wide() ? addr : addr & ADDR_MASK); }
        record(true, addr, 1, val);
        self->mem.poke(addr, val);
    }
    void write16(u32 addr, u16 val) const override
    {
        auto self = const_cast<TestCPU *>(this);
        if (failAccess(true, addr, 2)) { self->lastFaultWrite = true; self->faultLogIndex = self->log.size(); record(true, addr, 2, val); throw BusError(wide() ? addr : addr & ADDR_MASK); }
        record(true, addr, 2, val);
        self->mem.poke(addr, u8(val >> 8));
        self->mem.poke(addr + 1, u8(val));
    }
    void write32(u32 addr, u32 val) const override
    {
        if (!wide()) { write16(addr, u16(val >> 16)); write16(addr + 2, u16(val)); return; }
        auto self = const_cast<TestCPU *>(this);
        if (failAccess(true, addr, 4)) { self->lastFaultWrite = true; self->faultLogIndex = self->log.size(); record(true, addr, 2, u16(val >> 16)); throw BusError(wide() ? addr : addr & ADDR_MASK); }
        record(true, addr, 2, u16(val >> 16));
        for (int i = 0; i < 4; i++) self->mem.poke(addr + i, u8(val >> (24 - 8 * i)));
    }

    // Lets bus errors restore the registers (Moira does this only if an MMU is enabled or bus error emulation is on)
    void enableRestart() { flags |= State::RESTARTABLE; }

    u8 dsack(u32) const override { return (wideBus || wide()) ? DSACK_32 : DSACK_16; }

    void load(const sst::State &s)
    {
        logging = false;
        mem.load(s);
        reset();
        setSR(u16(s.sr));
        setUSP(s.usp);
        setISP(s.ssp);
        for (int i = 0; i < 8; i++) setD(i, s.d[i]);
        for (int i = 0; i < 7; i++) setA(i, s.a[i]);
        setPC(s.pc - 4);
        setPC0(s.pc - 4);
        setIRD(u16(s.prefetch[0]));
        setIRC(u16(s.prefetch[1]));
        setClock(0);
        log.clear();
        exceptions = 0;
        fault = Fault();
    }
};

}
