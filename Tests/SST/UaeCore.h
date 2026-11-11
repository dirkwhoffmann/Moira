// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "RefCore.h"
#include "UAECpu.h"

namespace sstrunner {

// The 68000 core of UAE (WinUAE, cycle exact). The core is a singleton.
struct UaeCore : RefCore {

    static inline UaeCore *self = nullptr;

    Memory &mem;
    Fault &fault;
    std::vector<mame::BusCycle> cycles;
    bool didHalt = false;
    uint64_t cycleBase = 0;

    CPUModel model = CPUModel::M68000;
    std::string label = "UAE (WinUAE, cycle exact 68000)";

    UaeCore(Memory &m, Fault &f) : mem(m), fault(f)
    {
        self = this;
        configure(CPUModel::M68000);
    }

    bool supports(CPUModel) const override { return true; }

    void setModel(CPUModel m) override { configure(m); }

    void configure(CPUModel m)
    {
        using uaecpu::Cpu;
        model = m;

        Cpu cpu =
        m == CPUModel::M68000   ? Cpu::M68000 :
        m == CPUModel::M68010   ? Cpu::M68010 :
        m == CPUModel::M68EC020 ? Cpu::M68EC020 :
        m == CPUModel::M68020   ? Cpu::M68020 :
        m == CPUModel::M68EC030 ? Cpu::M68EC030 :
        m == CPUModel::M68030   ? Cpu::M68030 :
        m == CPUModel::M68EC040 ? Cpu::M68EC040 :
        m == CPUModel::M68LC040 ? Cpu::M68LC040 : Cpu::M68040;

        // Only the 68040 has a built-in FPU
        auto fpu = m == CPUModel::M68040 ? uaecpu::Fpu::Internal : uaecpu::Fpu::None;

        bool exact = m == CPUModel::M68000 || m == CPUModel::M68010;
        label = exact ? std::string("UAE (WinUAE, cycle exact ") + common::modelName(m) + ")" : std::string("UAE (WinUAE, ") + common::modelName(m) + ")";

        uaecpu::init(cpu, fpu, mem.ram.data(), [](int size, uint32_t addr, uint32_t, uint32_t, uint64_t) {

            // Writes go directly into the memory. Remember them, so that the memory can be reset.
            for (int i = 0; i < size; i++) self->mem.touched.push_back((addr + i) & ADDR_MASK);
        });
        uaecpu::setMemoryMask(ADDR_MASK);
        uaecpu::setTraceInStep(false);
        uaecpu::setAccessHook([](const uaecpu::BusAccess &a) { return self->access(a); });
    }

    // Is called for each bus access of the CPU
    bool access(const uaecpu::BusAccess &a)
    {
        // The 68020 and above are not cycle exact. Only data accesses are counted (one per operand access).
        if (a.size == 4 || model >= CPUModel::M68EC020) {
            if (a.fetch) return false;
            u32 w = a.addr & ~1u;
            cycles.push_back({ 0, a.write, w, a.write ? u16(a.size == 4 ? a.value >> 16 : a.value) : mem.peek16(w), 0xFFFF, int(a.fc) });
            return fault.hit();
        }
        u16 mask = a.size == 2 ? 0xFFFF : (a.addr & 1) ? 0x00FF : 0xFF00;
        u32 word = a.addr & ~1u;
        u16 data = a.write ? (a.size == 2 ? u16(a.value) : (a.addr & 1) ? u16(a.value & 0xFF) : u16(a.value << 8)) : mem.peek16(word);
        cycles.push_back({ int(a.cycle - cycleBase), a.write, word, data, mask, int(a.fc) });
        return fault.hit();
    }

    void load(const sst::State &s) override
    {
        uaecpu::State u;
        for (int i = 0; i < 8; i++) u.d[i] = s.d[i];
        for (int i = 0; i < 7; i++) u.a[i] = s.a[i];
        u.usp = s.usp;
        u.isp = s.ssp;
        u.sr = u16(s.sr);
        u.pc = (s.pc - 4) & ADDR_MASK;

        // Loading the state fetches the prefetch queue. This is not part of the instruction.
        uaecpu::setAccessHook(nullptr);
        uaecpu::setState(u);
        if (model == CPUModel::M68000) uaecpu::setPrefetch(u16(s.prefetch[0]), u16(s.prefetch[1]));
        uaecpu::setAccessHook([](const uaecpu::BusAccess &a) { return self->access(a); });
        cycles.clear();
        didHalt = false;
    }

    int execute() override
    {
        didHalt = !uaecpu::step();
        return int(uaecpu::cycles());
    }

    mame::State state() const override
    {
        auto u = uaecpu::getState();
        mame::State m;
        for (int i = 0; i < 8; i++) m.d[i] = u.d[i];
        for (int i = 0; i < 7; i++) m.a[i] = u.a[i];
        m.usp = u.usp;
        m.ssp = u.isp;
        m.sr = u.sr;
        m.pc = u.pc & ADDR_MASK;
        return m;
    }

    const std::vector<mame::BusCycle> &log() const override { return cycles; }
    bool halted() const override { return didHalt; }

    // UAE implements the 68010 address error frame only partially. The stacked PC and the two input
    // buffers differ from the frames recorded on real hardware (see Tests/Unit/StackFrameTests.cpp).
    bool reliableFrameField(const char *field) const override
    {
        // UAE leaves the internal words of the 68020 frames empty and sets the SSW only partially (no DF, RC, RB, ...)
        if (model >= CPUModel::M68EC020) return strcmp(field, "frame.int") && strcmp(field, "frame.ssw");
        if (model != CPUModel::M68010) return true;
        return strcmp(field, "frame.pc") && strcmp(field, "frame.dib") && strcmp(field, "frame.iib");
    }

    // Only the 68000 is cycle exact and logs its bus cycles like MAME
    bool hasBusLog() const override { return model == CPUModel::M68000; }

    const char *name() const override { return label.c_str(); }
    const char *shortName() const override { return "UAE"; }

    // UAE sets the I/N bit (bit 3) by rules that are not modeled, so it is not compared
    u16 codeMask() const override { return 0xFFF7; }

    // The registers of a halted UAE are not compared (e.g., it keeps the trace bit and ends up with a different SSP)
    bool comparesHaltedRegs() const override { return false; }

    // UAE doesn't log the aborted access of an address error
    bool logsAbortedAccess() const override { return false; }

    // UAE and MAME disagree on the frames of bus errors that hit the stacking, the vector fetch,
    // or the first fetches of the handler of a group 1 or 2 exception
    bool disagreesOnStacking() const override { return true; }
};

}
