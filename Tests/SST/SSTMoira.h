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

    TestCPU() { setModel(CPUModel::M68000); }

    void record(bool write, u32 addr, int size, u16 data) const
    {
        u16 mask = size == 2 ? 0xFFFF : (addr & 1) ? 0x00FF : 0xFF00;
        u16 word = size == 2 ? data : (addr & 1) ? data & 0xFF : u16(data << 8);
        if (logging) log.push_back({ getClock() - 2, write, addr & ADDR_MASK & ~1u, mask, word, readFC() });
    }

    u8 read8(u32 addr) const override
    {
        auto self = const_cast<TestCPU *>(this);
        u8 v = mem.peek(addr);
        if (logging && self->fault.hit()) { record(false, addr, 1, 0); throw BusError(addr & ADDR_MASK); }
        record(false, addr, 1, v);
        return v;
    }
    u16 read16(u32 addr) const override
    {
        auto self = const_cast<TestCPU *>(this);
        u16 v = mem.peek16(addr);
        if (logging && self->fault.hit()) { record(false, addr, 2, 0); throw BusError(addr & ADDR_MASK); }
        record(false, addr, 2, v);
        return v;
    }
    u32 read32(u32 addr) const override { return u32(read16(addr) << 16 | read16(addr + 2)); }

    void write8(u32 addr, u8 val) const override
    {
        auto self = const_cast<TestCPU *>(this);
        if (logging && self->fault.hit()) { record(true, addr, 1, val); throw BusError(addr & ADDR_MASK); }
        record(true, addr, 1, val);
        self->mem.poke(addr, val);
    }
    void write16(u32 addr, u16 val) const override
    {
        auto self = const_cast<TestCPU *>(this);
        if (logging && self->fault.hit()) { record(true, addr, 2, val); throw BusError(addr & ADDR_MASK); }
        record(true, addr, 2, val);
        self->mem.poke(addr, u8(val >> 8));
        self->mem.poke(addr + 1, u8(val));
    }
    void write32(u32 addr, u32 val) const override { write16(addr, u16(val >> 16)); write16(addr + 2, u16(val)); }

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
        fault = Fault();
    }
};

}
