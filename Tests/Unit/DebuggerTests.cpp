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

TEST_SUITE_BEGIN("Debugger");

/* A CPU with 1 MB of RAM that records every debugger event. The tests run on
 * the 68000 (the reference core) and on the 68020 (which fetches and accesses
 * memory through different code paths).
 */
class TestCPU : public Moira {

public:

    mutable std::vector<u8> ram = std::vector<u8>(1 << 20);

    struct Event { char kind; u32 value; };
    std::vector<Event> events;
    std::vector<u32> logged;

    static constexpr u32 codeStart = 0x1000;
    static constexpr u32 stackStart = 0x0F00;

    u8 read8(u32 addr) const override { return ram[addr & 0xFFFFF]; }
    u16 read16(u32 addr) const override { return ram[addr & 0xFFFFF] << 8 | ram[(addr + 1) & 0xFFFFF]; }
    u32 read32(u32 addr) const override { return read16(addr) << 16 | read16(addr + 2); }
    void write8(u32 addr, u8 val) const override { ram[addr & 0xFFFFF] = val; }
    void write16(u32 addr, u16 val) const override { ram[addr & 0xFFFFF] = val >> 8; ram[(addr + 1) & 0xFFFFF] = val & 0xFF; }
    void write32(u32 addr, u32 val) const override { write16(addr, val >> 16); write16(addr + 2, val & 0xFFFF); }

    void didReachSoftstop(u32 addr) override { events.push_back({ 's', addr }); }
    void didReachBreakpoint(u32 addr) override { events.push_back({ 'b', addr }); }
    void didReachWatchpoint(u32 addr) override { events.push_back({ 'w', addr }); }
    void didReachCatchpoint(u8 vector) override { events.push_back({ 'c', vector }); }
    void didReachSoftwareTrap(u32 addr) override { events.push_back({ 't', addr }); }
    void didLogInstruction(const Registers &r) override { logged.push_back(r.pc0); }

    void poke16(u32 addr, u16 val) { ram[addr] = val >> 8; ram[addr + 1] = val & 0xFF; }
    void poke32(u32 addr, u32 val) { poke16(addr, val >> 16); poke16(addr + 2, val & 0xFFFF); }

    // Number of events of a certain kind
    int count(char kind) const { int n = 0; for (auto &e : events) n += e.kind == kind; return n; }

    TestCPU(CPUModel model)
    {
        setModel(model);
        poke32(0, stackStart);
        poke32(4, codeStart);
        for (int i = 2; i < 256; i++) poke32(i * 4, 0x4000 + i * 4);
        for (int i = 2; i < 256; i++) poke16(0x4000 + i * 4, 0x4E71);   // NOP handlers
        reset();
        setSR(0x2700);
        setISP(0x1F00);
    }

    // Writes a program into memory (followed by NOPs) and points the PC to it
    void load(std::initializer_list<u16> words)
    {
        u32 addr = codeStart;
        for (auto w : words) { poke16(addr, w); addr += 2; }
        for (int i = 0; i < 512; i++) { poke16(addr, 0x4E71); addr += 2; }
        debugger.jump(codeStart);
        events.clear();
    }

    void step(int n = 1) { for (int i = 0; i < n; i++) execute(); }
};

const CPUModel models[] = { CPUModel::M68000, CPUModel::M68020 };

#define FOR_EACH_MODEL for (auto model : models) { INFO("Model ", int(model)); TestCPU cpu(model);

#define END_MODELS }


//
// Guard lists
//

TEST_CASE("Guards: adding, finding and removing")
{
    FOR_EACH_MODEL
    auto &bp = cpu.debugger.breakpoints;

    CHECK(bp.elements() == 0);
    CHECK(!bp.isSetAt(0x1000));

    bp.setAt(0x1000);
    bp.setAt(0x2000);
    bp.setAt(0x3000);
    bp.setAt(0x2000);                       // Duplicate, ignored

    CHECK(bp.elements() == 3);
    CHECK(bp.isSet(0));
    CHECK(bp.isSet(2));
    CHECK(!bp.isSet(3));
    CHECK(bp.isSetAt(0x2000));
    CHECK(*bp.guardAddr(1) == 0x2000);
    CHECK(!bp.guardAddr(3).has_value());
    CHECK(bp.guardNr(3) == nullptr);

    // Removing keeps the order of the remaining guards
    bp.removeAt(0x1000);
    CHECK(bp.elements() == 2);
    CHECK(*bp.guardAddr(0) == 0x2000);
    CHECK(*bp.guardAddr(1) == 0x3000);

    bp.remove(0);
    CHECK(bp.elements() == 1);
    CHECK(*bp.guardAddr(0) == 0x3000);

    bp.remove(7);                           // Out of range, ignored
    bp.removeAt(0x9999);                    // Not set, ignored
    CHECK(bp.elements() == 1);

    bp.removeAll();
    CHECK(bp.elements() == 0);
    END_MODELS
}

