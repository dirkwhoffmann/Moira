// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "MameCpu.h"
#include "emu.h"
#include "m68000.h"

// MAME's core has a second set of handlers for systems with an external MMU.
// They are not needed here (the MMU hook is never installed).
const m68000_device::handler m68000_device::s_handlers_if[] = { nullptr };
const m68000_device::handler m68000_device::s_handlers_ip[] = { nullptr };

namespace mame {

namespace {

struct StopRun { };

// Gives access to the internals of MAME's 68000
class Device : public m68000_device {

public:

    Device() : m68000_device(config, nullptr, nullptr, 0) { }

    using m68000_device::m_da;
    using m68000_device::m_sr;
    using m68000_device::m_pc;
    using m68000_device::m_ipc;
    using m68000_device::m_au;
    using m68000_device::m_ird;
    using m68000_device::m_irdi;
    using m68000_device::m_ir;
    using m68000_device::m_irc;
    using m68000_device::m_dbin;
    using m68000_device::m_icount;
    using m68000_device::m_inst_state;
    using m68000_device::m_inst_substate;
    using m68000_device::m_decode_table;
    using m68000_device::m_post_run;
    using m68000_device::m_sp;
    using m68000_device::m_isr;
    using m68000_device::m_handlers_f;
    using m68000_device::m_handlers_p;
    using m68000_device::do_post_run;

    /* Runs the core until the instruction that has been loaded is complete.
     * This is MAME's execute_run() loop with an additional stop condition: The
     * loop ends when the next instruction begins or when a trace or interrupt
     * exception is about to start. If the loaded instruction raises an
     * exception itself (e.g., an address error), the exception sequence is
     * part of the run and ends when the first instruction of the handler
     * begins.
     */
    bool runOne()
    {
        int instructions = 0;

        for (;;) {

            if (m_icount > 0 && m_inst_substate)
                (this->*(m_handlers_p[m_inst_state]))();

            while (m_icount > 0) {

                // Exceptions that are raised by an instruction (e.g., a privilege violation)
                // use instruction states. They are no new instructions, because the
                // state does not belong to the opcode in the instruction register.
                bool begins = m_inst_state >= S_first_instruction && m_inst_state == m_decode_table[m_ird];

                if (begins) {

                    if (++instructions == 2) return true;
                    m_ipc = m_pc - 2;
                    m_irdi = m_ird;

                } else if (instructions >= 1 && (m_inst_state == S_TRACE || m_inst_state == S_INTERRUPT)) {

                    return true;
                }
                (this->*(m_handlers_f[m_inst_state]))();
            }

            if (m_post_run) do_post_run(); else break;
        }
        return false;
    }

    void prepare()
    {
        device_start();
        device_reset();
    }

    void restart(u32 ipc, u16 ird, u16 irc)
    {
        m_ipc = ipc;
        m_pc = ipc + 2;
        m_au = ipc + 4;
        m_ir = m_ird = m_irdi = ird;
        m_irc = m_dbin = irc;
        set_ftu_const();
        m_inst_state = m_decode_table[m_ird];
        m_inst_substate = 0;
        m_post_run = 0;
        update_interrupt();
    }

    void setSR(u16 sr)
    {
        m_sr = sr & (SR_SR | SR_CCR);
        m_isr = m_sr & SR_CCR;     // The ALU flags mirror the condition codes
        update_user_super();
        update_interrupt();
    }

private:

    machine_config config;
};

}

struct Cpu::Impl : public MameBus {

    Cpu &cpu;
    Device dev;
    std::vector<BusCycle> cycles;
    int budget = 0;
    int hooks = 0;
    bool wasHalted = false;

    Impl(Cpu &c) : cpu(c)
    {
        dev.connect(this);
        dev.prepare();
    }

    int now() const { return budget - dev.m_icount; }

    u16 read(u32 addr, u16 mask, int fc) override
    {
        // Interrupt acknowledge cycle
        if (fc == 7) {

            int level = (addr & 0xF) >> 1;
            u8 v = cpu.vector ? cpu.vector(level) : u8(0x18 + level);
            cycles.push_back({ now(), false, addr, v, mask, fc });
            return v;
        }

        bool fault = cpu.busError && cpu.busError(addr, false, fc);
        u16 value = fault ? 0 : cpu.read(addr, mask);
        cycles.push_back({ now(), false, addr, value, mask, fc });

        if (fault) dev.trigger_bus_error();
        return value;
    }

    void write(u32 addr, u16 val, u16 mask, int fc) override
    {
        bool fault = cpu.busError && cpu.busError(addr, true, fc);
        cycles.push_back({ now(), true, addr, val, mask, fc });

        if (fault) { dev.trigger_bus_error(); return; }
        cpu.write(addr, val, mask);
    }
};

Cpu::Cpu() : impl(std::make_unique<Impl>(*this)) { }
Cpu::~Cpu() = default;

void
Cpu::load(const State &s)
{
    auto &d = impl->dev;

    for (int i = 0; i < 8; i++) d.m_da[i] = s.d[i];
    for (int i = 0; i < 7; i++) d.m_da[8 + i] = s.a[i];
    d.m_da[15] = s.usp;
    d.m_da[16] = s.ssp;

    d.setSR(s.sr);
    d.restart(s.pc, s.ird, s.irc);
    impl->wasHalted = false;
}

int
Cpu::execute()
{
    auto &d = impl->dev;

    impl->cycles.clear();
    impl->hooks = 0;
    impl->budget = 100000;
    d.m_icount = impl->budget;

    impl->wasHalted = !d.runOne();

    return impl->budget - d.m_icount;
}

State
Cpu::state() const
{
    auto &d = impl->dev;
    State s;

    for (int i = 0; i < 8; i++) s.d[i] = d.m_da[i];
    for (int i = 0; i < 7; i++) s.a[i] = d.m_da[8 + i];
    s.usp = d.m_da[15];
    s.ssp = d.m_da[16];
    s.sr = d.m_sr;
    s.pc = d.m_pc - 2;
    s.ird = d.m_ird;
    s.irc = d.m_irc;

    return s;
}

const std::vector<BusCycle> &Cpu::log() const { return impl->cycles; }
bool Cpu::halted() const { return impl->wasHalted; }

void
Cpu::setIPL(int level)
{
    impl->dev.execute_set_input(level, ASSERT_LINE);
}

}
