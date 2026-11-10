// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "doctest.h"
#include "Moira.h"
#include <cstdlib>
#include <vector>

using namespace moira;

namespace {

TEST_SUITE_BEGIN("MMU");

/* A minimal system consisting of 1 MB of RAM
 */
class TestCPU : public Moira {

public:

    using Moira::reg;
    using Moira::mmu;
    using Moira::iCache;

    mutable std::vector<u8> ram = std::vector<u8>(1 << 20);

    int enabled = 0;
    int disabled = 0;

    // Bus statistics (data region only, i.e., addresses >= 0x20000)
    mutable std::vector<u32> longReads;
    mutable int smallReads = 0;
    u8 portDsack = DSACK_16;

    u8 dsack(u32 addr) const override { return portDsack; }
    u8 read8(u32 addr) const override { if ((addr & 0xFFFFF) >= 0x20000) smallReads++; return ram[addr & 0xFFFFF]; }
    u16 read16(u32 addr) const override { return read8(addr) << 8 | read8(addr + 1); }
    u32 read32(u32 addr) const override
    {
        if ((addr & 0xFFFFF) >= 0x20000) longReads.push_back(addr & 0xFFFFF);
        return ram[addr & 0xFFFFF] << 24 | ram[(addr + 1) & 0xFFFFF] << 16 | ram[(addr + 2) & 0xFFFFF] << 8 | ram[(addr + 3) & 0xFFFFF];
    }
    void write8(u32 addr, u8 val) const override { ram[addr & 0xFFFFF] = val; }
    void write16(u32 addr, u16 val) const override { write8(addr, val >> 8); write8(addr + 1, val & 0xFF); }
    void write32(u32 addr, u32 val) const override { write16(addr, val >> 16); write16(addr + 2, val & 0xFFFF); }

    void mmuDidEnable() override { enabled++; }
    void mmuDidDisable() override { disabled++; }

    static constexpr u32 codeStart = 0x1000;
    static constexpr u32 stackStart = 0x0F00;

    TestCPU(CPUModel cpuModel = CPUModel::M68030)
    {
        setModel(cpuModel);

        // Reset vector
        write32(0, stackStart);
        write32(4, codeStart);
        reset();
    }

    // Continues the program at a given address
    void jump(u32 addr) { debugger.jump(addr); }

    // Executes a sequence of words as a program
    void run(std::initializer_list<u16> words, int instructions = 1)
    {
        u32 addr = codeStart;
        for (auto w : words) { write16(addr, w); addr += 2; }
        write16(addr, 0x4E71);     // NOP
        write16(addr + 2, 0x4E71);

        debugger.jump(codeStart);
        for (int i = 0; i < instructions; i++) execute();
    }
};


//
// Translation tables
//

/* Page size is 4 KB with a two-level table structure (10 + 10 index bits):
 *
 *      Logical    Physical
 *      0x0000     0x0000    (vectors, stack)
 *      0x1000     0x1000    (code)
 *      0x2000     0x5000    (relocated)
 *      0x3000     ---       (invalid)
 *      0x4000     0x6000    (write protected)
 *      0x5000     0x7000
 */
constexpr u32 tableA = 0x10000;
constexpr u32 tableB = 0x11000;

constexpr u32 TC_E     = 1 << 31;
constexpr u32 TC_SRE   = 1 << 25;
constexpr u32 TC_4K_2L = 0xC << 20 | 0xA << 12 | 0xA << 8;   // PS = 12, IS = 0, TIA = 10, TIB = 10

u64 rootPointer(u32 table, int dt, u16 limit = 0x7FFF, bool lu = false)
{
    return u64(lu) << 63 | u64(limit & 0x7FFF) << 48 | u64(dt) << 32 | table;
}

void setupShortTables(TestCPU &cpu)
{
    cpu.write32(tableA, tableB | 2);                // Table descriptor (short)
    cpu.write32(tableB + 4 * 0, 0x0000 | 1);        // Page descriptors (short)
    cpu.write32(tableB + 4 * 1, 0x1000 | 1);
    cpu.write32(tableB + 4 * 2, 0x5000 | 1);
    cpu.write32(tableB + 4 * 3, 0);                 // Invalid
    cpu.write32(tableB + 4 * 4, 0x6000 | 1 | 4);    // Write protected
    cpu.write32(tableB + 4 * 5, 0x7000 | 1);

    cpu.mmu.mmu030.crp = rootPointer(tableA, 2);
    cpu.mmu.mmu030.srp = cpu.mmu.mmu030.crp;
    cpu.mmu.mmu030.tc  = TC_E | TC_4K_2L;
    cpu.updateMmuFlag();

    // Bus error vector
    cpu.write32(2 * 4, 0x1800);
}

TEST_CASE("Disabled")
{
    TestCPU cpu;
    setupShortTables(cpu);
    cpu.mmu.mmu030.tc = TC_4K_2L;                          // E bit is cleared
    cpu.updateMmuFlag();

    // Without the E bit, logical addresses are physical addresses
    cpu.write32(0x2004, 0xCAFEBABE);
    cpu.reg.a[0] = 0x2004;
    cpu.run({ 0x2010 });                            // MOVE.L (A0),D0
    CHECK(cpu.reg.d[0] == 0xCAFEBABE);
}

TEST_CASE("Short Tables")
{
    TestCPU cpu;
    setupShortTables(cpu);

    // Read from a relocated page
    cpu.write32(0x5004, 0xCAFEBABE);
    cpu.reg.a[0] = 0x2004;
    cpu.run({ 0x2010 });                            // MOVE.L (A0),D0
    CHECK(cpu.reg.d[0] == 0xCAFEBABE);

    // The page offset is preserved
    cpu.write32(0x5FFC, 0x12345678);
    cpu.reg.a[0] = 0x2FFC;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x12345678);

    // Write to a relocated page
    cpu.reg.d[1] = 0xDEADBEEF;
    cpu.reg.a[0] = 0x2008;
    cpu.run({ 0x2081 });                            // MOVE.L D1,(A0)
    CHECK(cpu.read32(0x5008) == 0xDEADBEEF);
    CHECK(cpu.read32(0x2008) == 0);

    // Reading from an identity mapped page
    cpu.write32(0x1900, 0x0BADF00D);
    cpu.reg.a[0] = 0x1900;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x0BADF00D);
}

TEST_CASE("Update Bits")
{
    TestCPU cpu;
    setupShortTables(cpu);

    // A read access sets the U bit of the page descriptor
    cpu.reg.a[0] = 0x5000;
    cpu.run({ 0x2010 });
    CHECK((cpu.read32(tableB + 4 * 5) & 0x18) == 0x08);

    // A write access sets the U and the M bit
    cpu.reg.a[0] = 0x5000;
    cpu.run({ 0x2081 });
    CHECK((cpu.read32(tableB + 4 * 5) & 0x18) == 0x18);

    // The table descriptor has been marked as used, too
    CHECK((cpu.read32(tableA) & 0x08) == 0x08);
}

