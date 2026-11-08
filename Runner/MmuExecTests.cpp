// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "Moira.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace moira;

static int failures = 0;
static int checks = 0;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
    printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

/* A minimal system consisting of 1 MB of RAM
 */
class TestCPU : public Moira {

public:

    using Moira::reg;
    using Moira::mmu;

    mutable std::vector<u8> ram = std::vector<u8>(1 << 20);

    int enabled = 0;
    int disabled = 0;

    u8 read8(u32 addr) const override { return ram[addr & 0xFFFFF]; }
    u16 read16(u32 addr) const override { return read8(addr) << 8 | read8(addr + 1); }
    u32 read32(u32 addr) const override { return read16(addr) << 16 | read16(addr + 2); }
    void write8(u32 addr, u8 val) const override { ram[addr & 0xFFFFF] = val; }
    void write16(u32 addr, u16 val) const override { write8(addr, val >> 8); write8(addr + 1, val & 0xFF); }
    void write32(u32 addr, u32 val) const override { write16(addr, val >> 16); write16(addr + 2, val & 0xFFFF); }

    void mmuDidEnable() override { enabled++; }
    void mmuDidDisable() override { disabled++; }

    static constexpr u32 codeStart = 0x1000;
    static constexpr u32 stackStart = 0x0F00;

    TestCPU(Model cpuModel = Model::M68030)
    {
        setModel(cpuModel);

        // Reset vector
        write32(0, stackStart);
        write32(4, codeStart);
        reset();
    }

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
static constexpr u32 tableA = 0x10000;
static constexpr u32 tableB = 0x11000;

static constexpr u32 TC_E     = 1 << 31;
static constexpr u32 TC_SRE   = 1 << 25;
static constexpr u32 TC_4K_2L = 0xC << 20 | 0xA << 12 | 0xA << 8;   // PS = 12, IS = 0, TIA = 10, TIB = 10

static u64 rootPointer(u32 table, int dt, u16 limit = 0x7FFF, bool lu = false)
{
    return u64(lu) << 63 | u64(limit & 0x7FFF) << 48 | u64(dt) << 32 | table;
}

static void setupShortTables(TestCPU &cpu)
{
    cpu.write32(tableA, tableB | 2);                // Table descriptor (short)
    cpu.write32(tableB + 4 * 0, 0x0000 | 1);        // Page descriptors (short)
    cpu.write32(tableB + 4 * 1, 0x1000 | 1);
    cpu.write32(tableB + 4 * 2, 0x5000 | 1);
    cpu.write32(tableB + 4 * 3, 0);                 // Invalid
    cpu.write32(tableB + 4 * 4, 0x6000 | 1 | 4);    // Write protected
    cpu.write32(tableB + 4 * 5, 0x7000 | 1);

    cpu.mmu.crp = rootPointer(tableA, 2);
    cpu.mmu.srp = cpu.mmu.crp;
    cpu.mmu.tc  = TC_E | TC_4K_2L;

    // Bus error vector
    cpu.write32(2 * 4, 0x1800);
}

static void testDisabled()
{
    TestCPU cpu;
    setupShortTables(cpu);
    cpu.mmu.tc = TC_4K_2L;                          // E bit is cleared

    // Without the E bit, logical addresses are physical addresses
    cpu.write32(0x2004, 0xCAFEBABE);
    cpu.reg.a[0] = 0x2004;
    cpu.run({ 0x2010 });                            // MOVE.L (A0),D0
    CHECK(cpu.reg.d[0] == 0xCAFEBABE);
}

static void testShortTables()
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

static void testUpdateBits()
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

static void testFaults()
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
        cpu.mmu.crp = cpu.mmu.srp = rootPointer(tableA, 2, 0);   // ... but limit the index to 0
        cpu.reg.a[0] = 0x00400000;
        cpu.run({ 0x2010 });
        CHECK(cpu.reg.pc0 == 0x1800);
    }
}

static void testEarlyTermination()
{
    TestCPU cpu;
    setupShortTables(cpu);

    // The root pointer terminates the lookup: Physical = Table address + Logical address
    cpu.mmu.crp = cpu.mmu.srp = rootPointer(0x20000, 1);
    cpu.mmu.tc  = TC_E | TC_4K_2L;

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

static void testLongTables()
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

    cpu.mmu.crp = cpu.mmu.srp = rootPointer(longA, 3);

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

static void testSupervisorRootPointer()
{
    TestCPU cpu;
    setupShortTables(cpu);

    // With SRE set, the supervisor uses the SRP and the user uses the CRP.
    // The user map is empty.
    cpu.mmu.tc  = TC_E | TC_SRE | TC_4K_2L;
    cpu.mmu.crp = rootPointer(tableA + 0x800, 2);   // Table without valid descriptors

    // Supervisor mode
    cpu.write32(0x5004, 0xCAFEBABE);
    cpu.reg.a[0] = 0x2004;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0xCAFEBABE);
}