TEST_CASE("Guards: out-of-range indices are harmless")
{
    FOR_EACH_MODEL
    auto &bp = cpu.debugger.breakpoints;
    bp.setAt(0x1000);

    CHECK(bp.guardNr(-1) == nullptr);
    CHECK(!bp.isSet(-1));
    CHECK(!bp.guardAddr(-1).has_value());
    bp.remove(-1);
    bp.enable(-1);
    bp.disable(-1);
    bp.ignore(-1, 3);
    CHECK(bp.elements() == 1);
    END_MODELS
}

TEST_CASE("Guards: the list grows beyond its initial capacity")
{
    FOR_EACH_MODEL
    auto &wp = cpu.debugger.watchpoints;

    for (u32 i = 0; i < 100; i++) wp.setAt(0x10000 + 2 * i);
    CHECK(wp.elements() == 100);
    for (u32 i = 0; i < 100; i++) CHECK(*wp.guardAddr(i) == 0x10000 + 2 * i);

    for (u32 i = 0; i < 100; i += 2) wp.removeAt(0x10000 + 2 * i);
    CHECK(wp.elements() == 50);
    CHECK(*wp.guardAddr(0) == 0x10002);
    END_MODELS
}

TEST_CASE("Guards: enable, disable, toggle and replace")
{
    FOR_EACH_MODEL
    auto &bp = cpu.debugger.breakpoints;
    bp.setAt(0x1000);
    bp.setAt(0x2000);

    CHECK(bp.isEnabled(0));
    bp.disable(0);
    CHECK(bp.isDisabled(0));
    CHECK(!bp.isEnabled(0));
    CHECK(bp.isEnabled(1));

    bp.toggle(0);
    CHECK(bp.isEnabledAt(0x1000));
    bp.toggleAt(0x2000);
    CHECK(bp.isDisabledAt(0x2000));

    bp.disableAll();
    CHECK(bp.isDisabled(0));
    CHECK(bp.isDisabled(1));
    bp.enableAll();
    CHECK(bp.isEnabled(0));
    CHECK(bp.isEnabled(1));

    // Replacing changes the address; it refuses to create a duplicate
    bp.replace(0, 0x3000);
    CHECK(*bp.guardAddr(0) == 0x3000);
    bp.replace(0, 0x2000);
    CHECK(*bp.guardAddr(0) == 0x3000);
    bp.replace(5, 0x4000);
    CHECK(bp.elements() == 2);
    END_MODELS
}

TEST_CASE("Guards: matching rules")
{
    Guard g;
    g.addr = 0x1002;

    // A guard matches if its address lies inside the accessed range
    CHECK(!g.eval(0x1000, Byte));
    CHECK(!g.eval(0x1000, Word));
    CHECK(g.eval(0x1000, Long));
    CHECK(g.eval(0x1002, Byte));
    CHECK(g.eval(0x1001, Word));
    CHECK(!g.eval(0x1003, Word));

    // Disabled guards never match
    g.enabled = false;
    CHECK(!g.eval(0x1002, Byte));

    // The ignore counter swallows the first hits
    g.enabled = true;
    g.ignore = 2;
    CHECK(!g.eval(0x1002, Byte));
    CHECK(!g.eval(0x1002, Byte));
    CHECK(g.eval(0x1002, Byte));
    CHECK(g.eval(0x1002, Byte));
}


//
// Breakpoints
//