TEST_CASE("Faults")
{
    // Access to an invalid page
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.write32(0x3000, 0x11223344);
        cpu.reg.a[0] = 0x3000;
        cpu.reg.d[0] = 0;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.pc0 == 0x1800);               // The CPU jumped to the bus error handler
        CHECK(cpu.reg.d[0] == 0);
    }

    // Write access to a write protected page
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.write32(0x6000, 0x11111111);
        cpu.reg.a[0] = 0x4000;
        cpu.reg.d[1] = 0x22222222;
        cpu.run({ 0x2081 });                        // MOVE.L D1,(A0)
        CHECK(cpu.reg.pc0 == 0x1800);
        CHECK(cpu.read32(0x6000) == 0x11111111);
    }

    // Read access to a write protected page
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.write32(0x6004, 0x55667788);
        cpu.reg.a[0] = 0x4004;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.pc0 != 0x1800);
        CHECK(cpu.reg.d[0] == 0x55667788);
    }

    // Access to an address that is not covered by the table
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.reg.a[0] = 0x00400000;                  // Index 1 of table A is invalid
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.pc0 == 0x1800);
    }

    // Table index exceeds the limit of the root pointer
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.write32(tableA + 4, tableB | 2);        // Make index 1 valid ...
        cpu.mmu.mmu030.crp = cpu.mmu.mmu030.srp = rootPointer(tableA, 2, 0);   // ... but limit the index to 0
        cpu.reg.a[0] = 0x00400000;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.pc0 == 0x1800);
    }
}

TEST_CASE("Early Termination")
{
    TestCPU cpu;
    setupShortTables(cpu);

    // The root pointer terminates the lookup: Physical = Table address + Logical address
    cpu.mmu.mmu030.crp = cpu.mmu.mmu030.srp = rootPointer(0x20000, 1);
    cpu.mmu.mmu030.tc  = TC_E | TC_4K_2L;
    cpu.updateMmuFlag();

    // Move the code and the stack to the new location
    cpu.write16(0x20000 + TestCPU::codeStart, 0x2010);
    cpu.write16(0x20000 + TestCPU::codeStart + 2, 0x4E71);
    cpu.write16(0x20000 + TestCPU::codeStart + 4, 0x4E71);
    cpu.write32(0x30004, 0xCAFEBABE);
    cpu.reg.a[0] = 0x10004;                         // Physical address 0x30004
    // Debugger::jump prefetches without translation, so mirror the first words
    cpu.write16(TestCPU::codeStart, 0x2010);
    cpu.write16(TestCPU::codeStart + 2, 0x4E71);
    cpu.debugger.jump(TestCPU::codeStart);
    cpu.execute();
    CHECK(cpu.reg.d[0] == 0xCAFEBABE);
}

TEST_CASE("Long Tables")
{
    TestCPU cpu;
    setupShortTables(cpu);

    static constexpr u32 longA = 0x12000;
    static constexpr u32 longB = 0x13000;

    // Long descriptors are 8 bytes wide. Table descriptors: DT = 3
    cpu.write32(longA,     0x7FFF0003);             // Limit = 0x7FFF, upper limit
    cpu.write32(longA + 4, longB);

    // Page descriptors
    auto page = [&](int idx, u32 phys, u32 flags) {
        cpu.write32(longB + 8 * idx,     flags | 1);
        cpu.write32(longB + 8 * idx + 4, phys);
    };
    page(0, 0x0000, 0);
    page(1, 0x1000, 0);
    page(2, 0x5000, 0);
    page(4, 0x6000, 0x04);                          // Write protected
    page(5, 0x7000, 0x100);                         // Supervisor only

    cpu.mmu.mmu030.crp = cpu.mmu.mmu030.srp = rootPointer(longA, 3);

    // Read from a relocated page
    cpu.write32(0x5004, 0xCAFEBABE);
    cpu.reg.a[0] = 0x2004;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0xCAFEBABE);

    // Write to a relocated page
    cpu.reg.d[1] = 0xDEADBEEF;
    cpu.reg.a[0] = 0x2008;
    cpu.run({ 0x2081 });
    CHECK(cpu.read32(0x5008) == 0xDEADBEEF);

    // Write to a write protected page
    cpu.write32(0x6000, 0x11111111);
    cpu.reg.a[0] = 0x4000;
    cpu.reg.d[1] = 0x22222222;
    cpu.run({ 0x2081 });
    CHECK(cpu.reg.pc0 == 0x1800);
    CHECK(cpu.read32(0x6000) == 0x11111111);
}

TEST_CASE("Supervisor Root Pointer")
{
    TestCPU cpu;
    setupShortTables(cpu);

    // With SRE set, the supervisor uses the SRP and the user uses the CRP.
    // The user map is empty.
    cpu.mmu.mmu030.tc  = TC_E | TC_SRE | TC_4K_2L;
    cpu.updateMmuFlag();
    cpu.mmu.mmu030.crp = rootPointer(tableA + 0x800, 2);   // Table without valid descriptors

    // Supervisor mode
    cpu.write32(0x5004, 0xCAFEBABE);
    cpu.reg.a[0] = 0x2004;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0xCAFEBABE);
}

TEST_CASE("PMove")
{
    TestCPU cpu;
    cpu.reg.a[0] = 0x2000;

    // PMOVE (A0),CRP
    cpu.write32(0x2000, 0x7FFF0002);
    cpu.write32(0x2004, 0x00012340);
    cpu.run({ 0xF010, 0x4C00 });
    CHECK(cpu.mmu.mmu030.crp == 0x7FFF000200012340ULL);

    // PMOVE CRP,(A0)
    cpu.reg.a[0] = 0x2100;
    cpu.run({ 0xF010, 0x4E00 });
    CHECK((cpu.read32(0x2100) == 0x7FFF0002 && cpu.read32(0x2104) == 0x00012340));

    // PMOVE (A0),SRP and PMOVE SRP,(A0)
    cpu.reg.a[0] = 0x2000;
    cpu.write32(0x2000, 0x80010001);
    cpu.write32(0x2004, 0x00004560);
    cpu.run({ 0xF010, 0x4800 });
    CHECK(cpu.mmu.mmu030.srp == 0x8001000100004560ULL);
    cpu.reg.a[0] = 0x2200;
    cpu.run({ 0xF010, 0x4A00 });
    CHECK((cpu.read32(0x2200) == 0x80010001 && cpu.read32(0x2204) == 0x00004560));

    // PMOVE (A0),TT0 / TT1 and back
    cpu.reg.a[0] = 0x2300;
    cpu.write32(0x2300, 0x1234ABCD);
    cpu.run({ 0xF010, 0x0800 });
    CHECK(cpu.mmu.mmu030.tt0 == 0x1234ABCD);
    cpu.run({ 0xF010, 0x0C00 });
    CHECK(cpu.mmu.mmu030.tt1 == 0x1234ABCD);
    cpu.reg.a[0] = 0x2310;
    cpu.run({ 0xF010, 0x0A00 });
    CHECK(cpu.read32(0x2310) == 0x1234ABCD);
    cpu.reg.a[0] = 0x2320;
    cpu.run({ 0xF010, 0x0E00 });
    CHECK(cpu.read32(0x2320) == 0x1234ABCD);

    // PMOVE (A0),MMUSR and back
    cpu.reg.a[0] = 0x2400;
    cpu.write16(0x2400, 0xA5A5);
    cpu.run({ 0xF010, 0x6000 });
    CHECK(cpu.mmu.mmu030.mmusr == 0xA5A5);
    cpu.reg.a[0] = 0x2410;
    cpu.run({ 0xF010, 0x6200 });
    CHECK(cpu.read16(0x2410) == 0xA5A5);
}