static void testPMove()
{
    TestCPU cpu;
    cpu.reg.a[0] = 0x2000;

    // PMOVE (A0),CRP
    cpu.write32(0x2000, 0x7FFF0002);
    cpu.write32(0x2004, 0x00012340);
    cpu.run({ 0xF010, 0x4C00 });
    CHECK(cpu.mmu.crp == 0x7FFF000200012340ULL);

    // PMOVE CRP,(A0)
    cpu.reg.a[0] = 0x2100;
    cpu.run({ 0xF010, 0x4E00 });
    CHECK(cpu.read32(0x2100) == 0x7FFF0002 && cpu.read32(0x2104) == 0x00012340);

    // PMOVE (A0),SRP and PMOVE SRP,(A0)
    cpu.reg.a[0] = 0x2000;
    cpu.write32(0x2000, 0x80010001);
    cpu.write32(0x2004, 0x00004560);
    cpu.run({ 0xF010, 0x4800 });
    CHECK(cpu.mmu.srp == 0x8001000100004560ULL);
    cpu.reg.a[0] = 0x2200;
    cpu.run({ 0xF010, 0x4A00 });
    CHECK(cpu.read32(0x2200) == 0x80010001 && cpu.read32(0x2204) == 0x00004560);

    // PMOVE (A0),TT0 / TT1 and back
    cpu.reg.a[0] = 0x2300;
    cpu.write32(0x2300, 0x1234ABCD);
    cpu.run({ 0xF010, 0x0800 });
    CHECK(cpu.mmu.tt0 == 0x1234ABCD);
    cpu.run({ 0xF010, 0x0C00 });
    CHECK(cpu.mmu.tt1 == 0x1234ABCD);
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
    CHECK(cpu.mmu.mmusr == 0xA5A5);
    cpu.reg.a[0] = 0x2410;
    cpu.run({ 0xF010, 0x6200 });
    CHECK(cpu.read16(0x2410) == 0xA5A5);
}

static void testEnableDisable()
{
    TestCPU cpu;
    cpu.reg.a[0] = 0x2000;

    // PMOVE (A0),TC: Enable the MMU with an identity mapping (early termination)
    cpu.mmu.crp = cpu.mmu.srp = rootPointer(0, 1);
    cpu.write32(0x2000, TC_E | TC_4K_2L);
    cpu.run({ 0xF010, 0x4000 });
    CHECK(cpu.mmu.tc == (TC_E | TC_4K_2L));
    CHECK(cpu.enabled == 1 && cpu.disabled == 0);

    // The MMU is active: Redirect the root pointer
    cpu.mmu.crp = cpu.mmu.srp = rootPointer(0x20000, 1);
    cpu.write32(0x20000 + 0x2000, 0);
    cpu.mmu.crp = cpu.mmu.srp = rootPointer(0, 1);

    // PMOVE (A0),TC: Disable the MMU
    cpu.write32(0x2000, TC_4K_2L);
    cpu.run({ 0xF010, 0x4000 });
    CHECK(cpu.mmu.tc == TC_4K_2L);
    CHECK(cpu.enabled == 1 && cpu.disabled == 1);

    // PMOVE TC,(A0)
    cpu.reg.a[0] = 0x2100;
    cpu.run({ 0xF010, 0x4200 });
    CHECK(cpu.read32(0x2100) == TC_4K_2L);

    // RESET disables the MMU
    cpu.mmu.tc = TC_E | TC_4K_2L;
    cpu.mmu.crp = cpu.mmu.srp = rootPointer(0, 1);
    cpu.run({ 0x4E70 });
    CHECK(!(cpu.mmu.tc & TC_E));
}

static void testTransparentTranslation()
{
    TestCPU cpu;
    cpu.reg.a[0] = 0x2000;

    // TT0: Match addresses 0x20xxxxxx, ignore R/W, function code = 1 (user data)
    cpu.mmu.tt0 = 0x20000000 | 1 << 15 | 1 << 8 | 1 << 4;

    // PTESTR #1,(A0),#0 with a matching address
    cpu.reg.a[0] = 0x20001234;
    cpu.run({ 0xF010, 0x8000 | 1 << 9 | 0x11 });
    CHECK(cpu.mmu.mmusr & 0x40);

    // Non-matching address
    cpu.reg.a[0] = 0x30001234;
    cpu.run({ 0xF010, 0x8000 | 1 << 9 | 0x11 });
    CHECK(!(cpu.mmu.mmusr & 0x40));
}

