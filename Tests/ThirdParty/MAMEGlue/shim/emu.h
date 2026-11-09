// -----------------------------------------------------------------------------
// A minimal replacement for MAME's "emu.h"
//
// MAME's 68000 core is written as a MAME device. This header provides just
// enough of the device framework (devices, address spaces, delegates, state
// entries) to compile the unmodified core outside of MAME. The memory spaces
// forward all accesses to a bus object supplied by MameCpu (see MameCpu.h).
// -----------------------------------------------------------------------------

#pragma once

#include <bit>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;
using offs_t = u32;

#define ATTR_COLD
#define NAME(...) __VA_ARGS__
#define FUNC(x) &x

enum endianness_t { ENDIANNESS_LITTLE, ENDIANNESS_BIG };
enum { ASSERT_LINE = 1, CLEAR_LINE = 0 };
enum { INPUT_LINE_NMI = 32 };
enum { AS_PROGRAM = 0, AS_DATA = 1, AS_IO = 2, AS_OPCODES = 3 };
enum { STATE_GENPC = -1, STATE_GENPCBASE = -2, STATE_GENSP = -3, STATE_GENFLAGS = -4 };

template <typename T> constexpr T make_bitmask(unsigned bits) { return bits >= sizeof(T) * 8 ? ~T(0) : T((T(1) << bits) - 1); }

inline void logerror(const char *fmt, ...) {}

inline std::string string_format(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof buf, fmt, args);
    va_end(args);
    return buf;
}

namespace util {
    using ::string_format;
    struct disasm_interface { virtual ~disasm_interface() = default; };
}

//
// Devices
//

class machine_config { };
class device_t;
struct device_type_t { };
using device_type = const device_type_t *;
#define DECLARE_DEVICE_TYPE(Type, Class) extern const device_type Type;
#define DEFINE_DEVICE_TYPE(Type, Class, ShortName, FullName) \
    static const device_type_t Type##_object; const device_type Type = &Type##_object;

struct side_effects_guard { };

class running_machine {
public:
    side_effects_guard disable_side_effects() { return { }; }
    bool side_effects_disabled() const { return false; }
};

class address_space;
class address_map;
class address_map_entry {
public:
    template <typename... T> address_map_entry &before_time(T &&...) { return *this; }
    template <typename... T> address_map_entry &after_delay(T &&...) { return *this; }
    template <typename T> address_map_entry &lr8(T &&) { return *this; }
};
class address_map {
public:
    address_map_entry &operator()(offs_t, offs_t) { return entry; }
    address_map_entry entry;
};

struct address_map_constructor {
    address_map_constructor() = default;
    template <typename F, typename O> address_map_constructor(F, O) { }
};

struct address_space_config {
    address_space_config(const char *name, endianness_t endian, int dataWidth, int addrWidth, int shift = 0,
                         address_map_constructor ctor = address_map_constructor())
        : m_name(name), m_addr_width(addrWidth) { }
    const char *m_name;
    int m_addr_width;
};

// The bus the memory spaces are connected to
struct MameBus {
    virtual ~MameBus() = default;
    virtual u16 read(u32 addr, u16 mask, int fc) = 0;
    virtual void write(u32 addr, u16 val, u16 mask, int fc) = 0;
};

template <int AddrWidth, int DataWidth, int AddrShift, endianness_t Endian>
struct memory_access {
    struct specific {
        address_space *space = nullptr;
        inline u16 read_interruptible(offs_t addr, u16 mask = 0xFFFF) const;
        inline void write_interruptible(offs_t addr, u16 data, u16 mask = 0xFFFF) const;
        inline u16 read_word(offs_t addr) const;
    };
};

using memory_specific = memory_access<24, 1, 0, ENDIANNESS_BIG>::specific;

class address_space {
public:
    MameBus *bus = nullptr;
    int fc = 0;

    void specific(memory_specific &s) { s.space = this; }
    u16 read_word(offs_t addr) { return bus->read(addr & 0xFFFFFF, 0xFFFF, fc); }
};

template <int A, int D, int S, endianness_t E>
u16 memory_access<A, D, S, E>::specific::read_interruptible(offs_t addr, u16 mask) const { return space->bus->read(addr & 0xFFFFFF, mask, space->fc); }
template <int A, int D, int S, endianness_t E>
void memory_access<A, D, S, E>::specific::write_interruptible(offs_t addr, u16 data, u16 mask) const { space->bus->write(addr & 0xFFFFFF, data, mask, space->fc); }
template <int A, int D, int S, endianness_t E>
u16 memory_access<A, D, S, E>::specific::read_word(offs_t addr) const { return space->bus->read(addr & 0xFFFFFF, 0xFFFF, space->fc); }

