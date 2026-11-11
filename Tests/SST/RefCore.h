// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "SSTTypes.h"
#include "MameCpu.h"
#include "Common.h"
#include <functional>
#include <memory>

namespace sstrunner {

//
// Reference CPUs
//

// Common interface of the reference CPUs (the results are reported in MAME's types)
struct RefCore {

    virtual ~RefCore() = default;

    virtual void load(const sst::State &s) = 0;                         // Sets the registers (the memory is loaded already)
    virtual int execute() = 0;                                          // Runs one instruction and returns the elapsed cycles
    virtual mame::State state() const = 0;
    virtual const std::vector<mame::BusCycle> &log() const = 0;
    virtual bool halted() const = 0;
    virtual bool hasBusLog() const { return true; }                     // Does the core report its bus cycles?

    // Selects the CPU model. Unless a core overrides this, it only supports the 68000.
    virtual bool supports(CPUModel model) const { return model == CPUModel::M68000; }
    virtual void setModel(CPUModel model) { }

    //
    // Differences between the references
    //

    // Name of the core (shown in the banner)
    virtual const char *name() const = 0;
    virtual const char *shortName() const = 0;

    // Compared bits of the first word of bus and address error frames
    virtual u16 codeMask() const { return 0xFFFF; }

    // Are the registers of a halted CPU comparable?
    virtual bool comparesHaltedRegs() const { return true; }

    // Does the core log the aborted access of an address error?
    virtual bool logsAbortedAccess() const { return true; }

    // Does the core disagree with MAME on frames that fault while a group 1/2 exception is processed?
    virtual bool disagreesOnStacking() const { return false; }

    // Test cases the core cannot execute
    virtual bool unsupported(const sst::Test &t) const { return false; }
    virtual const char *unsupportedNote() const { return ""; }
};

// Creates a reference core that operates on the given memory
using CoreFactory = std::function<RefCore *(Memory &, Fault &)>;

struct Reference {

    Memory mem;
    Fault fault;
    std::unique_ptr<RefCore> core;
    RefCore &cpu;

    explicit Reference(const CoreFactory &create) : core(create(mem, fault)), cpu(*core) { }

    void load(const sst::State &s)
    {
        mem.load(s);
        cpu.load(s);
        fault = Fault();
    }
};

}