static void testTranslationBypass()
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
    cpu.mmu.tt0 = 1 << 15 | 1 << 8 | 7;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x11111111);

    // A different address base doesn't match
    cpu.mmu.tt0 = 0x20000000 | 1 << 15 | 1 << 8 | 7;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x22222222);

    // Ignoring all address bits matches everything
    cpu.mmu.tt0 = 0x20000000 | 0xFF << 16 | 1 << 15 | 1 << 8 | 7;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x11111111);

    // TT1 works the same way, and a read-only TT (RWM = 0, RW = 1) ignores writes
    cpu.mmu.tt0 = 0;
    cpu.mmu.tt1 = 1 << 15 | 1 << 9 | 0xFF << 16 | 7;
    cpu.run({ 0x2010 });
    CHECK(cpu.reg.d[0] == 0x11111111);
    cpu.reg.d[1] = 0xDEADBEEF;
    cpu.run({ 0x2081 });                        // MOVE.L D1,(A0) is translated
    CHECK(cpu.read32(0x5004) == 0xDEADBEEF);

    // Function code mismatch
    cpu.mmu.tt1 = 1 << 15 | 1 << 8 | 0xFF << 16 | 1 << 4;   // User data only
    cpu.run({ 0x2010 });                        // Supervisor data access
    CHECK(cpu.reg.d[0] == 0xDEADBEEF);
}

static void testPTest()
{
    TestCPU cpu;
    setupShortTables(cpu);
    cpu.mmu.tc = TC_E | TC_4K_2L;

    // PTESTR #1,(A0),#2,A1 (user data, level 2, return the descriptor address)
    auto ptest = [&](u32 addr, bool read, int level, bool a) {
        cpu.reg.a[0] = addr;
        cpu.reg.a[1] = 0xDEAD0000;
        cpu.run({ 0xF010, u16(0x8000 | level << 10 | read << 9 | a << 8 | a << 5 | 0x11) });
    };

    // Valid page: Two levels, no error bits
    ptest(0x2000, true, 2, true);
    CHECK((cpu.mmu.mmusr & 0xEE07) == 2);
    CHECK(cpu.reg.a[1] == tableB + 4 * 2);

    // Level 1 stops at the table descriptor
    ptest(0x2000, true, 1, true);
    CHECK((cpu.mmu.mmusr & 0xEE07) == 1);
    CHECK(cpu.reg.a[1] == tableA);

    // Invalid page
    ptest(0x3000, true, 7, true);
    CHECK((cpu.mmu.mmusr & 0x0400) == 0x0400);
    CHECK(cpu.reg.a[1] == tableB + 4 * 3);

    // Write-protected page
    ptest(0x4000, false, 7, false);
    CHECK((cpu.mmu.mmusr & 0x0800) == 0x0800);
    CHECK(cpu.reg.a[1] == 0xDEAD0000);              // A bit not set

    // PTEST doesn't modify the tables
    u32 before = cpu.read32(tableB + 4 * 5);
    ptest(0x5000, false, 7, false);
    CHECK(cpu.read32(tableB + 4 * 5) == before);

    // Level 0 without TT: The ATC is empty
    ptest(0x2000, true, 0, false);
    CHECK((cpu.mmu.mmusr & 0x0400) == 0x0400);

    // The A bit is illegal for level 0
    cpu.write32(4 * 4, 0x1900);                     // Illegal instruction vector
    cpu.write32(11 * 4, 0x1900);                    // Line F vector
    ptest(0x2000, true, 0, true);
    CHECK(cpu.reg.pc0 == 0x1900);
}

static void testATC()
{
    TestCPU cpu;
    setupShortTables(cpu);
    cpu.write32(0x5004, 0x11111111);
    cpu.write32(0x6004, 0x22222222);

    auto load = [&](u32 addr) { cpu.reg.a[0] = addr; cpu.run({ 0x2010 }); return cpu.reg.d[0]; };
    auto ptest0 = [&](u32 addr, int fc) {
        cpu.reg.a[0] = addr;
        cpu.run({ 0xF010, u16(0x8000 | 1 << 9 | 0x10 | fc) });
        return cpu.mmu.mmusr;
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

static void testModels()
{
    // Only the 68030 and 68040 have an MMU
    CHECK(!TestCPU(Model::M68000).hasMMU());
    CHECK(!TestCPU(Model::M68020).hasMMU());
    CHECK(TestCPU(Model::M68030).hasMMU());
    CHECK(TestCPU(Model::M68040).hasMMU());

    // NONE removes the MMU (e.g., to emulate a 68EC030). The selection survives a CPU change.
    TestCPU cpu(Model::M68030);
    cpu.setMmuModel(MMUModel::NONE);
    CHECK(!cpu.hasMMU());
    cpu.setModel(Model::M68040);
    CHECK(!cpu.hasMMU());
    cpu.setMmuModel(MMUModel::BUILTIN);
    CHECK(cpu.hasMMU());
    cpu.setModel(Model::M68020);
    CHECK(!cpu.hasMMU());
}

int main()
{
    testModels();
    testDisabled();
    testShortTables();
    testUpdateBits();
    testFaults();
    testEarlyTermination();
    testLongTables();
    testSupervisorRootPointer();
    testPMove();
    testEnableDisable();
    testTransparentTranslation();
    testTranslationBypass();
    testPTest();
    testATC();

    printf("%d checks, %d failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