TEST_CASE("Breakpoints stop before the instruction is executed")
{
    FOR_EACH_MODEL
    cpu.load({ 0x4E71, 0x4E71, 0x4E71, 0x4E71 });
    cpu.debugger.breakpoints.setAt(0x1004);

    cpu.step();
    CHECK(cpu.count('b') == 0);
    cpu.step();
    REQUIRE(cpu.count('b') == 1);
    CHECK(cpu.events.back().value == 0x1004);
    CHECK(cpu.getPC() == 0x1004);               // Not executed yet

    // The breakpoint stays armed
    cpu.load({ 0x4E71, 0x4E71, 0x4E71 });
    cpu.step(3);
    CHECK(cpu.count('b') == 1);
    END_MODELS
}

TEST_CASE("Breakpoints: disabled, removed and ignored")
{
    FOR_EACH_MODEL
    auto &bp = cpu.debugger.breakpoints;

    cpu.load({ 0x4E71, 0x4E71, 0x4E71 });
    bp.setAt(0x1002);
    bp.disable(0);
    cpu.step(3);
    CHECK(cpu.count('b') == 0);

    cpu.load({ 0x4E71, 0x4E71, 0x4E71 });
    bp.enable(0);
    bp.remove(0);
    cpu.step(3);
    CHECK(cpu.count('b') == 0);

    // Ignore the first hit
    bp.setAt(0x1002, 1);
    cpu.load({ 0x4E71, 0x4E71, 0x4E71 });
    cpu.step(3);
    CHECK(cpu.count('b') == 0);
    cpu.load({ 0x4E71, 0x4E71, 0x4E71 });
    cpu.step(3);
    CHECK(cpu.count('b') == 1);
    END_MODELS
}

TEST_CASE("Breakpoints: removing the last one stops the checking")
{
    FOR_EACH_MODEL
    cpu.load({ 0x4E71, 0x4E71, 0x4E71 });
    cpu.debugger.breakpoints.setAt(0x1002);
    cpu.debugger.breakpoints.removeAll();
    cpu.step(3);
    CHECK(cpu.count('b') == 0);
    END_MODELS
}

TEST_CASE("Breakpoints survive a reset")
{
    FOR_EACH_MODEL
    cpu.debugger.breakpoints.setAt(0x1002);
    cpu.debugger.watchpoints.setAt(0x2000);
    cpu.reset();
    cpu.setSR(0x2700);
    cpu.load({ 0x4E71, 0x4E71, 0x4E71 });
    cpu.step(3);
    CHECK(cpu.count('b') == 1);
    END_MODELS
}

TEST_CASE("Breakpoint on a branch target")
{
    FOR_EACH_MODEL
    // BRA.S +4 ; NOP ; NOP ; NOP (target)
    cpu.load({ 0x6004, 0x4E71, 0x4E71, 0x4E71 });
    cpu.debugger.breakpoints.setAt(0x1006);
    cpu.step(2);
    REQUIRE(cpu.count('b') == 1);
    CHECK(cpu.events.back().value == 0x1006);
    END_MODELS
}


//
// Single-stepping
//

TEST_CASE("Step into")
{
    FOR_EACH_MODEL
    cpu.load({ 0x4E71, 0x4E71, 0x4E71 });
    cpu.debugger.stepInto();

    cpu.step();
    REQUIRE(cpu.count('s') == 1);
    CHECK(cpu.events.back().value == 0x1002);

    // A soft stop is deleted when it is reached
    cpu.step(2);
    CHECK(cpu.count('s') == 1);

    // And can be armed again
    cpu.debugger.stepInto();
    cpu.step();
    CHECK(cpu.count('s') == 2);
    END_MODELS
}

TEST_CASE("Step over a subroutine call")
{
    FOR_EACH_MODEL
    // 1000: BSR.W sub ; 1004: NOP ; ... sub at 1010: NOP ; NOP ; RTS
    cpu.load({ 0x6100, 0x000E, 0x4E71 });
    cpu.poke16(0x1010, 0x4E71);
    cpu.poke16(0x1012, 0x4E71);
    cpu.poke16(0x1014, 0x4E75);

    cpu.debugger.stepOver();
    cpu.step(4);                                // BSR, NOP, NOP, RTS
    REQUIRE(cpu.count('s') == 1);
    CHECK(cpu.events.back().value == 0x1004);   // Stops behind the call, not inside it
    END_MODELS
}