class device_state_entry {
public:
    int index() const { return 0; }
    device_state_entry &callimport() { return *this; }
    device_state_entry &callexport() { return *this; }
    device_state_entry &noshow() { return *this; }
    device_state_entry &formatstr(const char *) { return *this; }
};

class device_memory_interface {
public:
    using space_config_vector = std::vector<std::pair<int, const address_space_config *>>;
    virtual ~device_memory_interface() = default;
    virtual space_config_vector memory_space_config() const { return { }; }
    virtual bool memory_translate(int, int, offs_t &, address_space *&) { return false; }
};

class device_t {
public:
    device_t(const machine_config &, device_type, const char *, device_t *, u32) { }
    virtual ~device_t() = default;
    running_machine &machine() { return theMachine; }
    template <typename... T> void save_item(T &&...) { }

    virtual void device_start() { }
    virtual void device_reset() { }

private:
    running_machine theMachine;
};

// Delegates and callbacks
template <typename Sig> class devcb_delegate {
public:
    devcb_delegate(device_t &) { }
    void resolve() { }
    bool isnull() const { return !fn; }
    template <typename F> void set(F &&f) { fn = std::forward<F>(f); }
    devcb_delegate &bind() { return *this; }
    template <typename... A> auto operator()(A &&... args) { if constexpr (std::is_void_v<decltype(fn(std::forward<A>(args)...))>) { if (fn) fn(std::forward<A>(args)...); } else return fn ? fn(std::forward<A>(args)...) : decltype(fn(std::forward<A>(args)...))(); }
    std::function<Sig> fn;
};
using write32sm_delegate = devcb_delegate<void(u32, u32)>;
using write8sm_delegate = devcb_delegate<void(offs_t, u8)>;
using write_line_delegate = devcb_delegate<void(int)>;
using devcb_write_line = devcb_delegate<void(int)>;

class cpu_device : public device_t, public device_memory_interface {
public:
    cpu_device(const machine_config &mc, device_type type, const char *tag, device_t *owner, u32 clock) : device_t(mc, type, tag, owner, clock)
    {
        static const int fcs[] = { 5, 5, 0, 6 };
        for (int i = 0; i < 8; i++) { spaces[i].fc = i; }
        spaces[AS_PROGRAM].fc = 5;
        spaces[AS_OPCODES].fc = 6;
        spaces[4].fc = 7;      // CPU space
        spaces[5].fc = 1;      // User program (data)
        spaces[6].fc = 2;      // User opcodes
    }

    virtual std::unique_ptr<util::disasm_interface> create_disassembler() { return nullptr; }
    virtual bool cpu_is_interruptible() const { return false; }
    virtual u32 execute_min_cycles() const noexcept { return 1; }
    virtual u32 execute_max_cycles() const noexcept { return 1; }
    virtual void execute_run() { }
    virtual void execute_set_input(int, int) { }
    virtual bool execute_input_edge_triggered(int) const noexcept { return false; }
    virtual void state_import(const device_state_entry &) { }
    virtual void state_string_export(const device_state_entry &, std::string &) const { }
    virtual space_config_vector memory_logical_space_config() const { return { }; }

    bool has_configured_map(int) const { return false; }
    bool has_space(int) const { return true; }
    address_space &space(int id) { return spaces[id]; }

    template <typename T> device_state_entry &state_add(int, const std::string &, T &) { return entry; }
    void set_icountptr(int &) { }
    bool debugger_enabled() const { return bool(instructionHook); }
    void debugger_instruction_hook(offs_t pc) { if (instructionHook) instructionHook(pc); }
    void debugger_exception_hook(int vector) { if (exceptionHook) exceptionHook(vector); }
    void debugger_wait_hook() { }
    int standard_irq_callback(int, offs_t) { return 0; }
    bool access_to_be_redone() { return false; }

    // Connects all memory spaces to a bus
    void connect(MameBus *bus) { for (auto &s : spaces) s.bus = bus; }

    std::function<void(offs_t)> instructionHook;
    std::function<void(int)> exceptionHook;

private:
    address_space spaces[8];
    device_state_entry entry;
};