TEST_CASE("Enable Disable")
{
    TestCPU cpu;
    cpu.reg.a[0] = 0x2000;

    // PMOVE (A0),TC: Enable the MMU with an identity mapping (early termination)
    cpu.mmu.mmu030.crp = cpu.mmu.mmu030.srp = rootPointer(0, 1);
    cpu.write32(0x2000, TC_E | TC_4K_2L);
    cpu.run({ 0xF010, 0x4000 });
    CHECK(cpu.mmu.mmu030.tc == (TC_E | TC_4K_2L));
    CHECK((cpu.enabled == 1 && cpu.disabled == 0));

    // The MMU is active: Redirect the root pointer
    cpu.mmu.mmu030.crp = cpu.mmu.mmu030.srp = rootPointer(0x20000, 1);
    cpu.write32(0x20000 + 0x2000, 0);
    cpu.mmu.mmu030.crp = cpu.mmu.mmu030.srp = rootPointer(0, 1);

    // PMOVE (A0),TC: Disable the MMU
    cpu.write32(0x2000, TC_4K_2L);
    cpu.run({ 0xF010, 0x4000 });
    CHECK(cpu.mmu.mmu030.tc == TC_4K_2L);
    CHECK((cpu.enabled == 1 && cpu.disabled == 1));

    // PMOVE TC,(A0)
    cpu.reg.a[0] = 0x2100;
    cpu.run({ 0xF010, 0x4200 });
    CHECK(cpu.read32(0x2100) == TC_4K_2L);

    // RESET disables the MMU
    cpu.mmu.mmu030.tc = TC_E | TC_4K_2L;
    cpu.updateMmuFlag();
    cpu.mmu.mmu030.crp = cpu.mmu.mmu030.srp = rootPointer(0, 1);
    cpu.run({ 0x4E70 });
    CHECK(!(cpu.mmu.mmu030.tc & TC_E));
}

TEST_CASE("Transparent Translation")
{
    TestCPU cpu;
    cpu.reg.a[0] = 0x2000;

    // TT0: Match addresses 0x20xxxxxx, ignore R/W, function code = 1 (user data)
    cpu.mmu.mmu030.tt0 = 0x20000000 | 1 << 15 | 1 << 8 | 1 << 4;

    // PTESTR #1,(A0),#0 with a matching address
    cpu.reg.a[0] = 0x20001234;
    cpu.run({ 0xF010, 0x8000 | 1 << 9 | 0x11 });
    CHECK((cpu.mmu.mmu030.mmusr & 0x40));

    // Non-matching address
    cpu.reg.a[0] = 0x30001234;
    cpu.run({ 0xF010, 0x8000 | 1 << 9 | 0x11 });
    CHECK(!(cpu.mmu.mmu030.mmusr & 0x40));
}

TEST_CASE("Translation Bypass")
{
    TestCPU cpu;
    setupShortTables(cpu);
    cpu.write32(0x2004, 0x11111111);
    cpu.write32(0x5004, 0x22222222);
    cpu.reg.a[0] = 0x2004;

    // Without TT, the page is relocated
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x22222222);

    // TT0: Address 0x00xxxxxx, any function code, read and write
    cpu.mmu.mmu030.tt0 = 1 << 15 | 1 << 8 | 7;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x11111111);

    // A different address base doesn't match
    cpu.mmu.mmu030.tt0 = 0x20000000 | 1 << 15 | 1 << 8 | 7;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x22222222);

    // Ignoring all address bits matches everything
    cpu.mmu.mmu030.tt0 = 0x20000000 | 0xFF << 16 | 1 << 15 | 1 << 8 | 7;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x11111111);

    // TT1 works the same way, and a read-only TT (RWM = 0, RW = 1) ignores writes
    cpu.mmu.mmu030.tt0 = 0;
    cpu.mmu.mmu030.tt1 = 1 << 15 | 1 << 9 | 0xFF << 16 | 7;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x11111111);
    cpu.reg.d[1] = 0xDEADBEEF;
    cpu.run({ 0x2081 });                        // MOVE.L D1,(A0) is translated
    CHECK(cpu.read32(0x5004) == 0xDEADBEEF);

    // Function code mismatch
    cpu.mmu.mmu030.tt1 = 1 << 15 | 1 << 8 | 0xFF << 16 | 1 << 4;   // User data only
    cpu.run({ 0x2010 });                        // Supervisor data access
    CHECK(cpu.reg.d[0] == 0xDEADBEEF);
}

TEST_CASE("PTest")
{
    TestCPU cpu;
    setupShortTables(cpu);
    cpu.mmu.mmu030.tc = TC_E | TC_4K_2L;
    cpu.updateMmuFlag();

    // PTESTR #1,(A0),#2,A1 (user data, level 2, return the descriptor address)
    auto ptest = [&](u32 addr, bool read, int level, bool a) {
        cpu.reg.a[0] = addr;
        cpu.reg.a[1] = 0xDEAD0000;
        cpu.run({ 0xF010, u16(0x8000 | level << 10 | read << 9 | a << 8 | a << 5 | 0x11) });
    };

    // Valid page: Two levels, no error bits
    ptest(0x2000, true, 2, true);
    CHECK((cpu.mmu.mmu030.mmusr & 0xEE07) == 2);
    CHECK(cpu.reg.a[1] == tableB + 4 * 2);

    // Level 1 stops at the table descriptor
    ptest(0x2000, true, 1, true);
    CHECK((cpu.mmu.mmu030.mmusr & 0xEE07) == 1);
    CHECK(cpu.reg.a[1] == tableA);

    // Invalid page
    ptest(0x3000, true, 7, true);
    CHECK((cpu.mmu.mmu030.mmusr & 0x0400) == 0x0400);
    CHECK(cpu.reg.a[1] == tableB + 4 * 3);

    // Write-protected page
    ptest(0x4000, false, 7, false);
    CHECK((cpu.mmu.mmu030.mmusr & 0x0800) == 0x0800);
    CHECK(cpu.reg.a[1] == 0xDEAD0000);              // A bit not set

    // PTEST doesn't modify the tables
    u32 before = cpu.read32(tableB + 4 * 5);
    ptest(0x5000, false, 7, false);
    CHECK(cpu.read32(tableB + 4 * 5) == before);

    // Level 0 without TT: The ATC is empty
    ptest(0x2000, true, 0, false);
    CHECK((cpu.mmu.mmu030.mmusr & 0x0400) == 0x0400);

    // The A bit is illegal for level 0
    cpu.write32(4 * 4, 0x1900);                     // Illegal instruction vector
    cpu.write32(11 * 4, 0x1900);                    // Line F vector
    ptest(0x2000, true, 0, true);
    CHECK(cpu.reg.pc0 == 0x1900);
}