TEST_CASE("Step over is not triggered by the previous soft stop")
{
    FOR_EACH_MODEL
    cpu.load({ 0x4E71, 0x4E71, 0x4E71, 0x4E71 });
    cpu.debugger.stepOver();
    cpu.step();
    CHECK(cpu.count('s') == 1);
    cpu.step(3);
    CHECK(cpu.count('s') == 1);
    END_MODELS
}

TEST_CASE("Soft stop and breakpoint on the same address both report")
{
    FOR_EACH_MODEL
    cpu.load({ 0x4E71, 0x4E71, 0x4E71 });
    cpu.debugger.breakpoints.setAt(0x1002);
    cpu.debugger.stepInto();
    cpu.step();
    CHECK(cpu.count('s') == 1);
    CHECK(cpu.count('b') == 1);
    END_MODELS
}


//
// Watchpoints
//

TEST_CASE("Watchpoints observe data writes")
{
    FOR_EACH_MODEL
    // MOVE.W D0,(A0) ; MOVE.L D0,(A1) ; MOVE.B D0,(A2)
    cpu.load({ 0x3080, 0x2280, 0x1480 });
    cpu.setA(0, 0x2000);
    cpu.setA(1, 0x3000);
    cpu.setA(2, 0x4001);
    cpu.debugger.watchpoints.setAt(0x2000);
    cpu.debugger.watchpoints.setAt(0x3002);     // Inside a long word write
    cpu.debugger.watchpoints.setAt(0x4001);

    cpu.step();
    CHECK(cpu.count('w') == 1);
    cpu.step();
    CHECK(cpu.count('w') >= 2);
    cpu.step();
    CHECK(cpu.count('w') >= 3);
    CHECK(cpu.events.back().value == 0x4001);
    END_MODELS
}

TEST_CASE("Watchpoints observe data reads")
{
    FOR_EACH_MODEL
    // MOVE.W (A0),D0 ; MOVE.L (A1),D1 ; MOVE.B (A2),D2
    cpu.load({ 0x3010, 0x2211, 0x1412 });
    cpu.setA(0, 0x2000);
    cpu.setA(1, 0x3000);
    cpu.setA(2, 0x4001);
    cpu.debugger.watchpoints.setAt(0x2000);
    cpu.debugger.watchpoints.setAt(0x3003);
    cpu.debugger.watchpoints.setAt(0x4001);

    cpu.step(3);
    CHECK(cpu.count('w') >= 3);
    END_MODELS
}

TEST_CASE("Watchpoints do not fire for other addresses")
{
    FOR_EACH_MODEL
    cpu.load({ 0x3080, 0x2280 });               // MOVE.W D0,(A0) ; MOVE.L D0,(A1)
    cpu.setA(0, 0x2000);
    cpu.setA(1, 0x3000);
    cpu.debugger.watchpoints.setAt(0x2002);     // Behind the word
    cpu.debugger.watchpoints.setAt(0x2FFF);     // In front of the long word
    cpu.debugger.watchpoints.setAt(0x3004);     // Behind the long word
    cpu.step(2);
    CHECK(cpu.count('w') == 0);
    END_MODELS
}

TEST_CASE("Watchpoints: disabled and ignored")
{
    FOR_EACH_MODEL
    cpu.setA(0, 0x2000);
    cpu.debugger.watchpoints.setAt(0x2000);
    cpu.debugger.watchpoints.disable(0);
    cpu.load({ 0x3080, 0x3080 });
    cpu.step(2);
    CHECK(cpu.count('w') == 0);

    cpu.debugger.watchpoints.enable(0);
    cpu.debugger.watchpoints.ignore(0, 1);
    cpu.load({ 0x3080, 0x3080 });
    cpu.step(2);
    CHECK(cpu.count('w') == 1);
    END_MODELS
}

TEST_CASE("Watchpoints observe stack accesses")
{
    FOR_EACH_MODEL
    // MOVE.W D0,-(A7)
    cpu.load({ 0x3F00 });
    cpu.setSP(0x2000);
    cpu.debugger.watchpoints.setAt(0x1FFE);
    cpu.step();
    CHECK(cpu.count('w') == 1);
    END_MODELS
}


//
// Catchpoints
//

