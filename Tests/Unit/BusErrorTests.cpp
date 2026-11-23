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

    // If set, the host supplies the complete stack frame itself
    bool customFrame = false;
    u16 customSR = 0;

    void check(u32 addr, bool write) const
    {
        if ((addr & ~1u) != faultAddr) return;
        if (!(write ? faultOnWrite : faultOnRead)) return;
        if (customFrame) {
            StackFrame frame = {};
            frame.addr = addr;
            frame.sr = customSR;
            throw BusError(frame);
        }
        throw BusError(addr);
    }

    u16 peek16(u32 addr) const { return ram[addr] << 8 | ram[addr + 1]; }
    void poke16(u32 addr, u16 val) { ram[addr] = val >> 8; ram[addr + 1] = val & 0xFF; }
    void poke32(u32 addr, u32 val) { poke16(addr, val >> 16); poke16(addr + 2, val & 0xFFFF); }

    TestCPU(CPUModel model = CPUModel::M68000)
    {
        setModel(model);
        poke32(0, stackStart);
        poke32(4, codeStart);
        poke32(8, handler);       // Bus error vector
        poke16(handler, 0x4E71);
        poke32(12, aeHandler);    // Address error vector
        poke16(aeHandler, 0x4E71);
        poke32(0x24, traceHandler); // Trace vector
        poke16(traceHandler, 0x4E71);
        reset();
    }

    static constexpr u32 aeHandler = 0x4100;
    static constexpr u32 traceHandler = 0x4200;

    // Indicates whether the CPU is in loop mode (68010)
    bool looping() const { return flags & State::LOOPING; }

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

TEST_CASE("A failed prefetch shows the flags that the instruction has set")
{
    TestCPU cpu;
    cpu.faultAddr = 0x1004;
    cpu.setD(0, 0x00008000);
    cpu.setD(1, 1);

    // LSL.W D1,D0 (MAME shows the flags of the unshifted operand while the prefetch is pending)
    auto f = cpu.run({ 0xE368 });

    CHECK((f.sr & 0x1F) == 0x08);                   // N set (operand is negative), Z, V, C clear
    CHECK((cpu.getSR() & 0x1F) == 0x08);
    CHECK(cpu.getD(0) == 0x00008000);               // The instruction has not been completed
}

