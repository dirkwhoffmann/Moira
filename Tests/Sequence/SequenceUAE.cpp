// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/* Reference core: WinUAE's cycle exact 68000/68010 (see Sequence.cpp) */

#include "Sequence.h"
#include "UAECpu.h"

using namespace sequence;

namespace {

std::vector<u8> uaeMem(MEM_MASK + 1);
std::vector<Write> uaeWrites;

void uaeWrite(int size, uint32_t addr, uint32_t value, uint32_t, uint64_t)
{
    if (size == 1) {
        uaeWrites.push_back({ addr & MEM_MASK, u8(value) });
    } else {
        uaeWrites.push_back({ addr & MEM_MASK, u8(value >> 8) });
        uaeWrites.push_back({ (addr + 1) & MEM_MASK, u8(value) });
    }
}

class UaeReference : public Reference {

    uaecpu::Cpu model = uaecpu::Cpu::M68000;

public:

    const char *name() override { return "UAE"; }

    bool init(const std::string &cpu) override
    {
        if (cpu == "68000") model = uaecpu::Cpu::M68000;
        else if (cpu == "68010") model = uaecpu::Cpu::M68010;
        else if (cpu == "68020") model = uaecpu::Cpu::M68020;
        else if (cpu == "68030") model = uaecpu::Cpu::M68030;
        else if (cpu == "68040") model = uaecpu::Cpu::M68040;
        else return false;
        return true;
    }

    void load(const std::vector<u8> &mem, const Program &prog) override
    {
        uaeMem = mem;
        uaecpu::init(model, uaecpu::Fpu::None, uaeMem.data(), uaeWrite);
        uaecpu::setMemoryMask(MEM_MASK);

        uaecpu::State s;
        for (int i = 0; i < 8; i++) s.d[i] = prog.d[i];
        for (int i = 0; i < 7; i++) s.a[i] = prog.a[i];
        s.sr = u16(prog.sr);
        s.isp = SSP; s.usp = USP; s.pc = PROG;
        uaecpu::setState(s);
        // The 68020 and later fetch their instructions from memory
        if (model == uaecpu::Cpu::M68000 || model == uaecpu::Cpu::M68010)
            uaecpu::setPrefetch(u16(mem[PROG] << 8 | mem[PROG + 1]), u16(mem[PROG + 2] << 8 | mem[PROG + 3]));
        uaecpu::setIPL(0);
    }

    void setIPL(int level) override { uaecpu::setIPL(level); }

    void step() override
    {
        uaeWrites.clear();
        uaecpu::stepSeq();
    }

    RefState state() override
    {
        uaecpu::State u = uaecpu::getState();
        RefState r;
        for (int i = 0; i < 8; i++) r.d[i] = u.d[i];
        for (int i = 0; i < 7; i++) r.a[i] = u.a[i];
        r.usp = u.usp; r.isp = u.isp; r.sp = u.a7; r.sr = u.sr; r.pc = u.pc;
        return r;
    }

    bool stopped() override { return uaecpu::stopped(); }

    bool interrupted() override
    {
        for (int v : uaecpu::exceptions()) if (v >= 25 && v <= 31) return true;
        return false;
    }

    bool addressError() override
    {
        for (int v : uaecpu::exceptions()) if (v == 2 || v == 3) return true;
        return false;
    }

    const std::vector<Write> &writes() override { return uaeWrites; }

    std::string disassemble(u32 addr) override
    {
        char text[128] = "?";
        uaecpu::disassemble(addr, text, sizeof text);
        return text;
    }
};

}

int main(int argc, char **argv)
{
    UaeReference ref;
    return sequence::run(argc, argv, ref);
}