TEST_CASE("Catchpoints stop at exceptions")
{
    FOR_EACH_MODEL
    auto &cp = cpu.debugger.catchpoints;
    cp.setAt(32);                               // TRAP #0
    cp.setAt(4);                                // Illegal instruction
    cp.setAt(5);                                // Division by zero
    cp.setAt(10);                               // Line A

    // TRAP #0 ; ILLEGAL ; DIVU #0,D0 ; Line A
    cpu.load({ 0x4E40, 0x4AFC, 0x80FC, 0x0000, 0xA123 });
    cpu.step(2);
    CHECK(cpu.count('c') == 1);
    CHECK(cpu.events.back().value == 32);
    END_MODELS
}

TEST_CASE("Catchpoints: each vector reports exactly once per exception")
{
    FOR_EACH_MODEL
    auto &cp = cpu.debugger.catchpoints;
    cp.setAt(4);
    cp.setAt(5);
    cp.setAt(10);

    cpu.load({ 0x4AFC });                       // ILLEGAL
    cpu.step();
    CHECK(cpu.count('c') == 1);
    CHECK(cpu.events.back().value == 4);

    cpu.setD(0, 1);
    cpu.load({ 0x80FC, 0x0000 });               // DIVU #0,D0
    cpu.step();
    CHECK(cpu.count('c') == 1);
    CHECK(cpu.events.back().value == 5);

    cpu.load({ 0xA123 });                       // Line A
    cpu.step();
    CHECK(cpu.count('c') == 1);
    CHECK(cpu.events.back().value == 10);
    END_MODELS
}

TEST_CASE("Catchpoints: disabled, removed and ignored")
{
    FOR_EACH_MODEL
    auto &cp = cpu.debugger.catchpoints;
    cp.setAt(4);
    cp.disable(0);
    cpu.load({ 0x4AFC });
    cpu.step();
    CHECK(cpu.count('c') == 0);

    cp.enable(0);
    cp.ignore(0, 1);
    cpu.load({ 0x4AFC });
    cpu.step();
    CHECK(cpu.count('c') == 0);
    cpu.load({ 0x4AFC });
    cpu.step();
    CHECK(cpu.count('c') == 1);

    cp.removeAll();
    cpu.load({ 0x4AFC });
    cpu.step();
    CHECK(cpu.count('c') == 0);
    END_MODELS
}

TEST_CASE("Catchpoints stop at interrupts")
{
    FOR_EACH_MODEL
    cpu.debugger.catchpoints.setAt(26);         // Level 2 interrupt
    cpu.load({ 0x4E71, 0x4E71, 0x4E71, 0x4E71 });
    cpu.setSR(0x2000);
    cpu.setIPL(2);
    cpu.step(4);
    CHECK(cpu.count('c') == 1);
    CHECK(cpu.events.back().value == 26);
    END_MODELS
}

TEST_CASE("Catchpoints stop at address errors")
{
    TestCPU cpu(CPUModel::M68000);
    cpu.debugger.catchpoints.setAt(3);
    cpu.setA(0, 0x2001);
    cpu.load({ 0x3010 });                       // MOVE.W (A0),D0
    cpu.step();
    CHECK(cpu.count('c') == 1);
    CHECK(cpu.events.back().value == 3);
}


//
// Software traps
//

TEST_CASE("Software traps: creation and resolution")
{
    SoftwareTraps traps;

    auto a = traps.create(0x5240);
    auto b = traps.create(0x4E71);
    CHECK(Debugger::isLineAInstr(a));
    CHECK(Debugger::isLineAInstr(b));
    CHECK(a != b);
    CHECK(traps.resolve(a) == 0x5240);
    CHECK(traps.resolve(b) == 0x4E71);
    CHECK(traps.resolve(0x1234) == 0x1234);     // Not a trap

    auto c = traps.create(0xA0F0, 0x7001);
    CHECK(c == 0xA0F0);
    CHECK(traps.resolve(0xA0F0) == 0x7001);
}