TEST_CASE("Moira does not modify a stack frame supplied by the host")
{
    TestCPU cpu;
    cpu.faultAddr = 0x1004;
    cpu.customFrame = true;
    cpu.customSR = 0x0015;
    cpu.setD(0, 0x00008000);
    cpu.setD(1, 1);

    // The same failing prefetch as above, but the host provides the frame
    auto f = cpu.run({ 0xE368 });

    CHECK(f.sr == 0x0015);
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

/* The 68010 has a loop mode that speeds up tight DBcc loops. Faults that occur
 * in this mode need to be processed like faults of normal instructions.
 */

// MOVE.W (A0)+,(A1)+ followed by DBRA: A one-instruction loop of 41 iterations
static void loadLoop(TestCPU &cpu)
{
    const u16 prog[] = { 0x41F8, 0x3000,            // LEA $3000.w,A0
                         0x43F8, 0x5000,            // LEA $5000.w,A1
                         0x7028,                    // MOVEQ #40,D0
                         0x32D8,                    // loop: MOVE.W (A0)+,(A1)+
                         0x51C8, 0xFFFC,            //       DBRA D0,loop
                         0x4E71, 0x4E71, 0x4E71 };

    for (size_t i = 0; i < sizeof(prog) / 2; i++) cpu.poke16(TestCPU::codeStart + 2 * u32(i), prog[i]);
    cpu.debugger.jump(TestCPU::codeStart);
    cpu.setSR(0x2700);
    cpu.setISP(0x1F00);
}

TEST_CASE("Bus errors in loop mode are processed like other bus errors")
{
    for (u32 fault : { 0x3000u, 0x3002u, 0x3004u, 0x3010u, 0x3050u }) {

        INFO("Fault address ", fault);

        TestCPU cpu(CPUModel::M68010);
        cpu.faultAddr = fault;
        loadLoop(cpu);

        bool reached = false, loopBefore = false;

        for (int i = 0; i < 200 && !reached; i++) {

            loopBefore = cpu.looping();
            REQUIRE_NOTHROW(cpu.execute());                     // No exception may escape
            reached = cpu.getPC0() == TestCPU::handler;
        }

        REQUIRE(reached);
        CHECK(loopBefore == (fault >= 0x3002));                 // The first iteration runs normally
        CHECK_FALSE(cpu.looping());                             // The fault terminates loop mode

        // The CPU continues with the handler
        REQUIRE_NOTHROW(cpu.execute());
        CHECK(cpu.getPC0() == TestCPU::handler + 2);
    }
}

TEST_CASE("Address errors in loop mode are processed like other address errors")
{
    TestCPU cpu(CPUModel::M68010);
    loadLoop(cpu);

    // Run until the CPU has entered loop mode
    for (int i = 0; i < 20 && !cpu.looping(); i++) cpu.execute();
    REQUIRE(cpu.looping());

    // Make the next read access misaligned
    cpu.setA(0, 0x3001);

    bool reached = false;
    for (int i = 0; i < 5 && !reached; i++) {

        REQUIRE_NOTHROW(cpu.execute());
        reached = cpu.getPC0() == TestCPU::aeHandler;
    }

    REQUIRE(reached);
    CHECK_FALSE(cpu.looping());
    REQUIRE_NOTHROW(cpu.execute());
    CHECK(cpu.getPC0() == TestCPU::aeHandler + 2);
}

TEST_CASE("Trace exceptions terminate loop mode")
{
    TestCPU cpu(CPUModel::M68010);
    loadLoop(cpu);

    // Run until the CPU has entered loop mode
    for (int i = 0; i < 20 && !cpu.looping(); i++) cpu.execute();
    REQUIRE(cpu.looping());

    // Set the trace bit
    cpu.setSR(0xA700);

    bool reached = false;
    for (int i = 0; i < 5 && !reached; i++) {

        REQUIRE_NOTHROW(cpu.execute());
        reached = cpu.getPC0() == TestCPU::traceHandler;
    }

    REQUIRE(reached);
    CHECK_FALSE(cpu.looping());

    // The handler runs without trace and executes normally
    cpu.setSR(0x2700);
    REQUIRE_NOTHROW(cpu.execute());
    CHECK(cpu.getPC0() == TestCPU::traceHandler + 2);
}

/* The 68030 and 68040 report the cause of a fault in the special status word.
 * Moira builds it if the host only provides the fault address.
 */

static u32 peek32(TestCPU &cpu, u32 addr) { return u32(cpu.peek16(addr)) << 16 | cpu.peek16(addr + 2); }

TEST_CASE("68030 bus error frames of host-signaled faults")
{
    SUBCASE("Data read") {

        TestCPU cpu(CPUModel::M68030);
        cpu.faultAddr = 0x2000;
        cpu.setA(0, 0x2000);
        cpu.run({ 0x3010 });                            // MOVE.W (A0),D0

        u32 sp = cpu.getSP();
        CHECK((cpu.peek16(sp + 6) >> 12) == 0xB);          // Long bus fault frame
        CHECK(cpu.peek16(sp + 10) == (SSW030::DF | SSW030::RW | SSW030::SIZ_W | 1));
        CHECK(peek32(cpu, sp + 16) == 0x2000);
    }

    SUBCASE("Long word read") {

        TestCPU cpu(CPUModel::M68030);
        cpu.faultAddr = 0x2000;
        cpu.setA(0, 0x2000);
        cpu.run({ 0x2010 });                            // MOVE.L (A0),D0

        u32 sp = cpu.getSP();
        CHECK(cpu.peek16(sp + 10) == (SSW030::DF | SSW030::RW | SSW030::SIZ_L | 1));
    }

    SUBCASE("Data write") {

        TestCPU cpu(CPUModel::M68030);
        cpu.faultAddr = 0x2000;
        cpu.faultOnRead = false;
        cpu.setA(0, 0x2000);
        cpu.setD(0, 0x1234);
        cpu.run({ 0x3080 });                            // MOVE.W D0,(A0)

        u32 sp = cpu.getSP();
        CHECK((cpu.peek16(sp + 6) >> 12) == 0xA);          // Short bus fault frame
        CHECK(cpu.peek16(sp + 10) == (SSW030::DF | SSW030::SIZ_W | 1));
        CHECK(peek32(cpu, sp + 16) == 0x2000);
        CHECK(peek32(cpu, sp + 24) == 0x1234);          // Data output buffer
    }

    SUBCASE("Instruction fetch") {

        TestCPU cpu(CPUModel::M68030);
        cpu.faultAddr = 0x2000;
        cpu.setA(0, 0x2000);
        cpu.run({ 0x4ED0 });                            // JMP (A0), the fetch at the target fails

        u32 sp = cpu.getSP();
        CHECK((cpu.peek16(sp + 6) >> 12) == 0xB);
        CHECK(cpu.peek16(sp + 10) == (SSW030::FB | SSW030::RB | SSW030::RW | SSW030::SIZ_W | 2));
    }
}

TEST_CASE("68040 access error frames of host-signaled faults")
{
    SUBCASE("Data read") {

        TestCPU cpu(CPUModel::M68040);
        cpu.faultAddr = 0x2000;
        cpu.setA(0, 0x2000);
        cpu.run({ 0x3010 });                            // MOVE.W (A0),D0

        u32 sp = cpu.getSP();
        CHECK((cpu.peek16(sp + 6) >> 12) == 0x7);          // Access error frame
        CHECK(cpu.peek16(sp + 12) == (SSW040::RW | SSW040::SIZ_W | 1));
        CHECK(peek32(cpu, sp + 20) == 0x2000);
    }

    SUBCASE("Data write") {

        TestCPU cpu(CPUModel::M68040);
        cpu.faultAddr = 0x2000;
        cpu.faultOnRead = false;
        cpu.setA(0, 0x2000);
        cpu.setD(0, 0x1234);
        cpu.run({ 0x3080 });                            // MOVE.W D0,(A0)

        u32 sp = cpu.getSP();
        CHECK(cpu.peek16(sp + 12) == (SSW040::SIZ_W | 1));
        CHECK(peek32(cpu, sp + 20) == 0x2000);
        CHECK(peek32(cpu, sp + 28) == 0x1234);          // Write-back 3 data
    }
}

TEST_CASE("A frame supplied by the host keeps its special status word (68030)")
{
    TestCPU cpu(CPUModel::M68030);
    cpu.faultAddr = 0x2000;
    cpu.customFrame = true;
    cpu.setA(0, 0x2000);
    cpu.run({ 0x3010 });                                // MOVE.W (A0),D0

    // The host frame carries no special status word
    CHECK(cpu.peek16(cpu.getSP() + 10) == 0);
}

TEST_SUITE_END();

}
