// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include "MoiraTypes.h"
#include "StrWriter.h"
#include <unordered_map>

namespace MOIRA_NAMESPACE {

//
// A single breakpoint, watchpoint, or catchpoint
//

/// A single breakpoint, watchpoint, or catchpoint
struct Guard {

    /// The observed address
    u32 addr = 0;

    /// Disabled guards never trigger
    bool enabled = true;

    /// Ignore counter
    isize ignore = 0;

public:

    /// Returns true if the guard hits
    bool eval(u32 addr, Size S = Byte);
};


//
// A collection of breakpoints, watchpoints, or catchpoints
//

/** A collection of breakpoints, watchpoints, or catchpoints
 *
 * Guards are identified either by their index in the list (functions taking
 * an `nr` argument) or by the address they observe (functions ending in `At`).
 */
class Guards {

    friend class Debugger;

protected:

    // Capacity of the guards array
    isize capacity = 1;

    // Array holding all guards
    Guard *guards = new Guard[1];

    // Number of currently stored guards
    isize count = 0;

public:

    /// A copy of the latest match
    std::optional <Guard> hit;


    //
    // Constructing
    //

public:

    /// Destroys the guard list
    virtual ~Guards();


    //
    // Inspecting the guard list
    //

    /// Returns the number of guards in the list
    isize elements() const { return count; }
    /// Returns the guard with the given index (nullptr if there is none)
    Guard *guardNr(isize nr) const;
    /// Returns the guard observing the given address (nullptr if there is none)
    Guard *guardAt(u32 addr) const;

    /// Returns the address observed by the guard with the given index
    std::optional<u32> guardAddr(isize nr) const;


    //
    // Adding or removing guards
    //

    /// Checks if a guard with the given index exists
    bool isSet(isize nr) const { return guardNr(nr) != nullptr; }
    /// Checks if a guard observes the given address
    bool isSetAt(u32 addr) const { return guardAt(addr) != nullptr; }

    /// Adds a guard that observes the given address (the first `ignores` hits are ignored)
    void setAt(u32 addr, isize ignores = 0);
    /// Changes the address observed by the guard with the given index
    void replace(isize nr, u32 addr);

    /// Removes the guard with the given index
    void remove(isize nr);
    /// Removes the guard observing the given address
    void removeAt(u32 addr);
    /// Removes all guards
    void removeAll() { count = 0; setNeedsCheck(false); }


    //
    // Enabling or disabling guards
    //

    /// Checks if the guard with the given index is enabled
    bool isEnabled(isize nr) const;
    /// Checks if the guard observing the given address is enabled
    bool isEnabledAt(u32 addr) const;
    /// Checks if the guard with the given index is disabled
    bool isDisabled(isize nr) const;
    /// Checks if the guard observing the given address is disabled
    bool isDisabledAt(u32 addr) const;

    /// Enables the guard with the given index
    void enable(isize nr) { setEnable(nr, true); }
    /// Enables the guard observing the given address
    void enableAt(u32 addr) { setEnableAt(addr, true); }
    /// Enables all guards
    void enableAll() { setEnableAll(true); }
    /// Disables the guard with the given index
    void disable(isize nr) { setEnable(nr, false); }
    /// Disables the guard observing the given address
    void disableAt(u32 addr) { setEnableAt(addr, false); }
    /// Disables all guards
    void disableAll() { setEnableAll(false); }
    /// Toggles the guard with the given index
    void toggle(isize nr) { setEnable(nr, isDisabled(nr)); }
    /// Toggles the guard observing the given address
    void toggleAt(u32 addr) { setEnableAt(addr, isDisabledAt(addr)); }
    /// Enables or disables the guard with the given index
    void setEnable(isize nr, bool val);
    /// Enables or disables the guard observing the given address
    void setEnableAt(u32 addr, bool val);
    /// Enables or disables all guards
    void setEnableAll(bool val);

    /// Makes the guard with the given index ignore the next `count` hits
    void ignore(isize nr, isize count);


    //
    // Checking guards
    //

    /// Indicates if guard checking is necessary
    virtual void setNeedsCheck(bool value) { };

    /// Evaluates all guards
    bool eval(u32 addr, Size S = Byte);
};

/// The list of breakpoints (execution stops before the instruction at the address is executed)
class Breakpoints : public Guards {

    class Moira &moira;

public:

    Breakpoints(Moira& ref) : moira(ref) { }
    void setNeedsCheck(bool value) override;
};

/// The list of watchpoints (execution stops when the CPU accesses the address)
class Watchpoints : public Guards {

    class Moira &moira;

public:

    Watchpoints(Moira& ref) : moira(ref) { }
    void setNeedsCheck(bool value) override;
};

/// The list of catchpoints (execution stops when the CPU takes the exception with the given vector number)
class Catchpoints : public Guards {

    class Moira &moira;

public:

    Catchpoints(Moira& ref) : moira(ref) { }
    void setNeedsCheck(bool value) override;
};


//
// Software traps
//

/// A software trap
struct SoftwareTrap {

    /// The original instruction that has been replaced by this trap
    u16 instruction;
};

/** A table of software traps
 *
 * A software trap replaces an instruction by a special opcode. The original
 * instruction is remembered and can be recovered with resolve().
 */
struct SoftwareTraps {

    /// The installed traps, indexed by their opcode
    std::unordered_map<u16, SoftwareTrap>traps;

    /// Creates a new software trap for a given instruction
    u16 create(u16 instr);
    /// Creates a software trap with a given opcode for a given instruction
    u16 create(u16 key, u16 instr);

    /// Replaces a software trap by its original opcode
    u16 resolve(u16 instr) const;
};

/** The debugger of the CPU
 *
 * Each CPU owns a debugger (see Moira::debugger). It manages breakpoints,
 * watchpoints, catchpoints and software traps, implements single-stepping and
 * records the most recently executed instructions.
 */
class Debugger {

public:

    /// Reference to the connected CPU
    class Moira &moira;

    /// The breakpoint list
    Breakpoints breakpoints = Breakpoints(moira);

    /// The watchpoint list
    Watchpoints watchpoints = Watchpoints(moira);

    /// The catchpoint list
    Catchpoints catchpoints = Catchpoints(moira);

    /// Software traps
    SoftwareTraps swTraps;

private:

    /* Soft breakpoint for implementing single-stepping. In contrast to a
     * standard (hard) breakpoint, a soft breakpoint is deleted when reached.
     * If a softStop is set, the CPU halts if it matches the program counter
     * (used to implement "step over") or if it contains a negative value (used
     * to implement "step into").
     */
    std::optional <i64> softStop;

    // Buffer storing logged instructions
    static const int logBufferCapacity = 256;
    Registers logBuffer[logBufferCapacity];

    // Logging counter
    isize logCnt = 0;


    //
    // Constructing
    //

public:

    /// Creates a debugger for a CPU
    Debugger(Moira& ref) : moira(ref) { }

    /// Resets the debugger
    void reset();


    //
    // Analyzing instructions
    //

    /// Checks if an opcode belongs to the line A instruction group
    static bool isLineAInstr(u16 opcode) { return (opcode & 0xF000) == 0xA000; }
    /// Checks if an opcode belongs to the line F instruction group
    static bool isLineFInstr(u16 opcode) { return (opcode & 0xF000) == 0xF000; }


    //
    // Providing textual descriptions
    //

    /// Returns a human-readable name for an exception vector
    static std::string vectorName(u8 vector);


    //
    // Working with breakpoints, watchpoints, and catchpoints
    //

    /// Sets a soft breakpoint that will trigger immediately
    void stepInto();

    /// Sets a soft breakpoint to the next instruction
    void stepOver();

    /// Checks whether the soft stop matches the given address
    bool softstopMatches(u32 addr);

    /// Checks whether a breakpoint is set at the given address
    bool breakpointMatches(u32 addr);

    /// Checks whether a watchpoint observes an access of the given size
    bool watchpointMatches(u32 addr, Size S);

    /// Checks whether a catchpoint is set for the given exception vector
    bool catchpointMatches(u32 vectorNr);


    //
    // Working with the log buffer
    //

    /// Turns instruction logging on
    void enableLogging();

    /// Turns instruction logging off
    void disableLogging();

    /// Returns the number of logged instructions
    int loggedInstructions() const;

    /// Logs an instruction
    void logInstruction();

    /// Reads an item from the log buffer (n == 0 returns the most recently recorded entry)
    const Registers &logEntryRel(int n) const;

    /// Reads an item from the log buffer (n == 0 returns the oldest entry)
    const Registers &logEntryAbs(int n) const;

    /// Clears the log buffer
    void clearLog() { logCnt = 0; }


    //
    // Changing state
    //

    /// Continues program execution at the specified address
    void jump(u32 addr);
};

}