TEST_CASE("ATC")
{
    TestCPU cpu;
    setupShortTables(cpu);
    cpu.write32(0x5004, 0x11111111);
    cpu.write32(0x6004, 0x22222222);

    auto load = [&](u32 addr) { cpu.reg.a[0] = addr; cpu.run({ 0x2010 }); return cpu.reg.d[0]; };
    auto ptest0 = [&](u32 addr, int fc) {
        cpu.reg.a[0] = addr;
        cpu.run({ 0xF010, u16(0x8000 | 1 << 9 | 0x10 | fc) });
        return cpu.mmu.mmu030.mmusr;
    };

    // Before the first access, the ATC doesn't know the page
    CHECK(ptest0(0x2000, 5) == 0x0400);

    // Translation creates an ATC entry (supervisor data, FC = 5)
    CHECK(load(0x2004) == 0x11111111);
    CHECK(ptest0(0x2000, 5) == 0);
    CHECK(ptest0(0x2000, 1) == 0x0400);

    // Changing the table has no effect while the entry is cached
    cpu.write32(tableB + 4 * 2, 0x6000 | 1);
    CHECK(load(0x2004) == 0x11111111);

    // PFLUSHA drops the entry
    cpu.run({ 0xF000, 0x2400 });
    CHECK(ptest0(0x2000, 5) == 0x0400);
    CHECK(load(0x2004) == 0x22222222);

    // PFLUSH #5,#7,(A0) flushes a single page ...
    cpu.write32(tableB + 4 * 2, 0x5000 | 1);
    cpu.reg.a[0] = 0x2000;
    cpu.run({ 0xF010, 0x3800 | 7 << 5 | 0x10 | 5 });
    CHECK(load(0x2004) == 0x11111111);

    // ... and PFLUSH #1,#7 flushes by function code only (nothing to flush here)
    cpu.write32(tableB + 4 * 2, 0x6000 | 1);
    cpu.run({ 0xF000, 0x3000 | 7 << 5 | 0x10 | 1 });
    CHECK(load(0x2004) == 0x11111111);
    cpu.run({ 0xF000, 0x3000 | 7 << 5 | 0x10 | 5 });
    CHECK(load(0x2004) == 0x22222222);

    // PMOVE to a translation register flushes the ATC
    cpu.write32(tableB + 4 * 2, 0x5000 | 1);
    cpu.reg.a[0] = 0x1800;                          // (identity mapped)
    cpu.write32(0x1800, 0x7FFF0002);
    cpu.write32(0x1804, tableA);
    cpu.run({ 0xF010, 0x4C00 });                    // PMOVE (A0),CRP
    CHECK(load(0x2004) == 0x11111111);

    // PLOAD creates an entry, including the write-protect state
    cpu.run({ 0xF000, 0x2400 });
    cpu.reg.a[0] = 0x4000;
    cpu.run({ 0xF010, 0x2000 | 0x10 | 5 });         // PLOADW #5,(A0)
    CHECK(ptest0(0x4000, 5) == 0x0800);

    // Invalid pages are cached, too
    cpu.run({ 0xF000, 0x2400 });
    cpu.write32(0x3004, 0);
    cpu.reg.a[0] = 0x3004;
    cpu.write32(2 * 4, 0x1800);
    cpu.run({ 0x2010 });                            // Bus error
    CHECK(ptest0(0x3000, 5) == 0x8400);
}


TEST_CASE("Bus Fault Frames")
{
    // A read fault pushes a long bus fault frame (format $B)
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.reg.a[0] = 0x3000;
        cpu.run({ 0x2018, 0xABCD, 0xEF01 });        // MOVE.L (A0)+,D0

        u32 sp = cpu.reg.sp;
        CHECK(sp == TestCPU::stackStart - 92);
        CHECK(cpu.read32(sp + 0x18) == 0);                              // Data output buffer
        CHECK(cpu.read16(sp + 0x0C) == 0xABCD);                         // Instruction pipe stage C
        CHECK(cpu.read16(sp + 0x0E) == 0xEF01);                         // Instruction pipe stage B
        CHECK(cpu.read16(sp + 6) == (0xB000 | 2 * 4));
        CHECK(cpu.read32(sp + 2) == TestCPU::codeStart);                // Restart address
        CHECK(cpu.read16(sp + 0x0A) == (SSW030::DF | SSW030::RW | SSW030::SIZ_L | FC::SUPERVISOR_DATA));
        CHECK(cpu.read32(sp + 0x10) == 0x3000);                         // Fault address
        CHECK(cpu.reg.a[0] == 0x3000);                                  // A0 has been rolled back
    }

    // A write fault pushes a short bus fault frame (format $A)
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.reg.a[0] = 0x4000;
        cpu.reg.d[1] = 0x22222222;
        cpu.run({ 0x20C1, 0x1234, 0x5678 });        // MOVE.L D1,(A0)+

        u32 sp = cpu.reg.sp;
        CHECK(sp == TestCPU::stackStart - 32);
        CHECK(cpu.read32(sp + 0x18) == 0x22222222);                     // Data output buffer
        CHECK(cpu.read16(sp + 0x0C) == 0x1234);                         // Instruction pipe stage C
        CHECK(cpu.read16(sp + 0x0E) == 0x5678);                         // Instruction pipe stage B
        CHECK(cpu.read16(sp + 6) == (0xA000 | 2 * 4));
        CHECK(cpu.read16(sp + 0x0A) == (SSW030::DF | SSW030::SIZ_L | FC::SUPERVISOR_DATA));
        CHECK(cpu.read32(sp + 0x10) == 0x4000);
        CHECK(cpu.reg.a[0] == 0x4000);
    }

    // A byte access reports its size
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.reg.a[0] = 0x3001;
        cpu.run({ 0x1010 });                        // MOVE.B (A0),D0
        CHECK((cpu.read16(cpu.reg.sp + 0x0A) & 0x30) == SSW030::SIZ_B);
    }

    // The instruction is restarted when the handler returns with RTE
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.write32(0x8004, 0xCAFEBABE);
        cpu.write16(0x1800, 0x4E73);                // RTE
        cpu.reg.a[0] = 0x3004;
        cpu.reg.sr.ipl = 0;
        cpu.run({ 0x2018 });                        // MOVE.L (A0)+,D0
        CHECK(cpu.reg.pc0 == 0x1800);

        // Map the page and return
        cpu.write32(tableB + 4 * 3, 0x8000 | 1);
        cpu.mmu.mmu030.atcFlush();
        cpu.execute();                              // RTE
        CHECK(cpu.reg.sp == TestCPU::stackStart);
        CHECK(cpu.reg.pc0 == TestCPU::codeStart);
        cpu.execute();                              // MOVE.L (A0)+,D0
        CHECK(cpu.reg.d[0] == 0xCAFEBABE);
        CHECK(cpu.reg.a[0] == 0x3008);
    }

    // A fault while prefetching the next instruction aborts the current one
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.write32(tableB + 4 * 5, 0x7000 | 1);
        cpu.write16(0x7FFC, 0x5291);                // ADDQ.L #1,(A1) at logical address 0x5FFC
        cpu.write16(0x7FFE, 0x4E71);                // Followed by a NOP in the same page
        cpu.write32(0x5000, 41);                    // Logical address 0x2000
        cpu.write16(0x1800, 0x4E73);                // RTE
        cpu.reg.a[1] = 0x2000;
        cpu.jump(0x5FFC);                           // The next page (0x6000) is invalid
        cpu.execute();

        u32 sp = cpu.reg.sp;
        CHECK(cpu.read16(sp + 6) == (0xB000 | 2 * 4));
        CHECK(cpu.read32(sp + 2) == 0x5FFC);        // Restart address
        CHECK(cpu.read16(sp + 0x0A) == (SSW030::FB | SSW030::RB | SSW030::RW | SSW030::SIZ_W | FC::SUPERVISOR_PROG));
        CHECK(cpu.read32(sp + 0x10) == 0x6000);
        CHECK(cpu.read32(0x5000) == 41);            // The memory is still untouched

        // Map the page and return. The memory is updated exactly once.
        cpu.write32(tableB + 4 * 6, 0x9000 | 1);
        cpu.mmu.mmu030.atcFlush();
        cpu.execute();                              // RTE
        cpu.execute();                              // ADDQ.L #1,(A1)
        CHECK(cpu.read32(0x5000) == 42);
        CHECK(cpu.reg.pc0 == 0x5FFE);
    }
}

