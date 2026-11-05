// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// Connects the test runner to one Moira configuration.
//
// This file is compiled once per configuration (MOIRA_NAMESPACE selects it),
// and everything in it lives in that configuration's namespace.

#include "config.h"
#include "TestCPU.h"

namespace MOIRA_NAMESPACE::runner {

class Adapter : public ::runner::MoiraAdapter {

    TestCPU cpu;

    static Float80 toFloat80(const ::runner::FpReg &r) { return Float80::fromBits(r.se, r.man); }
    static ::runner::FpReg fromFloat80(const Float80 &f) { return { f.signExp(), f.mantissa() }; }

    void recordRegisters(::runner::Result &r) const;

    // Used by the doExec macro
    bool isFpuOpcode(u16 op) const { return (op & 0xFF00) == 0xF200 && cpu.hasFPU(); }

public:

    std::string build() const override { return Moira::build(); }
    int compat() const override { return int(MOIRA_NAMESPACE::compat); }

    void setModel(int model) override { cpu.setModel(Model(model)); }
    u32 cacrMask() const override { return cpu.cacrMask(); }
    bool hasMMU() const override { return cpu.hasMMU(); }
    bool hasFPU() const override { return cpu.hasFPU(); }

    ::runner::InstrInfo getInstrInfo(u16 op) const override
    {
        auto info = cpu.getInstrInfo(op);
        return { int(info.I), int(info.S), int(info.M) };
    }

    void reset(const ::runner::Setup &s) override;
    void resetFpu(const ::runner::Setup &s) override;
    void run(::runner::Result &r) override;
    void recordFpu(::runner::Result &r) override;
};

void
Adapter::reset(const ::runner::Setup &s)
{
    cpu.reset();
    cpu.setClock(0);

    for (int i = 0; i < 8; i++) {
        cpu.setD(i, s.d[i]);
    }
    for (int i = 0; i < 7; i++) {
        cpu.setA(i, s.a[i]);
    }
    cpu.setVBR(s.vbr);
    cpu.setSFC(s.sfc);
    cpu.setDFC(s.dfc);
    cpu.setCCR(s.ccr);
    cpu.setCACR(s.cacr);
    cpu.setCAAR(s.caar);
    cpu.setSupervisorMode(s.supervisor);
}

void
Adapter::resetFpu(const ::runner::Setup &s)
{
    for (int i = 0; i < 8; i++) cpu.fpu.fpr[i] = toFloat80(s.fpr[i]);
    cpu.fpu.setFPCR(s.fpcr);
    cpu.fpu.setFPSR(s.fpsr);
    cpu.fpu.setFPIAR(s.fpiar);
}

void
Adapter::recordFpu(::runner::Result &r)
{
    for (int i = 0; i < 8; i++) r.fpr[i] = fromFloat80(cpu.fpu.fpr[i]);
    r.fpcr = cpu.fpu.getFPCR();
    r.fpsr = cpu.fpu.getFPSR();
    r.fpiar = cpu.fpu.getFPIAR();
}

void
Adapter::recordRegisters(::runner::Result &r) const
{
    r.pc = cpu.getPC();
    r.sr = cpu.getSR();
    r.usp = cpu.getUSP();
    r.isp = cpu.getISP();
    r.msp = cpu.getMSP();
    r.fc = cpu.readFC();
    r.vbr = cpu.getVBR();
    r.sfc = cpu.getSFC();
    r.dfc = cpu.getDFC();
    r.cacr = cpu.getCACR();
    r.caar = cpu.getCAAR();
    for (int i = 0; i < 8; i++) r.d[i] = cpu.getD(i);
    for (int i = 0; i < 8; i++) r.a[i] = cpu.getA(i);
}

void
Adapter::run(::runner::Result &r)
{
    r.oldpc = cpu.getPC();
    r.opcode = get16(moiraMem, r.oldpc);
    r.elapsed[0] = r.elapsed[1] = 0;

    // Disassemble the instruction
    if (doDasm(r.opcode)) {

        // Disassemble the instruction in Musashi format
        cpu.setDasmSyntax(Syntax::MUSASHI);
        cpu.setDasmLetterCase(LetterCase::MIXED_CASE);
        cpu.setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        clock_t elapsed = clock();
        r.dasmCntMusashi = cpu.disassemble(r.dasmMusashi, r.oldpc);
        r.elapsed[1] = clock() - elapsed;

        // Disassemble the instruction in GNU format (binutils)
#ifdef MOTOROLA
        cpu.setDasmSyntax(Syntax::GNU);
#else
        cpu.setDasmSyntax(Syntax::GNU_MIT);
#endif
        cpu.setDasmNumberFormat({ .prefix = "$", .radix = 10, .plainZero = true });
        r.dasmCntBinutils = cpu.disassemble(r.dasmBinutils, r.oldpc);

        // Disassemble the instruction in Moira's own format (compared with UAE)
        cpu.setDasmSyntax(Syntax::UAE);
        cpu.setDasmLetterCase(LetterCase::LOWER_CASE);
        cpu.setDasmNumberFormat({ .prefix = "$", .radix = 16 });
        r.dasmCntMoiraUAE = cpu.disassemble(r.dasmMoiraUAE, r.oldpc);
    }

    // Run the Moira CPU
    if (doExec(r.opcode)) {

        u32 pc = cpu.getPC();
        u16 op = get16(moiraMem, pc);

        int64_t cycles = cpu.getClock();

        if (VERBOSE)
            printf("$%04x ($%04x): %s (Moira)\n", r.oldpc, r.opcode, r.dasmMusashi);

        // Execute instruction
        clock_t elapsed = clock();
        cpu.execute();
        r.elapsed[0] = clock() - elapsed;

        // Record the result
        r.cycles = (int)(cpu.getClock() - cycles);
        r.oldpc = pc;
        r.opcode = op;
        recordRegisters(r);
    }
}

// Entry point used by the runner. Each configuration exports its own.
std::unique_ptr<::runner::MoiraAdapter> create() { return std::make_unique<Adapter>(); }

}
