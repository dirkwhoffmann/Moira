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

TEST_SUITE_BEGIN("BusError");

/* A 68000 system with 1 MB of RAM that signals a bus error whenever the
 * CPU accesses a certain address. The CPU is only told the fault address.
 */
class TestCPU : public Moira {

public:

    mutable std::vector<u8> ram = std::vector<u8>(1 << 20);

    u32 faultAddr = 0xFFFFFFFF;
    bool faultOnRead = true;
    bool faultOnWrite = true;

    static constexpr u32 codeStart = 0x1000;
    static constexpr u32 stackStart = 0x0F00;
    static constexpr u32 handler = 0x4000;

    u8 read8(u32 addr) const override { check(addr, false); return ram[addr & 0xFFFFF]; }
    u16 read16(u32 addr) const override { check(addr, false); return ram[addr & 0xFFFFF] << 8 | ram[(addr + 1) & 0xFFFFF]; }
    void write8(u32 addr, u8 val) const override { check(addr, true); ram[addr & 0xFFFFF] = val; }
    u32 read32(u32 addr) const override { return read16(addr) << 16 | read16(addr + 2); }
    void write32(u32 addr, u32 val) const override { write16(addr, val >> 16); write16(addr + 2, val & 0xFFFF); }
    void write16(u32 addr, u16 val) const override { check(addr, true); ram[addr & 0xFFFFF] = val >> 8; ram[(addr + 1) & 0xFFFFF] = val & 0xFF; }

    void check(u32 addr, bool write) const
    {
        if ((addr & ~1u) != faultAddr) return;
        if (write ? faultOnWrite : faultOnRead) throw BusError(addr);
    }

    u16 peek16(u32 addr) const { return ram[addr] << 8 | ram[addr + 1]; }
    void poke16(u32 addr, u16 val) { ram[addr] = val >> 8; ram[addr + 1] = val & 0xFF; }
    void poke32(u32 addr, u32 val) { poke16(addr, val >> 16); poke16(addr + 2, val & 0xFFFF); }

    TestCPU()
    {
        setModel(CPUModel::M68000);
        poke32(0, stackStart);
        poke32(4, codeStart);
        poke32(8, handler);       // Bus error vector
        poke16(handler, 0x4E71);
        reset();
    }

    // Executes a program in user mode and returns the bus error frame (if any)
    struct Frame { u16 code; u32 addr; u16 ird; u16 sr; u32 pc; };

    Frame run(std::initializer_list<u16> words)
    {
        u32 addr = codeStart;
        for (auto w : words) { poke16(addr, w); addr += 2; }
        for (int i = 0; i < 4; i++) { poke16(addr, 0x4E71); addr += 2; }

        debugger.jump(codeStart);
        setSR(0x0000);
        setISP(0x1F00);
        setUSP(0x8000);
        execute();

        REQUIRE(getPC() == handler);

        u32 sp = getSP();
        return { peek16(sp), u32(peek16(sp + 2) << 16 | peek16(sp + 4)), peek16(sp + 6),
                 peek16(sp + 8), u32(peek16(sp + 10) << 16 | peek16(sp + 12)) };
    }
};

TEST_CASE("Moira completes bus error frames (read)")
{
    TestCPU cpu;
    cpu.faultAddr = 0x2000;
    cpu.setA(0, 0x2000);

    // MOVE.W (A0),D0
    auto f = cpu.run({ 0x3010 });

    CHECK(f.addr == 0x2000);
    CHECK(f.ird == 0x3010);
    CHECK(f.pc == 0x1002);
    CHECK((f.code & 0x10) == 0x10);     // Read access
    CHECK((f.code & 0x7) == 1);         // Function code: user data
}

TEST_CASE("Moira completes bus error frames (write)")
{
    TestCPU cpu;
    cpu.faultAddr = 0x2000;
    cpu.faultOnRead = false;
    cpu.setA(0, 0x2000);

    // MOVE.W D0,(A0)
    auto f = cpu.run({ 0x3080 });

    CHECK(f.addr == 0x2000);
    CHECK((f.code & 0x10) == 0);        // Write access
    CHECK((f.code & 0x7) == 1);
    // The next opcode has been prefetched before the write cycle, but the
    // frame still holds the instruction that is being executed
    CHECK(f.pc == 0x1004);
    CHECK(f.ird == 0x3080);
}

TEST_CASE("Bus error frames follow the addressing mode")
{
    TestCPU cpu;
    cpu.faultAddr = 0x2000;

    // ADD.W -(A0),D0
    cpu.setA(0, 0x2002);
    auto f = cpu.run({ 0xD060 });
    CHECK(f.pc == 0x1004);

    // ADD.W 0(A0),D0
    cpu.setA(0, 0x2000);
    f = cpu.run({ 0xD068, 0x0000 });
    CHECK(f.pc == 0x1002);
}

TEST_SUITE_END();

}
