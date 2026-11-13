// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* Interrupt acknowledgment and function code pins
 *
 * These areas are invisible to the comparison against UAE, MAME and Musashi
 * (none of them exposes the FC pins, and the interrupt modes other than
 * autovectoring are never driven), and they are not testable on an Amiga.
 * The tests below therefore check Moira against the documented behavior of
 * the 68000, 68010 and 68020.
 */

#include "doctest.h"
#include "Moira.h"
#include <vector>

using namespace moira;

namespace {

TEST_SUITE_BEGIN("IrqFc");

class IrqCPU : public Moira {

public:

    mutable std::vector<u8> ram = std::vector<u8>(1 << 20);

    struct Access { bool write; u32 addr; int fc; };
    mutable std::vector<Access> log;
    bool logging = false;

    // Interrupt bookkeeping
    std::vector<u8> irqLevels;              // Levels reported by willInterrupt
    mutable std::vector<u8> userVecCalls;   // Levels passed to readIrqUserVector
    u16 userVec[8] = { 0, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47 };

    static constexpr u32 codeStart = 0x1000;
    static constexpr u32 handler(int vec) { return 0x4000 + vec * 4; }

    u8 read8(u32 a) const override { note(false, a); return ram[a & 0xFFFFF]; }
    u16 read16(u32 a) const override { note(false, a); return ram[a & 0xFFFFF] << 8 | ram[(a + 1) & 0xFFFFF]; }
    u32 read32(u32 a) const override { return read16(a) << 16 | read16(a + 2); }
    void write8(u32 a, u8 v) const override { note(true, a); ram[a & 0xFFFFF] = v; }
    void write16(u32 a, u16 v) const override { note(true, a); ram[a & 0xFFFFF] = v >> 8; ram[(a + 1) & 0xFFFFF] = v & 0xFF; }
    void write32(u32 a, u32 v) const override { write16(a, v >> 16); write16(a + 2, v & 0xFFFF); }

    u16 readIrqUserVector(u8 level) const override { userVecCalls.push_back(level); userVecFC.push_back(readFC()); return userVec[level]; }
    void willInterrupt(u8 level) override { irqLevels.push_back(level); }

    struct SpaceCycle { CpuSpaceType type; u8 arg; int fc; };
    std::vector<SpaceCycle> spaceCycles;
    mutable std::vector<int> userVecFC;     // FC seen inside readIrqUserVector

    void cpuSpaceCycle(CpuSpaceType type, u8 arg) override { spaceCycles.push_back({ type, arg, readFC() }); }

    void note(bool w, u32 a) const { if (logging) log.push_back({ w, a & 0xFFFFFF, readFC() }); }

    void poke16(u32 a, u16 v) { ram[a] = v >> 8; ram[a + 1] = v & 0xFF; }
    void poke32(u32 a, u32 v) { poke16(a, v >> 16); poke16(a + 2, v & 0xFFFF); }
    u16 peek16(u32 a) const { return ram[a] << 8 | ram[a + 1]; }
    u32 peek32(u32 a) const { return peek16(a) << 16 | peek16(a + 2); }

    CPUModel model0;

    IrqCPU(CPUModel model) : model0(model)
    {
        setModel(model);
        poke32(0, 0xF00);
        poke32(4, codeStart);
        for (int i = 2; i < 256; i++) { poke32(i * 4, handler(i)); poke16(handler(i), 0x4E71); }
        reset();
        setSR(0x2700);
        setISP(0x1F00);
        setUSP(0x1E00);
    }

    void setA(int n, u32 v) { reg.a[n] = v; }

    void load(std::initializer_list<u16> words) { load(std::vector<u16>(words)); }

    void load(const std::vector<u16> &words)
    {
        u32 a = codeStart;
        for (auto w : words) { poke16(a, w); a += 2; }
        for (int i = 0; i < 256; i++) { poke16(a, 0x4E71); a += 2; }   // NOPs
        debugger.jump(codeStart);
        irqLevels.clear();
        userVecCalls.clear();
        userVecFC.clear();
        spaceCycles.clear();
    }

    void step(int n = 1) { for (int i = 0; i < n; i++) execute(); }

    // Size of the interrupt stack frame
    bool shortFrame() const { return model0 == CPUModel::M68000; }
    u32 frameSize() const { return shortFrame() ? 6 : 8; }