TEST_CASE("Models")
{
    // Only the 68030 and 68040 have an MMU
    CHECK(!TestCPU(CPUModel::M68000).hasMMU());
    CHECK(!TestCPU(CPUModel::M68020).hasMMU());
    CHECK(TestCPU(CPUModel::M68030).hasMMU());
    CHECK(TestCPU(CPUModel::M68040).hasMMU());

    // NONE removes the MMU (e.g., to emulate a 68EC030). The selection survives a CPU change.
    TestCPU cpu(CPUModel::M68030);
    cpu.setMmuModel(MMUModel::NONE);
    CHECK(!cpu.hasMMU());
    cpu.setModel(CPUModel::M68040);
    CHECK(!cpu.hasMMU());
    cpu.setMmuModel(MMUModel::BUILTIN);
    CHECK(cpu.hasMMU());
    cpu.setModel(CPUModel::M68020);
    CHECK(!cpu.hasMMU());

    // BUILTIN resolves to the MMU type of the CPU
    {
        TestCPU c030(CPUModel::M68030);
        CHECK(c030.mmu.getModel() == MMUModel::M68030);
        TestCPU c040(CPUModel::M68040);
        CHECK(c040.mmu.getModel() == MMUModel::M68040);
        TestCPU c020(CPUModel::M68020);
        CHECK(c020.mmu.getModel() == MMUModel::NONE);
    }

    // Selecting a type only has an effect if the CPU provides it
    {
        TestCPU c030(CPUModel::M68030);
        c030.setMmuModel(MMUModel::M68030);
        CHECK(c030.mmu.getModel() == MMUModel::M68030);
        c030.setMmuModel(MMUModel::M68040);
        CHECK(c030.mmu.getModel() == MMUModel::NONE);

        TestCPU c040(CPUModel::M68040);
        c040.setMmuModel(MMUModel::M68030);
        CHECK(c040.mmu.getModel() == MMUModel::NONE);
        c040.setMmuModel(MMUModel::M68040);
        CHECK(c040.mmu.getModel() == MMUModel::M68040);
    }
}

constexpr u32 dataBase = 0x20000;

