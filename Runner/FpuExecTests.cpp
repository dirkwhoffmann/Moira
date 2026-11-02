// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "Moira.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace moira;

static int failures = 0;
static int checks = 0;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
    printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)

static bool same(const Float80 &a, const Float80 &b) { return a.identical(b); }

/* A minimal system consisting of 1 MB of RAM
 */
class TestCPU : public Moira {

public:

    using Moira::reg;
    using Moira::fpu;

    mutable std::vector<u8> ram = std::vector<u8>(1 << 20);

    u8 read8(u32 addr) const override { return ram[addr & 0xFFFFF]; }
    u16 read16(u32 addr) const override { return read8(addr) << 8 | read8(addr + 1); }
    u32 read32(u32 addr) const override { return read16(addr) << 16 | read16(addr + 2); }
    void write8(u32 addr, u8 val) const override { ram[addr & 0xFFFFF] = val; }
    void write16(u32 addr, u16 val) const override { write8(addr, val >> 8); write8(addr + 1, val & 0xFF); }
    void write32(u32 addr, u32 val) const override { write16(addr, val >> 16); write16(addr + 2, val & 0xFFFF); }

    static constexpr u32 codeStart = 0x1000;

    TestCPU(FPUModel fpuModel = FPUModel::M68882)
    {
        setModel(Model::M68040);
        fpu.setModel(fpuModel);

        // Reset vector
        write32(0, 0x8000);
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

    u32 pcOffset() const { return reg.pc0 - codeStart; }
};

static Float80 F(double d) { return Float80::fromDouble(d); }
static void put32(TestCPU &cpu, u32 addr, u32 val) { cpu.write32(addr, val); }

static void testMove()
{
    TestCPU cpu;

    // FMOVE.L D0,FP0
    cpu.reg.d[0] = 42;
    cpu.run({ 0xF200, 0x4000 });
    CHECK(same(cpu.fpu.fpr[0], Float80::fromInt(42)));
    CHECK((cpu.fpu.fpsr & FPSR::CC_MASK) == 0);

    // FMOVE.W D1,FP1 (negative)
    cpu.reg.d[1] = 0xFFFF;
    cpu.run({ 0xF201, 0x5080 });
    CHECK(same(cpu.fpu.fpr[1], Float80::fromInt(-1)));
    CHECK((cpu.fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_N);

    // FMOVE.B D2,FP2
    cpu.reg.d[2] = 0x1280;
    cpu.run({ 0xF202, 0x5900 });
    CHECK(same(cpu.fpu.fpr[2], Float80::fromInt(-128)));

    // FMOVE.S D0,FP3: 1.5f
    cpu.reg.d[0] = 0x3FC00000;
    cpu.run({ 0xF200, 0x4580 });
    CHECK(same(cpu.fpu.fpr[3], F(1.5)));

    // FMOVE.D #2.5,FP4
    cpu.run({ 0xF23C, 0x5600, 0x4004, 0x0000, 0x0000, 0x0000 });
    CHECK(same(cpu.fpu.fpr[4], F(2.5)));
    CHECK(cpu.pcOffset() == 12);        // The program counter is behind the instruction

    // FMOVE.X #imm,FP5
    cpu.run({ 0xF23C, 0x4A80, 0x4000, 0x0000, 0xC90F, 0xDAA2, 0x2168, 0xC235 });
    CHECK(same(cpu.fpu.fpr[5], Float80::fromBits(0x4000, 0xC90FDAA22168C235ULL)));

    // FMOVE FP4,FP6
    cpu.run({ 0xF200, 0x1300 });
    CHECK(same(cpu.fpu.fpr[6], F(2.5)));
    CHECK((cpu.fpu.fpsr & FPSR::CC_MASK) == 0);

    // FMOVE.L FP4,D3 rounds to nearest even
    cpu.run({ 0xF203, 0x6000 | (4 << 7) });
    CHECK(cpu.reg.d[3] == 2);

    // FMOVE.S FP0 -> memory (A0)
    cpu.reg.a[0] = 0x2000;
    cpu.run({ 0xF210, 0x6400 | (3 << 7) });
    CHECK(cpu.read32(0x2000) == 0x3FC00000);

    // FMOVE.D FP4,-(A0) and FMOVE.D (A0)+,FP7
    cpu.reg.a[0] = 0x2010;
    cpu.run({ 0xF220, 0x7400 | (4 << 7) });
    CHECK(cpu.reg.a[0] == 0x2008);
    CHECK(cpu.read32(0x2008) == 0x40040000 && cpu.read32(0x200C) == 0);
    cpu.run({ 0xF218, 0x5400 | (7 << 7) });
    CHECK(cpu.reg.a[0] == 0x2010);
    CHECK(same(cpu.fpu.fpr[7], F(2.5)));

    // FMOVE.X FP5,(8,A1) and back via FMOVE.X (8,A1),FP0
    cpu.reg.a[1] = 0x3000;
    cpu.run({ 0xF229, 0x6800 | (5 << 7), 0x0008 });
    CHECK(cpu.read16(0x3008) == 0x4000 && cpu.read16(0x300A) == 0);
    CHECK(cpu.read32(0x300C) == 0xC90FDAA2 && cpu.read32(0x3010) == 0x2168C235);
    cpu.run({ 0xF229, 0x4800, 0x0008 });
    CHECK(same(cpu.fpu.fpr[0], Float80::fromBits(0x4000, 0xC90FDAA22168C235ULL)));

    // Single precision rounding of a register-to-register move (FSMOVE needs the 68040)
    {
        TestCPU c040(FPUModel::INTERNAL);
        c040.fpu.fpr[0] = Float80::one() / Float80::fromInt(3);
        c040.run({ 0xF200, 0x0040 | (1 << 7) });
        CHECK(c040.fpu.fpr[1].mantissa() == 0xAAAAAB0000000000ULL);
    }
}

static void testPacked()
{
    TestCPU cpu;

    // FMOVE.P #-1.5E2,FP0
    cpu.run({ 0xF23C, 0x4C00, 0x8002, 0x0001, 0x5000, 0x0000, 0x0000, 0x0000 });
    CHECK(same(cpu.fpu.fpr[0], Float80::fromInt(-150)));

    // FMOVE.P FP0,(A0) with k-factor 17 (static)
    cpu.reg.a[0] = 0x2000;
    cpu.run({ 0xF210, 0x6C00 | 0x0011 });
    CHECK(cpu.read32(0x2000) == 0x80020001 && cpu.read32(0x2004) == 0x50000000 && cpu.read32(0x2008) == 0);

    // Dynamic k-factor: D1 = 3 digits of 1/3
    cpu.fpu.fpr[1] = Float80::one() / Float80::fromInt(3);
    cpu.reg.d[1] = 3;
    cpu.run({ 0xF210, 0x7C00 | (1 << 7) | (1 << 4) });
    CHECK(cpu.read32(0x2000) == 0x40010003 && cpu.read32(0x2004) == 0x33000000);
    CHECK(cpu.fpu.fpsr & FPEXP::INEX2);
}

static void testMovecr()
{
    TestCPU cpu;

    cpu.run({ 0xF200, 0x5C00 });
    CHECK(same(cpu.fpu.fpr[0], Float80::fromBits(0x4000, 0xC90FDAA22168C235ULL)));
    CHECK(cpu.fpu.fpsr & FPEXP::INEX2);

    cpu.run({ 0xF200, 0x5C00 | (2 << 7) | 0x33 });
    CHECK(same(cpu.fpu.fpr[2], Float80::fromInt(10)));
    CHECK((cpu.fpu.fpsr & FPEXP::INEX2) == 0);

    // Rounding mode affects the constant
    cpu.fpu.setFPCR(FPCR::RND_ZERO);
    cpu.run({ 0xF200, 0x5C00 });
    CHECK(same(cpu.fpu.fpr[0], Float80::fromBits(0x4000, 0xC90FDAA22168C234ULL)));
}

static void testMovem()
{
    TestCPU cpu;

    cpu.fpu.fpr[0] = F(1);
    cpu.fpu.fpr[1] = F(2);
    cpu.fpu.fpr[7] = F(8);

    // FMOVEM.X FP0-FP1/FP7,-(A0)
    cpu.reg.a[0] = 0x2000;
    cpu.run({ 0xF220, 0xE000 | 0x83 });
    CHECK(cpu.reg.a[0] == 0x2000 - 36);
    CHECK(cpu.read16(0x2000 - 12) == 0x4002);            // FP7 = 8 is stored at the highest address
    CHECK(cpu.read16(0x2000 - 24) == 0x4000);            // FP1 = 2
    CHECK(cpu.read16(0x2000 - 36) == 0x3FFF);            // FP0 = 1

    // FMOVEM.X (A0)+,FP2-FP4 (static, postincrement)
    cpu.run({ 0xF218, 0xD000 | 0x38 });
    CHECK(cpu.reg.a[0] == 0x2000 - 36 + 36);
    CHECK(same(cpu.fpu.fpr[2], F(1)));
    CHECK(same(cpu.fpu.fpr[3], F(2)));
    CHECK(same(cpu.fpu.fpr[4], F(8)));

    // Dynamic register list in D2: FMOVEM.X D2,FP... store FP0 and FP2 with postincrement
    cpu.reg.a[1] = 0x4000;
    cpu.reg.d[2] = 0xA0;
    cpu.run({ 0xF211, 0xF800 | (2 << 4) });
    CHECK(cpu.read16(0x4000) == 0x3FFF && cpu.read16(0x400C) == 0x3FFF);
    CHECK(cpu.reg.a[1] == 0x4000);

    // Control registers
    cpu.reg.a[2] = 0x5000;
    cpu.run({ 0xF23C, 0x9000, 0x0000, 0x0010 });                        // FMOVE.L #$10,FPCR
    CHECK(cpu.fpu.getFPCR() == 0x10 && cpu.fpu.getRoundingMode() == RoundingMode::ZERO);
    cpu.fpu.setFPCR(0);
    cpu.fpu.setFPIAR(0x12345678);
    cpu.fpu.setFPSR(0x08000000);

    cpu.run({ 0xF212, 0xA000 | 0x1C00 });                               // FMOVEM.L FPCR/FPSR/FPIAR,(A2)
    CHECK(cpu.read32(0x5000) == 0 && cpu.read32(0x5004) == 0x08000000 && cpu.read32(0x5008) == 0x12345678);

    cpu.reg.d[4] = 0x20;
    cpu.run({ 0xF204, 0x9000 });                                        // FMOVE.L D4,FPCR
    CHECK(cpu.fpu.getFPCR() == 0x20);
    cpu.run({ 0xF204, 0xA000 | 0x0400 });                               // FMOVE.L FPIAR,D4
    CHECK(cpu.reg.d[4] == 0x12345678);

    cpu.reg.a[3] = 0x6000;
    cpu.run({ 0xF223, 0xA000 | 0x1C00 });                               // FMOVEM.L FPCR/FPSR/FPIAR,-(A3)
    CHECK(cpu.reg.a[3] == 0x6000 - 12);
    CHECK(cpu.read32(0x6000 - 12) == 0x20 && cpu.read32(0x6000 - 4) == 0x12345678);
}

static void testArithmetic()
{
    TestCPU cpu;

    cpu.fpu.fpr[0] = F(1.5);
    cpu.fpu.fpr[1] = F(2.25);

    // FADD.X FP0,FP1
    cpu.run({ 0xF200, 0x0022 | (1 << 7) });
    CHECK(same(cpu.fpu.fpr[1], F(3.75)));

    // FSUB.D (A0),FP1
    cpu.reg.a[0] = 0x2000;
    cpu.write32(0x2000, 0x3FF80000); cpu.write32(0x2004, 0);                  // 1.5
    cpu.run({ 0xF210, 0x5400 | (1 << 7) | 0x28 });
    CHECK(same(cpu.fpu.fpr[1], F(2.25)));

    // FMUL.L #4,FP1
    cpu.run({ 0xF23C, 0x4000 | (1 << 7) | 0x23, 0x0000, 0x0004 });
    CHECK(same(cpu.fpu.fpr[1], F(9)));

    // FDIV.W D0,FP1 with D0 = 3
    cpu.reg.d[0] = 3;
    cpu.run({ 0xF200, 0x5000 | (1 << 7) | 0x20 });
    CHECK(same(cpu.fpu.fpr[1], F(3)));

    // FSQRT FP1,FP2
    cpu.fpu.fpr[1] = F(2.25);
    cpu.run({ 0xF200, 0x0004 | (1 << 10) | (2 << 7) });
    CHECK(same(cpu.fpu.fpr[2], F(1.5)));

    // FNEG FP1,FP2, FABS, FTST, FCMP
    cpu.run({ 0xF200, 0x001A | (1 << 10) | (2 << 7) });
    CHECK(same(cpu.fpu.fpr[2], F(-2.25)));
    cpu.run({ 0xF200, 0x0018 | (2 << 10) | (3 << 7) });
    CHECK(same(cpu.fpu.fpr[3], F(2.25)));
    cpu.run({ 0xF200, 0x003A | (2 << 10) });
    CHECK((cpu.fpu.fpsr & FPSR::CC_MASK) == FPSR::CC_N);
    cpu.run({ 0xF200, 0x0038 | (0 << 10) | (1 << 7) });                      // FCMP FP0,FP1: 2.25 - 1.5
    CHECK((cpu.fpu.fpsr & FPSR::CC_MASK) == 0);

    // Exceptions: 1 / 0 sets DZ, 0 * inf produces the default NaN
    cpu.fpu.fpr[4] = F(1);
    cpu.fpu.fpr[5] = F(0);
    cpu.run({ 0xF200, 0x0020 | (5 << 10) | (4 << 7) });
    CHECK(cpu.fpu.fpr[4].isInfinite());
    CHECK(cpu.fpu.fpsr & FPEXP::DZ);
    CHECK(cpu.fpu.fpsr & (1 << 4));

    cpu.fpu.fpr[4] = Float80::infinity();
    cpu.run({ 0xF200, 0x0023 | (5 << 10) | (4 << 7) });
    CHECK(same(cpu.fpu.fpr[4], FPU::defaultNaN()));
    CHECK(cpu.fpu.fpsr & FPEXP::OPERR);
    CHECK(!(cpu.fpu.fpsr & FPEXP::DZ));         // The exception status byte is cleared per instruction

    // FMOD / FREM
    cpu.fpu.fpr[0] = F(11);
    cpu.fpu.fpr[1] = F(3);
    cpu.run({ 0xF200, 0x0021 | (1 << 10) | (0 << 7) });
    CHECK(same(cpu.fpu.fpr[0], F(2)));
    CHECK(((cpu.fpu.fpsr >> 16) & 0xFF) == 3);

    // Source format in memory, post increment
    cpu.reg.a[0] = 0x2100;
    cpu.write16(0x2100, 0x4000); cpu.write16(0x2102, 0); cpu.write32(0x2104, 0xC0000000); cpu.write32(0x2108, 0);
    cpu.fpu.fpr[0] = F(1);
    cpu.run({ 0xF218, 0x4800 | 0x22 });                                      // FADD.X (A0)+,FP0
    CHECK(same(cpu.fpu.fpr[0], F(4)));
    CHECK(cpu.reg.a[0] == 0x210C);

    // 68040: FSADD rounds to single, FADD on 68040 is available too
    TestCPU c040(FPUModel::INTERNAL);
    c040.fpu.fpr[0] = Float80::one() / Float80::fromInt(3);
    c040.fpu.fpr[1] = Float80::zero();
    c040.fpu.fpr[2] = Float80::zero();
    c040.run({ 0xF200, 0x0062 | (0 << 10) | (1 << 7) });                      // FSADD FP0,FP1
    CHECK(c040.fpu.fpr[1].mantissa() == 0xAAAAAB0000000000ULL);
    c040.run({ 0xF200, 0x0066 | (0 << 10) | (2 << 7) });                      // FDADD FP0,FP2
    CHECK(c040.fpu.fpr[2].mantissa() == 0xAAAAAAAAAAAAA800ULL);
}

static void testTraps()
{
    // The 68040 does not implement FMOD in hardware. The instruction causes a line F exception.
    TestCPU cpu(FPUModel::INTERNAL);
    cpu.write32(11 * 4, 0x9000);
    cpu.fpu.fpr[0] = F(11);
    cpu.fpu.fpr[1] = F(3);
    cpu.run({ 0xF200, 0x0021 | (1 << 10) | (0 << 7) });
    CHECK(cpu.reg.pc0 == 0x9000);
    CHECK(same(cpu.fpu.fpr[0], F(11)));

    // Address registers cannot serve as floating-point operands
    TestCPU cpu2;
    cpu2.write32(11 * 4, 0x9000);
    cpu2.run({ 0xF208, 0x4000 });
    CHECK(cpu2.reg.pc0 == 0x9000);

    // Neither can a double be read from a data register
    TestCPU cpu4;
    cpu4.write32(11 * 4, 0x9000);
    cpu4.run({ 0xF200, 0x5400 });
    CHECK(cpu4.reg.pc0 == 0x9000);

    // Transcendental functions are not yet supported
    TestCPU cpu3;
    cpu3.write32(11 * 4, 0x9000);
    cpu3.run({ 0xF200, 0x000E });                                           // FSIN FP0,FP0
    CHECK(cpu3.reg.pc0 == 0x9000);
}

int main()
{
    testMove();
    testPacked();
    testMovecr();
    testMovem();
    testArithmetic();
    testTraps();

    printf("%d checks, %d failures\n", checks, failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