    // Checks the frame of a taken interrupt. The frame sits at the ISP.
    void checkFrame(u16 srBefore, u32 pc, int vec)
    {
        u32 sp = getSP();
        CHECK(sp == 0x1F00 - frameSize());
        CHECK(peek16(sp) == srBefore);
        CHECK(peek32(sp + 2) == pc);
        if (!shortFrame()) CHECK(peek16(sp + 6) == u16(vec * 4));    // Format 0, vector offset
    }
};

namespace FCv { enum { UD = 1, UP = 2, SD = 5, SP = 6, CPU = 7 }; }

const CPUModel models[] = { CPUModel::M68000, CPUModel::M68010, CPUModel::M68020 };

#define FOR_EACH_MODEL for (auto model : models) { INFO("Model ", int(model)); IrqCPU cpu(model);
#define END_MODELS }


//
// Interrupt acknowledgment
//

TEST_CASE("Autovector interrupts of all levels")
{
    for (auto model : models) {
        for (int level = 1; level <= 7; level++) {

            INFO("Model ", int(model), ", level ", level);
            IrqCPU cpu(model);
            cpu.load({});
            cpu.setSR(0x2000);

            cpu.setIPL(level);
            cpu.step(2);                // The first instruction finishes, then the interrupt is taken

            int vec = 24 + level;
            REQUIRE(cpu.irqLevels.size() == 1);
            CHECK(cpu.irqLevels[0] == level);
            CHECK(cpu.getPC0() == IrqCPU::handler(vec));
            CHECK(((cpu.getSR() >> 8) & 7) == level);               // Mask is raised to the level
            CHECK((cpu.getSR() & 0x2000) != 0);                     // Supervisor mode
            CHECK(cpu.userVecCalls.empty());                        // Autovectors need no device
            CHECK(cpu.getIrqMode() == IrqMode::AUTO);
            cpu.checkFrame(0x2000, IrqCPU::codeStart + 2, vec);
        }
    }
}

TEST_CASE("The interrupt mask blocks levels up to the mask")
{
    for (auto model : models) {
        for (int mask = 0; mask <= 7; mask++) {
            for (int level = 1; level <= 7; level++) {

                INFO("Model ", int(model), ", mask ", mask, ", level ", level);
                IrqCPU cpu(model);
                cpu.load({});
                cpu.setSR(0x2000 | mask << 8);
                cpu.setIPL(level);
                cpu.step(3);

                // Level 7 is non-maskable
                bool taken = level > mask || level == 7;
                CHECK(cpu.irqLevels.size() == (taken ? 1u : 0u));
            }
        }
    }
}

TEST_CASE("Interrupt from user mode switches to the interrupt stack")
{
    FOR_EACH_MODEL
    cpu.load({});
    cpu.setSR(0x0000);
    cpu.setIPL(4);
    cpu.step(2);

    CHECK(cpu.getPC0() == IrqCPU::handler(28));
    CHECK((cpu.getSR() & 0x2000) != 0);
    CHECK(cpu.getUSP() == 0x1E00);                                  // USP untouched
    CHECK(cpu.getISP() == 0x1F00 - cpu.frameSize());
    cpu.checkFrame(0x0000, IrqCPU::codeStart + 2, 28);              // Stacked SR shows user mode
    END_MODELS
}

TEST_CASE("Interrupts clear the trace bit")
{
    FOR_EACH_MODEL
    cpu.load({});
    cpu.setSR(0x2000);
    cpu.setIPL(2);
    cpu.step(2);
    CHECK((cpu.getSR() & 0xC000) == 0);
    END_MODELS
}

TEST_CASE("Level 7 is edge triggered")
{
    FOR_EACH_MODEL
    cpu.load({});
    cpu.setSR(0x2700);

    cpu.setIPL(7);
    cpu.step(2);
    CHECK(cpu.irqLevels.size() == 1);
    CHECK(cpu.getPC0() == IrqCPU::handler(31));

    // The pins stay at 7: the handler runs, no second interrupt
    cpu.step(20);
    CHECK(cpu.irqLevels.size() == 1);

    // Leaving level 7 and coming back triggers the next one
    cpu.setIPL(0);
    cpu.step(3);
    cpu.setIPL(7);
    cpu.step(3);
    CHECK(cpu.irqLevels.size() == 2);
    END_MODELS
}

TEST_CASE("User vectors: the device supplies the vector number")
{
    FOR_EACH_MODEL
    cpu.setIrqMode(IrqMode::USER);
    cpu.userVec[5] = 0x1A5;                 // Only the low byte counts
    cpu.poke32(0xA5 * 4, 0x6000);
    cpu.poke16(0x6000, 0x4E71);

    cpu.load({});
    cpu.setSR(0x2000);
    cpu.setIPL(5);
    cpu.step(2);

    REQUIRE(cpu.userVecCalls.size() == 1);
    CHECK(cpu.userVecCalls[0] == 5);
    CHECK(cpu.getPC0() == 0x6000);
    cpu.checkFrame(0x2000, IrqCPU::codeStart + 2, 0xA5);
    END_MODELS
}

TEST_CASE("User vectors: the device is asked for every level")
{
    FOR_EACH_MODEL
    cpu.setIrqMode(IrqMode::USER);
    for (int level = 1; level <= 7; level++) {

        INFO("level ", level);
        cpu.userVec[level] = 0x40 + level;
        cpu.load({});
        cpu.setSR(0x2000);
        cpu.setIPL(0);
        cpu.step();
        cpu.setIPL(level);
        cpu.step(2);

        REQUIRE(cpu.userVecCalls.size() == 1);
        CHECK(cpu.userVecCalls[0] == level);
        CHECK(cpu.getPC0() == IrqCPU::handler(0x40 + level));
        cpu.setIPL(0);
        cpu.setSR(0x2000);
        cpu.setISP(0x1F00);
    }
    END_MODELS
}

TEST_CASE("Spurious interrupts use vector 24")
{
    FOR_EACH_MODEL
    cpu.setIrqMode(IrqMode::SPURIOUS);
    cpu.load({});
    cpu.setSR(0x2000);
    cpu.setIPL(5);
    cpu.step(2);

    CHECK(cpu.getPC0() == IrqCPU::handler(24));
    CHECK(cpu.userVecCalls.empty());
    CHECK(((cpu.getSR() >> 8) & 7) == 5);                           // The level still raises the mask
    cpu.checkFrame(0x2000, IrqCPU::codeStart + 2, 24);
    END_MODELS
}

TEST_CASE("Uninitialized interrupts use vector 15")
{
    FOR_EACH_MODEL
    cpu.setIrqMode(IrqMode::UNINITIALIZED);
    cpu.load({});
    cpu.setSR(0x2000);
    cpu.setIPL(3);
    cpu.step(2);

    CHECK(cpu.getPC0() == IrqCPU::handler(15));
    CHECK(cpu.userVecCalls.empty());
    cpu.checkFrame(0x2000, IrqCPU::codeStart + 2, 15);
    END_MODELS
}

TEST_CASE("A pending interrupt is taken when the mask drops")
{
    FOR_EACH_MODEL
    // move #$2000,sr
    cpu.load({ 0x46FC, 0x2000 });
    cpu.setSR(0x2700);
    cpu.setIPL(3);
    cpu.step();                             // Executes the MOVE
    CHECK(cpu.irqLevels.empty());
    cpu.step();                             // Takes the interrupt before the next instruction
    REQUIRE(cpu.irqLevels.size() == 1);
    CHECK(cpu.getPC0() == IrqCPU::handler(27));
    cpu.checkFrame(0x2000, IrqCPU::codeStart + 4, 27);              // Points behind the MOVE
    END_MODELS
}


//
// CPU space cycles
//

TEST_CASE("CPU space: interrupt acknowledge is reported with FC = 7")
{
    for (auto model : models) {
        for (auto mode : { IrqMode::AUTO, IrqMode::USER, IrqMode::SPURIOUS, IrqMode::UNINITIALIZED }) {
            for (int level = 1; level <= 7; level++) {

                INFO("Model ", int(model), ", mode ", int(mode), ", level ", level);
                IrqCPU cpu(model);
                cpu.setIrqMode(mode);
                cpu.load({});
                cpu.setSR(0x2000);

                cpu.step();
                CHECK(cpu.spaceCycles.empty());             // No cycle without an interrupt
                cpu.setIPL(level);
                cpu.step(2);                                // The interrupt is taken after the first instruction

                REQUIRE(cpu.spaceCycles.size() == 1);
                CHECK(cpu.spaceCycles[0].type == CpuSpaceType::IRQ_ACK);
                CHECK(cpu.spaceCycles[0].arg == level);
                CHECK(cpu.spaceCycles[0].fc == FCv::CPU);

                // The device is asked during the same cycle
                if (mode == IrqMode::USER) {
                    REQUIRE(cpu.userVecFC.size() == 1);
                    CHECK(cpu.userVecFC[0] == FCv::CPU);
                }
            }
        }
    }
}

TEST_CASE("CPU space: the acknowledge cycle does not leak into later accesses")
{
    FOR_EACH_MODEL
    cpu.load({ 0x3039, 0x0000, 0x2000 });                           // move.w $2000,d0
    cpu.setSR(0x2000);
    cpu.setIPL(4);
    cpu.logging = true;
    cpu.step(2);                                                    // Interrupt, then the first handler instruction
    cpu.logging = false;
    REQUIRE(cpu.spaceCycles.size() == 1);
    for (auto &a : cpu.log) CHECK(a.fc != FCv::CPU);                // Stack, vector and fetch cycles are no CPU space cycles
    CHECK(cpu.readFC() != FCv::CPU);
    END_MODELS
}

TEST_CASE("CPU space: BKPT acknowledges the breakpoint number")
{
    for (auto model : { CPUModel::M68010, CPUModel::M68020 }) {
        for (int n = 0; n < 8; n++) {

            INFO("Model ", int(model), ", breakpoint ", n);
            IrqCPU cpu(model);
            cpu.load({ u16(0x4848 | n) });                          // bkpt #n
            cpu.setSR(0x2700);
            cpu.step();

            REQUIRE(cpu.spaceCycles.size() == 1);
            CHECK(cpu.spaceCycles[0].type == CpuSpaceType::BREAKPOINT);
            CHECK(cpu.spaceCycles[0].arg == n);
            CHECK(cpu.spaceCycles[0].fc == FCv::CPU);
            CHECK(cpu.getPC0() == IrqCPU::handler(4));              // The exception follows the cycle
            CHECK(cpu.readFC() != FCv::CPU);
        }
    }
}

TEST_CASE("CPU space: the 68000 has no BKPT cycle")
{
    IrqCPU cpu(CPUModel::M68000);
    cpu.load({ 0x4848 });
    cpu.setSR(0x2700);
    cpu.step();
    CHECK(cpu.spaceCycles.empty());
    CHECK(cpu.getPC0() == IrqCPU::handler(4));                      // Illegal instruction
}

TEST_CASE("CPU space: ordinary instructions and exceptions raise no cycle")
{
    FOR_EACH_MODEL
    cpu.load({ 0x4E40, 0x4E71, 0x4AFC });                           // trap #0 / nop / illegal
    cpu.setSR(0x2700);
    cpu.step(4);
    CHECK(cpu.spaceCycles.empty());
    END_MODELS
}

// Coprocessor interface: the first cycle of the protocol is a CPU space cycle
// with the coprocessor ID on A15-A13 and the register select on A4-A0.

static u8 cpArg(int cpid, u8 cir) { return u8(cpid << 5 | cir); }

TEST_CASE("CPU space: coprocessor cycles of the generic coprocessor interface")
{
    struct Case { const char *name; std::vector<u16> code; u8 cir; };

    // cpid 5 (no coprocessor attached)
    const Case cases[] = {
        { "cpGEN",     { 0xFA00, 0x0000 },                         CIR::COMMAND   },
        { "cpBcc.w",   { 0xFA80, 0x0001, 0x0000 },                 CIR::CONDITION },
        { "cpBcc.l",   { 0xFAC0, 0x0001, 0x0000, 0x0000 },         CIR::CONDITION },
        { "cpScc",     { 0xFA40, 0x0001 },                         CIR::CONDITION },
        { "cpTRAPcc",  { 0xFA7A, 0x0001, 0x0000 },                 CIR::CONDITION },
        { "cpSAVE",    { 0xFB10 },                                 CIR::SAVE      },
        { "cpRESTORE", { 0xFB50 },                                 CIR::RESTORE   },
    };

    for (auto model : { CPUModel::M68020, CPUModel::M68EC020, CPUModel::M68030 }) {
        for (auto &c : cases) {

            INFO("Model ", int(model), ", ", c.name);
            IrqCPU cpu(model);
            cpu.load(c.code);
            cpu.setSR(0x2700);
            cpu.step();

            REQUIRE(cpu.spaceCycles.size() == 1);
            CHECK(cpu.spaceCycles[0].type == CpuSpaceType::COPROCESSOR);
            CHECK(cpu.spaceCycles[0].arg == cpArg(5, c.cir));
            CHECK(cpu.spaceCycles[0].fc == FCv::CPU);
            CHECK(cpu.getPC0() == IrqCPU::handler(11));             // No coprocessor: line-F exception
            CHECK(cpu.readFC() != FCv::CPU);
        }
    }
}

TEST_CASE("CPU space: the coprocessor ID is taken from the opcode")
{
    for (int cpid = 0; cpid < 8; cpid++) {

        if (cpid == 1) continue;                                    // Reserved for the FPU
        IrqCPU cpu(CPUModel::M68020);
        cpu.load({ u16(0xF000 | cpid << 9), 0x0000 });              // cpGEN
        cpu.setSR(0x2700);
        cpu.step();

        REQUIRE(cpu.spaceCycles.size() == 1);
        CHECK(cpu.spaceCycles[0].arg == cpArg(cpid, CIR::COMMAND));
    }
}

TEST_CASE("CPU space: models without coprocessor interface have no coprocessor cycles")
{
    for (auto model : { CPUModel::M68000, CPUModel::M68010 }) {

        IrqCPU cpu(model);
        cpu.load({ 0xFA00, 0x0000 });
        cpu.setSR(0x2700);
        cpu.step();
        CHECK(cpu.spaceCycles.empty());
        CHECK(cpu.getPC0() == IrqCPU::handler(11));
    }
}

TEST_CASE("CPU space: privileged coprocessor instructions fail before the cycle")
{
    for (u16 op : { 0xFB10, 0xFB50 }) {                             // cpSAVE, cpRESTORE

        IrqCPU cpu(CPUModel::M68020);
        cpu.load({ op });
        cpu.setSR(0x0700);                                          // User mode
        cpu.step();
        CHECK(cpu.spaceCycles.empty());
        CHECK(cpu.getPC0() == IrqCPU::handler(8));                  // Privilege violation
    }
}

TEST_CASE("CPU space: an external 68881 is accessed via the coprocessor interface")
{
    struct Case { const char *name; std::vector<u16> code; u8 cir; };

    // cpid 1
    const Case cases[] = {
        { "FNOP",      { 0xF280, 0x0000 },                         CIR::CONDITION },
        { "FBcc",      { 0xF281, 0x0004, 0x4E71 },                 CIR::CONDITION },
        { "FADD",      { 0xF200, 0x0022 },                         CIR::COMMAND   },
        { "FMOVE",     { 0xF200, 0x0400 },                         CIR::COMMAND   },
        { "FScc",      { 0xF240, 0x0001 },                         CIR::CONDITION },
        { "FTRAPcc",   { 0xF27A, 0x0001, 0x0000 },                 CIR::CONDITION },
        { "FSAVE",     { 0xF310 },                                 CIR::SAVE      },
        { "FRESTORE",  { 0xF350 },                                 CIR::RESTORE   },
    };

    for (auto fpu : { FPUModel::M68881, FPUModel::M68882 }) {
        for (auto &c : cases) {

            INFO("FPU ", int(fpu), ", ", c.name);
            IrqCPU cpu(CPUModel::M68020);
            cpu.setFpuModel(fpu);
            cpu.load(c.code);
            cpu.poke32(0x2000, 0);
            cpu.setSR(0x2700);
            cpu.setA(0, 0x2000);
            cpu.step();

            REQUIRE(cpu.spaceCycles.size() >= 1);
            CHECK(cpu.spaceCycles[0].type == CpuSpaceType::COPROCESSOR);
            CHECK(cpu.spaceCycles[0].arg == cpArg(1, c.cir));
            CHECK(cpu.spaceCycles[0].fc == FCv::CPU);
            CHECK(cpu.spaceCycles.size() == 1);                     // Only the first cycle is reported
            CHECK(cpu.readFC() != FCv::CPU);
        }
    }
}

TEST_CASE("CPU space: the built-in FPU of the 68040 causes no coprocessor cycles")
{
    IrqCPU cpu(CPUModel::M68040);
    cpu.load({ 0xF200, 0x0022, 0xF280, 0x0000 });                   // FADD, FNOP
    cpu.setSR(0x2700);
    cpu.step(2);
    CHECK(cpu.spaceCycles.empty());
}


//
// Function code pins
//

// Runs one instruction and returns the accesses that are not instruction fetches
// (fetches are the accesses at codeStart and above, which is where the program is).
std::vector<IrqCPU::Access> run(IrqCPU &cpu, bool wantFetches = false)
{
    cpu.log.clear();
    cpu.logging = true;
    cpu.step();
    cpu.logging = false;
    if (wantFetches) return cpu.log;
    std::vector<IrqCPU::Access> result;
    for (auto &a : cpu.log) if (!(a.addr >= IrqCPU::codeStart && a.addr < IrqCPU::codeStart + 0x200)) result.push_back(a);
    return result;
}

TEST_CASE("FC: supervisor accesses")
{
    FOR_EACH_MODEL
    cpu.load({ 0x3039, 0x0000, 0x2000 });                           // move.w $2000,d0
    cpu.setSR(0x2700);
    auto all = run(cpu, true);

    bool sawData = false;
    for (auto &a : all) {
        if (a.addr == 0x2000) { CHECK(a.fc == FCv::SD); sawData = true; }
        else                  { CHECK(a.fc == FCv::SP); }           // Instruction fetches
    }
    CHECK(sawData);
    END_MODELS
}

TEST_CASE("FC: user accesses")
{
    FOR_EACH_MODEL
    cpu.load({ 0x3039, 0x0000, 0x2000 });                           // move.w $2000,d0
    cpu.setSR(0x0000);
    auto all = run(cpu, true);

    bool sawData = false;
    for (auto &a : all) {
        if (a.addr == 0x2000) { CHECK(a.fc == FCv::UD); sawData = true; }
        else                  { CHECK(a.fc == FCv::UP); }
    }
    CHECK(sawData);
    END_MODELS
}

TEST_CASE("FC: user writes use the data space")
{
    FOR_EACH_MODEL
    cpu.load({ 0x33FC, 0x1234, 0x0000, 0x2000 });                   // move.w #$1234,$2000
    cpu.setSR(0x0000);
    auto data = run(cpu);
    REQUIRE(data.size() == 1);
    CHECK(data[0].write);
    CHECK(data[0].addr == 0x2000);
    CHECK(data[0].fc == FCv::UD);
    END_MODELS
}

TEST_CASE("FC: PC-relative operands are program space accesses")
{
    FOR_EACH_MODEL
    cpu.load({ 0x303A, 0x0FFE });                                   // move.w $2000(pc),d0 (PC = $1002)
    for (u16 sr : { 0x0000, 0x2000 }) {
        INFO("SR ", sr);
        cpu.debugger.jump(IrqCPU::codeStart);
        cpu.setSR(sr);
        auto data = run(cpu);
        REQUIRE(data.size() == 1);
        CHECK(data[0].addr == 0x2000);
        CHECK(data[0].fc == (sr ? FCv::SP : FCv::UP));
    }
    END_MODELS
}

TEST_CASE("FC: exception processing runs in supervisor space")
{
    FOR_EACH_MODEL
    cpu.load({ 0x4E40 });                                           // trap #0 (from user mode)
    cpu.setSR(0x0000);
    auto all = run(cpu, true);

    int stackWrites = 0, vectorReads = 0, handlerFetches = 0;
    for (auto &a : all) {
        if (a.write) {
            CHECK(a.addr >= 0x1F00 - 8);
            CHECK(a.addr < 0x1F00);
            CHECK(a.fc == FCv::SD);                                 // Stack frame: supervisor data
            stackWrites++;
        } else if (a.addr >= 0x80 && a.addr < 0x84) {
            CHECK(a.fc == FCv::SD);                                 // Vector table: supervisor data
            vectorReads++;
        } else if (a.addr >= IrqCPU::handler(32)) {
            CHECK(a.fc == FCv::SP);                                 // Handler fetch: supervisor program
            handlerFetches++;
        }
    }
    CHECK(stackWrites == (cpu.shortFrame() ? 3 : 4));
    CHECK(vectorReads == 2);
    CHECK(handlerFetches >= 2);
    END_MODELS
}

TEST_CASE("FC: interrupt processing runs in supervisor space")
{
    FOR_EACH_MODEL
    cpu.load({});
    cpu.setSR(0x0000);                                              // Interrupted from user mode
    cpu.setIPL(3);
    cpu.step();
    cpu.log.clear();
    cpu.logging = true;
    cpu.step();
    cpu.logging = false;

    REQUIRE(!cpu.irqLevels.empty());
    int writes = 0;
    for (auto &a : cpu.log) {
        if (a.write) { CHECK(a.fc == FCv::SD); writes++; }
        else if (a.addr >= 0x6C && a.addr < 0x70) CHECK(a.fc == FCv::SD);
        else CHECK(a.fc == FCv::SP);
    }
    CHECK(writes == (cpu.shortFrame() ? 3 : 4));
    END_MODELS
}

TEST_CASE("FC: MOVES takes the function code from SFC and DFC")
{
    // MOVES exists on the 68010 and above
    for (auto model : { CPUModel::M68010, CPUModel::M68020 }) {
        for (u8 code : { 0, 1, 2, 3, 4, 5, 6, 7 }) {

            INFO("Model ", int(model), ", code ", int(code));
            IrqCPU cpu(model);
            cpu.setSFC(code);
            cpu.setDFC(u8((code + 3) & 7));

            // Write: moves.w d0,(a0)
            cpu.load({ 0x0E50, 0x0800 });
            cpu.setSR(0x2700);
            cpu.setA(0, 0x2000);
            cpu.setD(0, 0xBEEF);
            auto w = run(cpu);
            REQUIRE(w.size() == 1);
            CHECK(w[0].write);
            CHECK(w[0].fc == ((code + 3) & 7));                     // DFC

            // Read: moves.w (a0),d1
            cpu.load({ 0x0E50, 0x1000 });
            cpu.setSR(0x2700);
            auto r = run(cpu);
            REQUIRE(r.size() == 1);
            CHECK(!r[0].write);
            CHECK(r[0].fc == code);                                 // SFC
            CHECK((cpu.getD(1) & 0xFFFF) == 0xBEEF);

            // Afterwards the pins show the normal function code again
            cpu.load({ 0x3039, 0x0000, 0x2000 });
            auto n = run(cpu);
            REQUIRE(n.size() == 1);
            CHECK(n[0].fc == FCv::SD);
        }
    }
}

TEST_CASE("FC: MOVES ignores the privilege level for the data access")
{
    // The instruction itself is privileged, so it always runs in supervisor
    // mode. The instruction fetches keep using the supervisor program space.
    for (auto model : { CPUModel::M68010, CPUModel::M68020 }) {

        INFO("Model ", int(model));
        IrqCPU cpu(model);
        cpu.setDFC(1);                                              // User data
        cpu.load({ 0x0E50, 0x0800 });
        cpu.setSR(0x2700);
        cpu.setA(0, 0x2000);
        auto all = run(cpu, true);
        for (auto &a : all) CHECK(a.fc == (a.addr == 0x2000 ? FCv::UD : FCv::SP));
    }
}

TEST_CASE("FC: MOVES is privileged")
{
    for (auto model : { CPUModel::M68010, CPUModel::M68020 }) {

        INFO("Model ", int(model));
        IrqCPU cpu(model);
        cpu.load({ 0x0E50, 0x0800 });
        cpu.setSR(0x0000);
        cpu.setA(0, 0x2000);
        cpu.step();
        CHECK(cpu.getPC0() == IrqCPU::handler(8));                  // Privilege violation
    }
}

TEST_CASE("FC: the 68000 has no MOVES")
{
    IrqCPU cpu(CPUModel::M68000);
    cpu.load({ 0x0E50, 0x0800 });
    cpu.setSR(0x2700);
    cpu.step();
    CHECK(cpu.getPC0() == IrqCPU::handler(4));                      // Illegal instruction
}

TEST_CASE("FC: the pins follow the privilege level")
{
    FOR_EACH_MODEL
    // After RTE to user mode, the next data access uses FC 1
    cpu.load({ 0x4E73 });                                           // rte
    cpu.poke16(0x1100, 0x3039); cpu.poke16(0x1102, 0x0000); cpu.poke16(0x1104, 0x2000);
    cpu.setSR(0x2700);
    u32 sp = 0x1F00 - cpu.frameSize();
    cpu.setISP(sp);
    cpu.poke16(sp, 0x0000);                                         // SR = user mode
    cpu.poke32(sp + 2, 0x1100);                                     // PC
    if (!cpu.shortFrame()) cpu.poke16(sp + 6, 0x0000);              // Format 0
    cpu.step();                                                     // rte
    REQUIRE((cpu.getSR() & 0x2000) == 0);
    auto data = run(cpu);
    for (auto &a : data) INFO("access ", a.write, " ", a.addr, " fc ", a.fc);
    REQUIRE(data.size() == 1);
    CHECK(data[0].fc == FCv::UD);
    END_MODELS
}

TEST_SUITE_END();

}