TEST_CASE("Data Cache")
{
    // Without a cache, data accesses always reach the bus
    {
        TestCPU cpu;
        cpu.write32(dataBase, 0x11223344);
        cpu.reg.a[0] = dataBase;
        cpu.run({ 0x2010, 0x2010 }, 2);             // MOVE.L (A0),D0 (twice)
        CHECK(cpu.reg.d[0] == 0x11223344);
        CHECK(cpu.smallReads == 8);
    }

    // With the cache enabled, the second access is a hit
    {
        TestCPU cpu;
        cpu.write32(dataBase, 0x11223344);
        cpu.setCACR(CACR::ED);
        cpu.reg.a[0] = dataBase;
        cpu.run({ 0x2010, 0x2010 }, 2);
        CHECK(cpu.reg.d[0] == 0x11223344);
        CHECK(cpu.smallReads == 4);                 // One longword fetch via a 16-bit port

        // Subword accesses are served from the cached longword
        cpu.run({ 0x1028, 0x0003 }, 1);             // MOVE.B 3(A0),D0
        CHECK((cpu.reg.d[0] & 0xFF) == 0x44);
        CHECK(cpu.smallReads == 4);
    }

    // Write-through: Memory is updated, a cached longword is updated, too
    {
        TestCPU cpu;
        cpu.write32(dataBase, 0x11223344);
        cpu.setCACR(CACR::ED);
        cpu.reg.a[0] = dataBase;
        cpu.run({ 0x2010 });                        // Fill
        cpu.reg.d[1] = 0xAA;
        cpu.run({ 0x1141, 0x0001 });                // MOVE.B D1,1(A0)
        CHECK(cpu.ram[dataBase + 1] == 0xAA);
        int reads = cpu.smallReads;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0x11AA3344);
        CHECK(cpu.smallReads == reads);
    }

    // A write miss does not allocate, unless WA is set
    {
        TestCPU cpu;
        cpu.setCACR(CACR::ED);
        cpu.reg.a[0] = dataBase;
        cpu.reg.d[1] = 0x55667788;
        cpu.run({ 0x2081 });                        // MOVE.L D1,(A0)
        cpu.run({ 0x2010 });
        CHECK(cpu.smallReads == 4);

        TestCPU cpu2;
        cpu2.setCACR(CACR::ED | CACR::WA);
        cpu2.reg.a[0] = dataBase;
        cpu2.reg.d[1] = 0x55667788;
        cpu2.run({ 0x2081 });
        cpu2.run({ 0x2010 });
        CHECK(cpu2.smallReads == 0);
        CHECK(cpu2.reg.d[0] == 0x55667788);
    }

    // Clearing and freezing
    {
        TestCPU cpu;
        cpu.write32(dataBase, 0x11223344);
        cpu.setCACR(CACR::ED | CACR::FD);
        cpu.reg.a[0] = dataBase;
        cpu.run({ 0x2010, 0x2010 }, 2);
        CHECK(cpu.smallReads == 8);                 // Frozen: no allocation

        cpu.setCACR(CACR::ED);
        cpu.run({ 0x2010, 0x2010 }, 2);
        CHECK(cpu.smallReads == 12);
        cpu.setCACR(CACR::ED | CACR::CD);           // Clear
        cpu.run({ 0x2010 });
        CHECK(cpu.smallReads == 16);

        cpu.setCACR(CACR::ED | CACR::CED);          // Clear entry selected by CAAR
        cpu.setCAAR(dataBase);
        cpu.setCACR(CACR::ED | CACR::CED);
        cpu.run({ 0x2010 });
        CHECK(cpu.smallReads == 20);
    }

    // Logical tagging: A hit does not involve the MMU
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.write32(0x5000, 0xCAFEBABE);            // Physical page of logical 0x2000
        cpu.setCACR(CACR::ED);
        cpu.reg.a[0] = 0x2000;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0xCAFEBABE);

        // Invalidate the page, the cached longword is still reachable
        cpu.write32(tableB + 4 * 2, 0);
        cpu.mmu.mmu030.atcFlush();
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0xCAFEBABE);
        CHECK(cpu.reg.sp == TestCPU::stackStart);   // No exception
    }

    // Cache-inhibited pages bypass the cache
    {
        TestCPU cpu;
        setupShortTables(cpu);
        cpu.write32(tableB + 4 * 2, 0x5000 | 1 | 0x40);     // CI bit
        cpu.write32(0x5000, 0xCAFEBABE);
        cpu.setCACR(CACR::ED);
        cpu.reg.a[0] = 0x2000;
        cpu.run({ 0x2010, 0x2010 }, 2);
        CHECK(cpu.reg.d[0] == 0xCAFEBABE);
        CHECK(cpu.smallReads == 0);                 // (data region only counts >= 0x20000)
        cpu.write32(0x5000, 0x01020304);
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0x01020304);          // Not cached
    }

    // Burst fills: with a 32-bit port, a miss fetches the entire line in wrap-around order
    {
        TestCPU cpu;
        cpu.portDsack = DSACK_32;
        for (int i = 0; i < 4; i++) cpu.write32(dataBase + 4 * i, 0x01010101 * (i + 1));

        // Without DBE: one longword
        cpu.setCACR(CACR::ED);
        cpu.reg.a[0] = dataBase + 8;
        cpu.run({ 0x2010 });
        CHECK(cpu.longReads.size() == 1);

        // With DBE: four longwords, starting with the missed one
        TestCPU cpu2;
        cpu2.portDsack = DSACK_32;
        for (int i = 0; i < 4; i++) cpu2.write32(dataBase + 4 * i, 0x01010101 * (i + 1));
        cpu2.setCACR(CACR::ED | CACR::DBE);
        cpu2.reg.a[0] = dataBase + 8;
        cpu2.run({ 0x2010 });
        CHECK(cpu2.longReads.size() == 4);
        if (cpu2.longReads.size() == 4) {
            CHECK(cpu2.longReads[0] == dataBase + 8);
            CHECK(cpu2.longReads[1] == dataBase + 12);
            CHECK(cpu2.longReads[2] == dataBase + 0);
            CHECK(cpu2.longReads[3] == dataBase + 4);
        }
        cpu2.reg.a[0] = dataBase + 4;
        cpu2.run({ 0x2010 });
        CHECK(cpu2.longReads.size() == 4);          // Hit
        CHECK(cpu2.reg.d[0] == 0x02020202);

        // No burst through a 16-bit port
        TestCPU cpu3;
        cpu3.setCACR(CACR::ED | CACR::DBE);
        cpu3.reg.a[0] = dataBase + 8;
        cpu3.run({ 0x2010 });
        CHECK(cpu3.smallReads == 4);
    }

    // The 68020 has no data cache
    {
        TestCPU cpu(CPUModel::M68020);
        cpu.write32(dataBase, 0x11223344);
        cpu.setCACR(0x3313);
        cpu.reg.a[0] = dataBase;
        cpu.run({ 0x2010, 0x2010 }, 2);
        CHECK(cpu.smallReads == 8);
    }

    // The instruction cache of the 68030 holds four longwords per line, the 68020 one
    {
        TestCPU cpu;
        CHECK(cpu.iCache.numLongs() == 4);
        CHECK(cpu.iCache.numLines() == 16);
        cpu.setModel(CPUModel::M68020);
        CHECK(cpu.iCache.numLongs() == 1);
        CHECK(cpu.iCache.numLines() == 64);
    }
}


//
// 68040 MMU
//

/* Page size is 4 KB with the fixed three-level table structure:
 *
 *      Logical    Physical
 *      0x0000     0x0000    (vectors, stack)
 *      0x1000     0x1000    (code)
 *      0x2000     0x5000    (relocated)
 *      0x3000     ---       (invalid)
 *      0x4000     0x6000    (write protected)
 *      0x5000     0x7000
 */
constexpr u32 rootTable    = 0x10000;
constexpr u32 pointerTable = 0x10200;
constexpr u32 pageTable    = 0x10400;

constexpr u32 TC40_E = 1 << 15;
constexpr u32 TC40_P = 1 << 14;

void setupTables040(TestCPU &cpu, bool page8k = false)
{
    cpu.write32(rootTable, pointerTable | 2);
    cpu.write32(pointerTable, pageTable | 2);

    if (page8k) {

        // 8K pages: Logical 0x2000 -> 0x6000
        cpu.write32(pageTable + 4 * 0, 0x0000 | 1);
        cpu.write32(pageTable + 4 * 1, 0x6000 | 1);
        cpu.write32(pageTable + 4 * 2, 0xA000 | 1);

    } else {

        cpu.write32(pageTable + 4 * 0, 0x0000 | 1);
        cpu.write32(pageTable + 4 * 1, 0x1000 | 1);
        cpu.write32(pageTable + 4 * 2, 0x5000 | 1);
        cpu.write32(pageTable + 4 * 3, 0);                 // Invalid
        cpu.write32(pageTable + 4 * 4, 0x6000 | 1 | 4);    // Write protected
        cpu.write32(pageTable + 4 * 5, 0x7000 | 1);
    }

    cpu.mmu.mmu040.urp = rootTable;
    cpu.mmu.mmu040.srp = rootTable;
    cpu.mmu.mmu040.setTC(TC40_E | (page8k ? TC40_P : 0));

    // Bus error vector
    cpu.write32(2 * 4, 0x1800);
    cpu.reg.dfc = FC::SUPERVISOR_DATA;
    cpu.reg.sfc = FC::SUPERVISOR_DATA;
}

