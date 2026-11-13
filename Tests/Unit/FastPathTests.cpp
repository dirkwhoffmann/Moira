// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "doctest.h"
#include "Moira.h"
#include <vector>

using namespace moira;

namespace {

/* Models without an MMU run on the fast path of execute() unless a flag
 * requests more. The RESTARTABLE flag makes a faulted instruction restartable.
 * It is set while an MMU is enabled, or for the 68020 family if the library
 * has been compiled with MOIRA_EMULATE_BUS_ERROR. This file is compiled into
 * two executables: One is linked against a library with bus error emulation,
 * the other against the default library.
 */
TEST_SUITE_BEGIN("FastPath");

class FastCPU : public Moira {

public:

    mutable std::vector<u8> ram = std::vector<u8>(1 << 20);
    u32 faultAddr = 0xFFFFFFFF;

    static constexpr u32 codeStart = 0x1000;
    static constexpr u32 handler = 0x4000;

    u8 read8(u32 a) const override { check(a); return ram[a & 0xFFFFF]; }
    u16 read16(u32 a) const override { check(a); return ram[a & 0xFFFFF] << 8 | ram[(a + 1) & 0xFFFFF]; }
    u32 read32(u32 a) const override { return read16(a) << 16 | read16(a + 2); }
    void write8(u32 a, u8 v) const override { ram[a & 0xFFFFF] = v; }
    void write16(u32 a, u16 v) const override { ram[a & 0xFFFFF] = v >> 8; ram[(a + 1) & 0xFFFFF] = v & 0xFF; }
    void write32(u32 a, u32 v) const override { write16(a, v >> 16); write16(a + 2, v & 0xFFFF); }

    void check(u32 a) const { if ((a & ~1u) == faultAddr) throw BusError(a); }

    void poke16(u32 a, u16 v) { ram[a] = v >> 8; ram[a + 1] = v & 0xFF; }
    void poke32(u32 a, u32 v) { poke16(a, v >> 16); poke16(a + 2, v & 0xFFFF); }

    FastCPU(CPUModel model)
    {
        setModel(model);
        poke32(0, 0x1F00);
        poke32(4, codeStart);
        poke32(8, handler);                         // Bus error vector
        poke16(handler, 0x4E71);
        reset();
    }

    bool restartable() const { return flags & State::RESTARTABLE; }
    bool fastPath() const { return flags == 0; }

    // Programs the translation control register of the 68030 MMU
    void setTC(u32 tc) { mmu.mmu030.tc = tc; updateMmuFlag(); }
};

const CPUModel noMmuModels[] = {
    CPUModel::M68000, CPUModel::M68010, CPUModel::M68EC020, CPUModel::M68020
};

TEST_CASE("The restart flag follows the configuration")
{
    for (auto model : noMmuModels) {

        INFO("Model ", int(model));
        FastCPU cpu(model);

        bool expected = MOIRA_EMULATE_BUS_ERROR && model >= CPUModel::M68EC020;
        CHECK(cpu.restartable() == expected);
    }
}

TEST_CASE("Models without bus error emulation run on the fast path")
{
    for (auto model : noMmuModels) {

        INFO("Model ", int(model));
        FastCPU cpu(model);
        cpu.poke16(FastCPU::codeStart, 0x4E71);     // NOP
        cpu.poke16(FastCPU::codeStart + 2, 0x4E71);
        cpu.debugger.jump(FastCPU::codeStart);
        cpu.setSR(0x2700);

        // The first instruction clears the interrupt check flag
        cpu.execute();

        bool expected = !(MOIRA_EMULATE_BUS_ERROR && model >= CPUModel::M68EC020);
        CHECK(cpu.fastPath() == expected);
    }
}

TEST_CASE("An enabled MMU makes instructions restartable")
{
    FastCPU cpu(CPUModel::M68030);

    cpu.setTC(1u << 31 | 0xC << 20 | 0xA << 12 | 0xA << 8);
    CHECK(cpu.restartable());

    cpu.setTC(0);
    CHECK(cpu.restartable() == bool(MOIRA_EMULATE_BUS_ERROR));
}

TEST_CASE("Bus errors are processed with and without bus error emulation")
{
    for (auto model : noMmuModels) {

        INFO("Model ", int(model));
        FastCPU cpu(model);
        cpu.faultAddr = 0x2000;
        cpu.setA(0, 0x2000);

        cpu.poke16(FastCPU::codeStart, 0x3018);     // MOVE.W (A0)+,D0
        cpu.poke16(FastCPU::codeStart + 2, 0x4E71);
        cpu.debugger.jump(FastCPU::codeStart);
        cpu.setSR(0x2700);
        cpu.setISP(0x1F00);

        REQUIRE_NOTHROW(cpu.execute());
        CHECK(cpu.getPC0() == FastCPU::handler);
        CHECK(cpu.getSP() < 0x1F00);                // A frame has been pushed
        REQUIRE_NOTHROW(cpu.execute());
        CHECK(cpu.getPC0() == FastCPU::handler + 2);
    }
}

TEST_SUITE_END();

}