TEST_CASE("Software traps replace an instruction")
{
    FOR_EACH_MODEL
    auto key = cpu.debugger.swTraps.create(0x5240);     // ADDQ.W #1,D0

    cpu.setD(0, 0);
    cpu.load({ key, 0x4E71 });

    cpu.step();
    REQUIRE(cpu.count('t') == 1);
    CHECK(cpu.events.back().value == 0x1000);
    CHECK(cpu.getD(0) == 0);                    // Not executed yet
    CHECK(cpu.getPC() == 0x1000);

    cpu.step();                                 // Now the original instruction runs
    CHECK(cpu.getD(0) == 1);
    CHECK(cpu.getPC() == 0x1002);
    CHECK(cpu.count('t') == 1);
    END_MODELS
}

TEST_CASE("Line A instructions without a trap raise an exception")
{
    FOR_EACH_MODEL
    cpu.debugger.swTraps.create(0xA000, 0x4E71);
    cpu.load({ 0xA123 });
    cpu.step();
    CHECK(cpu.count('t') == 0);
    CHECK(cpu.getPC() == 0x4000 + 10 * 4);      // Line A handler
    END_MODELS
}


//
// Instruction log
//

TEST_CASE("Instruction logging")
{
    FOR_EACH_MODEL
    cpu.load({ 0x4E71, 0x4E71, 0x4E71, 0x4E71 });

    // Nothing is recorded while logging is off
    cpu.step();
    CHECK(cpu.debugger.loggedInstructions() == 0);
    CHECK(cpu.logged.empty());

    cpu.debugger.enableLogging();
    cpu.step(3);
    REQUIRE(cpu.debugger.loggedInstructions() == 3);
    CHECK(cpu.logged == std::vector<u32>{ 0x1002, 0x1004, 0x1006 });

    // Relative: 0 is the newest entry. Absolute: 0 is the oldest.
    CHECK(cpu.debugger.logEntryRel(0).pc0 == 0x1006);
    CHECK(cpu.debugger.logEntryRel(2).pc0 == 0x1002);
    CHECK(cpu.debugger.logEntryAbs(0).pc0 == 0x1002);
    CHECK(cpu.debugger.logEntryAbs(2).pc0 == 0x1006);

    cpu.debugger.disableLogging();
    cpu.step();
    CHECK(cpu.debugger.loggedInstructions() == 3);

    cpu.debugger.clearLog();
    CHECK(cpu.debugger.loggedInstructions() == 0);
    END_MODELS
}

TEST_CASE("The log buffer wraps around")
{
    FOR_EACH_MODEL
    cpu.load({});
    cpu.debugger.enableLogging();
    cpu.step(300);

    CHECK(cpu.debugger.loggedInstructions() == 256);
    CHECK(cpu.logged.size() == 300);
    CHECK(cpu.debugger.logEntryRel(0).pc0 == 0x1000 + 2 * 299);
    CHECK(cpu.debugger.logEntryRel(255).pc0 == 0x1000 + 2 * 44);
    CHECK(cpu.debugger.logEntryAbs(0).pc0 == 0x1000 + 2 * 44);
    CHECK(cpu.debugger.logEntryAbs(255).pc0 == 0x1000 + 2 * 299);
    END_MODELS
}


//
// Miscellaneous
//

TEST_CASE("Every vector has a name")
{
    for (int i = 0; i < 256; i++) {
        INFO("Vector ", i);
        CHECK(!Debugger::vectorName(u8(i)).empty());
    }
    CHECK(Debugger::vectorName(2) == "Bus error");
    CHECK(Debugger::vectorName(26) == "Level 2 interrupt");
    CHECK(Debugger::vectorName(32) == "Trap #0");
    CHECK(Debugger::vectorName(255) == "User interrupt vector");
}

TEST_CASE("jump() continues at an arbitrary address")
{
    FOR_EACH_MODEL
    cpu.load({ 0x4E71 });
    cpu.poke16(0x2000, 0x7005);                 // MOVEQ #5,D0
    cpu.debugger.jump(0x2000);
    CHECK(cpu.getPC() == 0x2000);
    cpu.step();
    CHECK(cpu.getD(0) == 5);
    CHECK(cpu.getPC() == 0x2002);

    // Odd addresses are rounded down
    cpu.poke16(0x3000, 0x7007);
    cpu.debugger.jump(0x3001);
    CHECK(cpu.getPC() == 0x3000);
    cpu.step();
    CHECK(cpu.getD(0) == 7);
    END_MODELS
}

TEST_SUITE_END();

}