TEST_CASE("MMU040")
{
    // Address translation
    {
        TestCPU cpu(CPUModel::M68040);
        CHECK(cpu.mmu.getModel() == MMUModel::M68040);
        CHECK(!cpu.mmu.isEnabled());
        setupTables040(cpu);
        CHECK(cpu.mmu.isEnabled());
        CHECK(cpu.enabled == 1);

        // Read from a relocated page
        cpu.write32(0x5004, 0xCAFEBABE);
        cpu.reg.a[0] = 0x2004;
        cpu.run({ 0x2010 });                        // MOVE.L (A0),D0
        CHECK(cpu.reg.d[0] == 0xCAFEBABE);

        // The U bit has been set in all three levels
        CHECK((cpu.read32(rootTable) & 8) == 8);
        CHECK((cpu.read32(pointerTable) & 8) == 8);
        CHECK((cpu.read32(pageTable + 4 * 2) & 0x18) == 0x08);

        // A write access sets the M bit, too (although the page is in the ATC)
        cpu.reg.d[1] = 0xDEADBEEF;
        cpu.reg.a[0] = 0x2008;
        cpu.run({ 0x2081 });                        // MOVE.L D1,(A0)
        CHECK(cpu.read32(0x5008) == 0xDEADBEEF);
        CHECK(cpu.read32(0x2008) == 0);
        CHECK((cpu.read32(pageTable + 4 * 2) & 0x18) == 0x18);

        // The page offset is preserved
        cpu.write32(0x5FFC, 0x12345678);
        cpu.reg.a[0] = 0x2FFC;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0x12345678);

        // Disabling translation
        cpu.mmu.mmu040.setTC(0);
        CHECK(!cpu.mmu.isEnabled());
        CHECK(cpu.disabled == 1);
        cpu.write32(0x2004, 0x11111111);
        cpu.reg.a[0] = 0x2004;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0x11111111);
    }

    // 8K pages
    {
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu, true);
        CHECK(cpu.mmu.mmu040.pageMask() == 0x1FFF);

        cpu.write32(0x7004, 0xCAFEBABE);            // Logical 0x3004
        cpu.reg.a[0] = 0x3004;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0xCAFEBABE);
    }

    // Faults (format $7)
    {
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu);
        cpu.reg.a[0] = 0x3000;
        cpu.run({ 0x2018 });                        // MOVE.L (A0)+,D0

        u32 sp = cpu.reg.sp;
        CHECK(sp == TestCPU::stackStart - 60);
        CHECK(cpu.read16(sp + 6) == (0x7000 | 2 * 4));
        CHECK(cpu.read32(sp + 2) == TestCPU::codeStart);
        CHECK(cpu.read16(sp + 0x0C) == (SSW040::ATC | SSW040::RW | SSW040::SIZ_L | FC::SUPERVISOR_DATA));
        CHECK(cpu.read32(sp + 0x14) == 0x3000);     // Fault address
        CHECK(cpu.read16(sp + 0x0E) == 0);          // No write-back
        CHECK(cpu.reg.a[0] == 0x3000);              // A0 has been rolled back
    }
    {
        // Write to a write protected page
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu);
        cpu.reg.a[0] = 0x4000;
        cpu.reg.d[1] = 0x22222222;
        cpu.run({ 0x20C1 });                        // MOVE.L D1,(A0)+

        u32 sp = cpu.reg.sp;
        CHECK(sp == TestCPU::stackStart - 60);
        CHECK(cpu.read16(sp + 0x0C) == (SSW040::ATC | SSW040::SIZ_L | FC::SUPERVISOR_DATA));
        CHECK(cpu.read16(sp + 0x0E) == (0x80 | FC::SUPERVISOR_DATA));     // Write-back 3 status
        CHECK(cpu.read32(sp + 0x14) == 0x4000);
        CHECK(cpu.read32(sp + 0x18) == 0x4000);     // WB3A
        CHECK(cpu.read32(sp + 0x1C) == 0x22222222); // WB3D
        CHECK(cpu.read32(0x6000) == 0);

        // Reading the page is fine
        cpu.reg.a[0] = 0x4000;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0);
    }
    {
        // The instruction is restarted when the handler returns with RTE
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu);
        cpu.write32(0x8004, 0xCAFEBABE);
        cpu.write16(0x1800, 0x4E73);                // RTE
        cpu.reg.a[0] = 0x3004;
        cpu.reg.sr.ipl = 0;
        cpu.run({ 0x2018 });                        // MOVE.L (A0)+,D0
        CHECK(cpu.reg.pc0 == 0x1800);

        cpu.write32(pageTable + 4 * 3, 0x8000 | 1);
        cpu.mmu.mmu040.atcFlush();
        cpu.execute();                              // RTE
        CHECK(cpu.reg.sp == TestCPU::stackStart);
        CHECK(cpu.reg.pc0 == TestCPU::codeStart);
        cpu.execute();                              // MOVE.L (A0)+,D0
        CHECK(cpu.reg.d[0] == 0xCAFEBABE);
        CHECK(cpu.reg.a[0] == 0x3008);
    }

    // Supervisor-only pages
    {
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu);
        cpu.write32(pageTable + 4 * 5, 0x7000 | 1 | 0x80);
        cpu.write32(0x7000, 0x600DF00D);
        cpu.reg.a[0] = 0x5000;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0x600DF00D);

        cpu.mmu.mmu040.atcFlush();
        cpu.write32(pageTable + 4 * 5, 0x7000 | 1 | 0x80);
        auto &mmu = cpu.mmu.mmu040;
        CHECK(!mmu.translate(0x5000, FC::USER_DATA, false).has_value());
        CHECK(mmu.translate(0x5000, FC::SUPERVISOR_DATA, false).has_value());
    }

    // Write protection is inherited from the upper levels
    {
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu);
        cpu.write32(pointerTable, pageTable | 2 | 4);
        auto &mmu = cpu.mmu.mmu040;
        CHECK(mmu.translate(0x2000, FC::SUPERVISOR_DATA, false) == 0x5000);
        CHECK(!mmu.translate(0x2000, FC::SUPERVISOR_DATA, true).has_value());
    }

    // Indirect page descriptors
    {
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu);
        cpu.write32(0x10800, 0x9000 | 1);
        cpu.write32(pageTable + 4 * 2, 0x10800 | 2);
        CHECK(cpu.mmu.mmu040.translate(0x2123, FC::SUPERVISOR_DATA, false) == 0x9123);
    }

    // Transparent translation
    {
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu);
        auto &mmu = cpu.mmu.mmu040;

        // Address range 0x00xxxxxx, any privilege level
        mmu.setDTT0(0x00FF8000 | 0x4000);
        cpu.write32(0x3000, 0xFEEDFACE);
        cpu.reg.a[0] = 0x3000;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.d[0] == 0xFEEDFACE);

        // Instruction fetches are not affected by the DTT registers
        CHECK(!mmu.translate(0x3000, FC::SUPERVISOR_PROG, false).has_value());
        mmu.setITT1(0x00FF8000 | 0x4000);
        CHECK(mmu.translate(0x3000, FC::SUPERVISOR_PROG, false) == 0x3000);

        // The supervisor field selects the privilege level
        mmu.setDTT0(0x00FF8000 | 0x2000);           // Supervisor only
        CHECK(mmu.translate(0x3000, FC::SUPERVISOR_DATA, false) == 0x3000);
        CHECK(!mmu.translate(0x3000, FC::USER_DATA, false).has_value());

        // The W bit makes writes fault
        mmu.setDTT0(0x00FF8000 | 0x4000 | 0x4);
        CHECK(mmu.translate(0x3000, FC::SUPERVISOR_DATA, false) == 0x3000);
        CHECK(!mmu.translate(0x3000, FC::SUPERVISOR_DATA, true).has_value());

        // Cache mode
        mmu.setDTT0(0x00FF8000 | 0x4000 | 0x40);
        bool ci = false;
        CHECK(mmu.translate(0x3000, FC::SUPERVISOR_DATA, false, &ci) == 0x3000);
        CHECK(ci);

        // Reserved bits are not writable
        mmu.setDTT1(0xFFFFFFFF);
        CHECK(mmu.dtt1 == 0xFFFFE364);
    }

    // PTEST
    {
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu);
        auto &mmu = cpu.mmu.mmu040;

        cpu.reg.a[0] = 0x2004;
        cpu.run({ 0xF568 });                        // PTESTR (A0)
        CHECK(mmu.mmusr == (0x5000 | MMUSR040_RESIDENT));

        cpu.run({ 0xF548 });                        // PTESTW (A0)
        CHECK(mmu.mmusr == (0x5000 | MMUSR040_RESIDENT | MMUSR040_MODIFIED));

        cpu.reg.a[0] = 0x3000;                      // Invalid
        cpu.run({ 0xF568 });
        CHECK(mmu.mmusr == 0);

        cpu.reg.a[0] = 0x4000;                      // Write protected
        cpu.run({ 0xF568 });
        CHECK(mmu.mmusr == (0x6000 | MMUSR040_WRITE_PROTECTED | MMUSR040_RESIDENT));

        mmu.setDTT0(0x00FF8000 | 0x4000);           // Transparent
        cpu.reg.a[0] = 0x3000;
        cpu.run({ 0xF568 });
        CHECK(mmu.mmusr == (MMUSR040_TRANSPARENT | MMUSR040_RESIDENT));
    }

    // PFLUSH
    {
        TestCPU cpu(CPUModel::M68040);
        setupTables040(cpu);

        auto readA0 = [&](u32 addr) {
            cpu.reg.a[0] = addr;
            cpu.run({ 0x2010 });
            return cpu.reg.d[0];
        };
        cpu.write32(0x5000, 1);
        cpu.write32(0x9000, 2);
        CHECK(readA0(0x2000) == 1);

        // The ATC still serves the old translation
        cpu.write32(pageTable + 4 * 2, 0x9000 | 1);
        CHECK(readA0(0x2000) == 1);

        // PFLUSH (A0)
        cpu.reg.a[0] = 0x2000;
        cpu.run({ 0xF508 });
        CHECK(readA0(0x2000) == 2);

        // PFLUSHA
        cpu.write32(pageTable + 4 * 2, 0x5000 | 1);
        cpu.run({ 0xF518 });
        CHECK(readA0(0x2000) == 1);

        // Global entries survive PFLUSHAN and PFLUSHN, but not PFLUSHA and PFLUSH
        cpu.write32(pageTable + 4 * 2, 0x9000 | 1 | 0x400);
        cpu.run({ 0xF518 });
        CHECK(readA0(0x2000) == 2);
        cpu.write32(pageTable + 4 * 2, 0x5000 | 1);
        cpu.run({ 0xF510 });                        // PFLUSHAN
        CHECK(readA0(0x2000) == 2);
        cpu.reg.a[0] = 0x2000;
        cpu.run({ 0xF500 });                        // PFLUSHN (A0)
        CHECK(readA0(0x2000) == 2);
        cpu.reg.a[0] = 0x2000;
        cpu.run({ 0xF508 });                        // PFLUSH (A0)
        CHECK(readA0(0x2000) == 1);
    }

    // Control registers
    {
        TestCPU cpu(CPUModel::M68040);
        cpu.write32(2 * 4, 0x1800);
        cpu.write32(4 * 4, 0x1900);

        // Only the E and the P bit are writable (E stays clear here, because
        // the tables do not exist)
        cpu.reg.d[0] = 0x7FFF;
        cpu.run({ 0x4E7B, 0x0003 });                // MOVEC D0,TC
        CHECK(cpu.mmu.mmu040.tc == 0x4000);
        CHECK(!cpu.mmu.isEnabled());

        cpu.run({ 0x4E7A, 0x1003 });                // MOVEC TC,D1
        CHECK(cpu.reg.d[1] == 0x4000);

        cpu.reg.d[0] = 0x12345678;
        cpu.run({ 0x4E7B, 0x0806 });                // MOVEC D0,URP
        cpu.run({ 0x4E7B, 0x0807 });                // MOVEC D0,SRP
        cpu.run({ 0x4E7B, 0x0805 });                // MOVEC D0,MMUSR
        cpu.run({ 0x4E7B, 0x0004 });                // MOVEC D0,ITT0
        cpu.run({ 0x4E7B, 0x0007 });                // MOVEC D0,DTT1
        CHECK(cpu.mmu.mmu040.urp == 0x12345678);
        CHECK(cpu.mmu.mmu040.srp == 0x12345678);
        CHECK(cpu.mmu.mmu040.mmusr == 0x12345678);
        CHECK(cpu.mmu.mmu040.itt0 == (0x12345678 & 0xFFFFE364));
        CHECK(cpu.mmu.mmu040.dtt1 == (0x12345678 & 0xFFFFE364));

        cpu.run({ 0x4E7A, 0x2806 });                // MOVEC URP,D2
        CHECK(cpu.reg.d[2] == 0x12345678);

        // The CAAR does not exist
        cpu.run({ 0x4E7B, 0x0802 });
        CHECK(cpu.reg.pc0 == 0x1900);

        // Enabling translation
        cpu.mmu.mmu040.setTC(0);
        cpu.mmu.mmu040.setTC(0xC000);
        CHECK(cpu.mmu.isEnabled());
    }

    // The 68EC040 has no MMU, but transparent translation registers
    {
        TestCPU cpu(CPUModel::M68EC040);
        CHECK(!cpu.mmu.isPresent());
        cpu.reg.d[0] = 0xFFFFFFFF;
        cpu.run({ 0x4E7B, 0x0003 });
        CHECK(cpu.mmu.mmu040.tc == 0);
        cpu.run({ 0x4E7B, 0x0006 });
        CHECK(cpu.mmu.mmu040.dtt0 == 0xFFFFE364);
    }
}

TEST_SUITE_END();

}
