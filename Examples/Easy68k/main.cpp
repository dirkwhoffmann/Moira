// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

/* A console runner for Easy68K programs.
 *
 * Easy68K is a popular 68000 development environment for teaching. Programs
 * written for it talk to their environment through TRAP #15. The task number is
 * passed in D0, and the tasks are described here:
 *
 *     http://www.easy68k.com/QuickStart/TrapTasks.htm
 *
 * This example takes an assembled program in Motorola S-record format and runs
 * it on a Moira CPU. TRAP #15 is intercepted by the host program, which
 * implements the console related tasks (text input and output, number
 * conversion, time, cursor control, font colors, halting).
 *
 * Usage:  easy68k [options]
 *         easy68k [options] --file program.s68
 *
 * Without a file, the tool runs a built-in Hello World program. It prints the
 * source code of the program, then loads the assembled S-records from the
 * program text below and executes them.
 *
 * Program output goes to stdout. Everything else (trace, messages) goes to
 * stderr, so that the output of the emulated program can be piped.
 */

#include "Moira.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

using namespace moira;


//
// S-record loader
//

/* The Motorola S-record format
 *
 * An S-record file is a text file in which every line is one record. A record
 * starts with the letter 'S', followed by a digit that denotes the record type.
 * The rest of the line consists of hexadecimal digits, two per byte:
 *
 *     S  t  cc  aaaa...  dddd...  ss
 *
 *     t       Record type (one digit)
 *     cc      Byte count: the number of bytes that follow (address, data, and
 *             checksum)
 *     aaaa    Address, big endian. The type determines its length: 2 bytes for
 *             S0, S1, S9, 3 bytes for S2, S8, and 4 bytes for S3, S7.
 *     dddd    Data bytes (S0, S1, S2, S3 only)
 *     ss      Checksum: the one's complement of the low byte of the sum of the
 *             count, address, and data bytes. Adding up all bytes after the
 *             'S' and the type digit, including the checksum, yields $FF.
 *
 * The record types are:
 *
 *     S0      Header. Carries a name or comment, but no program data.
 *     S1      Data with a 16-bit address
 *     S2      Data with a 24-bit address
 *     S3      Data with a 32-bit address
 *     S5, S6  Number of data records in the file (not needed here)
 *     S7      Start address (entry point), 32 bit. Terminates S3 files.
 *     S8      Start address (entry point), 24 bit. Terminates S2 files.
 *     S9      Start address (entry point), 16 bit. Terminates S1 files.
 *
 * Example (the Hello World program below):
 *
 *     S11D200043FA000A700D4E4F70094E4F48656C6C6F2C20776F726C642100C2
 *
 *     S1      A data record with a 16-bit address
 *     1D      29 bytes follow: 2 address bytes, 26 data bytes, 1 checksum byte
 *     2000    The data goes to address $2000
 *     43FA... The 26 bytes of program and data (the opcodes of LEA, MOVEQ,
 *             TRAP, ... followed by the ASCII text of the message)
 *     C2      Checksum
 *
 * The address in the final S9 record is the entry point of the program. An
 * assembler that doesn't know it (the program has no end label) writes 0. Such
 * an address is ignored here, and the program starts at the lowest address that
 * has been loaded.
 */

struct Segment {

    u32 addr;                   // Address of the first byte
    std::vector<u8> data;       // The bytes
};

struct Image {

    std::vector<Segment> segments;
    bool hasStart = false;
    u32 start = 0;

    u32 lowest() const
    {
        u32 result = 0xFFFFFFFF;
        for (auto &s : segments) result = std::min(result, s.addr);
        return result;
    }

    bool contains(u32 addr) const
    {
        for (auto &s : segments) if (addr >= s.addr && addr < s.addr + s.data.size()) return true;
        return false;
    }

    // Appends a block of data. Contiguous blocks are merged into one segment.
    void add(u32 addr, const std::vector<u8> &bytes)
    {
        if (!segments.empty()) {
            auto &last = segments.back();
            if (last.addr + last.data.size() == addr) {
                last.data.insert(last.data.end(), bytes.begin(), bytes.end());
                return;
            }
        }
        segments.push_back({ addr, bytes });
    }
};

static int hexDigit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Parses S-records. The records are separated by white space. The source
// describes the origin of the text in error messages.
static Image parseSRecords(const std::string &text, const std::string &source)
{
    Image image;
    std::istringstream stream(text);
    std::string rec;
    int count = 0;

    auto fail = [&](const std::string &msg) {
        throw std::runtime_error(source + ", record " + std::to_string(count) + ": " + msg);
    };

    while (stream >> rec) {

        count++;

        if (rec[0] != 'S' || rec.size() < 4) fail("Not an S-record");

        int type = rec[1] - '0';
        if (type < 0 || type > 9) fail("Invalid record type");

        // Convert the remaining characters to bytes
        std::vector<u8> bytes;
        if ((rec.size() - 2) % 2) fail("Odd number of hex digits");
        for (size_t i = 2; i < rec.size(); i += 2) {
            int hi = hexDigit(rec[i]), lo = hexDigit(rec[i + 1]);
            if (hi < 0 || lo < 0) fail("Invalid hex digit");
            bytes.push_back(u8(hi << 4 | lo));
        }

        // The count covers the address, the data, and the checksum
        if (bytes[0] != bytes.size() - 1) fail("Wrong byte count");

        u8 sum = 0;
        for (auto b : bytes) sum += b;
        if (sum != 0xFF) fail("Checksum error");

        // Determine the number of address bytes
        int alen = 0;
        switch (type) {
            case 1: case 9: alen = 2; break;
            case 2: case 8: alen = 3; break;
            case 3: case 7: alen = 4; break;
            default: continue;      // Header and counter records
        }
        if (bytes.size() < size_t(2 + alen)) fail("Record too short");

        u32 addr = 0;
        for (int i = 0; i < alen; i++) addr = addr << 8 | bytes[1 + i];

        if (type <= 3) {
            image.add(addr, std::vector<u8>(bytes.begin() + 1 + alen, bytes.end() - 1));
        } else {
            image.hasStart = true;
            image.start = addr;
        }
    }

    if (image.segments.empty()) throw std::runtime_error(source + ": No program data found");
    return image;
}

static Image loadSRecords(const std::string &path)
{
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open " + path);

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseSRecords(buffer.str(), path);
}


//
// The computer
//

static const u32 memSize   = 0x1000000;     // 16 MB, as in Easy68K
static const u32 memMask   = memSize - 1;
static const u32 stackAddr = 0x1000000;     // Initial stack pointer, as in Easy68K

struct Options {

    CPUModel model = CPUModel::M68000;
    bool trace     = false;                 // Show a trace
    bool hasStart  = false;                 // Overrides the start address
    u32 start      = 0;
    u64 maxCycles  = 0;                     // 0 = unlimited
};

class Easy68k : public Moira {

    // The computer's memory
    mutable std::vector<u8> ram = std::vector<u8>(memSize);

    // State of the TRAP #15 environment
    bool color = false;                     // Output is a terminal that understands ANSI codes
    bool styled = false;                    // Font style is not the default
    bool warned[256] = { };                 // Unsupported tasks that have been reported

public:

    // Result of a run
    bool stopped = false;                   // Execution should stop
    int exitCode = 0;

    Easy68k()
    {
        color = isatty(fileno(stdout));
    }

    ~Easy68k()
    {
        if (styled) fputs("\033[0m", stdout);
        fflush(stdout);
    }

    using Moira::willExecute;


    //
    // Moira: Memory interface
    //

    u8 read8(u32 addr) const override { return ram[addr & memMask]; }
    u16 read16(u32 addr) const override { return u16(read8(addr) << 8 | read8(addr + 1)); }
    u32 read32(u32 addr) const override { return u32(read16(addr) << 16 | read16(addr + 2)); }

    void write8(u32 addr, u8 val) const override { ram[addr & memMask] = val; }
    void write16(u32 addr, u16 val) const override { write8(addr, u8(val >> 8)); write8(addr + 1, u8(val)); }
    void write32(u32 addr, u32 val) const override { write16(addr, u16(val >> 16)); write16(addr + 2, u16(val)); }

    // Easy68K does not know about exception handlers. If a program causes an
    // exception for which no handler is installed, we stop instead of running
    // into the code at address 0.
    void willExecute(M68kException exc, u16 vector) override
    {
        if (read32(u32(vector) * 4) == 0) {
            fprintf(stderr, "\nException %d at $%06X (no handler installed)\n", vector, getPC0());
            stopped = true;
            exitCode = 1;
        }
    }


    //
    // Loading and starting
    //

    void load(const Image &image, u32 start)
    {
        for (auto &s : image.segments)
            for (size_t i = 0; i < s.data.size(); i++) ram[(s.addr + u32(i)) & memMask] = s.data[i];

        // Reset the CPU. This loads the initial SP and PC from the vector table.
        // The table is temporarily written, because the program might use the
        // memory at address 0.
        u8 saved[8];
        for (int i = 0; i < 8; i++) saved[i] = ram[i];
        for (int i = 0; i < 4; i++) {
            ram[i] = u8(stackAddr >> (24 - 8 * i));
            ram[4 + i] = u8(start >> (24 - 8 * i));
        }
        reset();
        for (int i = 0; i < 8; i++) ram[i] = saved[i];
    }


    //
    // Execution
    //

    void trace()
    {
        char instr[128];
        (void)disassemble(instr, getPC0());

        fprintf(stderr, "%06X %s\n", getPC0(), instr);
        
        /*
        u16 sr = getSR();
        char flags[6];
        for (int i = 0; i < 5; i++) flags[i] = (sr >> (4 - i)) & 1 ? "XNZVC"[i] : '-';
        flags[5] = 0;

        fprintf(stderr, "%06X %-26s %s", getPC0(), instr, flags);
        for (int i = 0; i < 8; i++) fprintf(stderr, " %08X", getD(i));
        fprintf(stderr, " |");
        for (int i = 0; i < 8; i++) fprintf(stderr, " %08X", getA(i));
        fprintf(stderr, "\n");
        */
    }

    // Runs the program until it stops
    void run(const Options &opt)
    {
        while (!stopped && !isHalted()) {

            u32 pc = getPC0();
            u16 op = read16(pc);

            if (opt.trace) trace();

            // TRAP #15 is handled by the host
            if (op == 0x4E4F) {

                trap15();

                // Continue with the next instruction (this refills the prefetch queue)
                if (!stopped) debugger.jump(pc + 2);
                continue;
            }

            // STOP waits for an interrupt that never comes
            if (op == 0x4E72) {
                fprintf(stderr, "\nSTOP instruction at $%06X\n", pc);
                break;
            }

            execute();

            if (opt.maxCycles && u64(getClock()) >= opt.maxCycles) {
                fprintf(stderr, "\nCycle limit reached at $%06X\n", getPC0());
                exitCode = 1;
                break;
            }
        }

        if (isHalted()) {
            fprintf(stderr, "\nThe CPU halted at $%06X\n", getPC0());
            exitCode = 1;
        }
    }


    //
    // TRAP #15
    //

    // Writes a character to the console. Control characters are interpreted
    // as described in the Easy68K documentation.
    void put(u8 c)
    {
        switch (c) {

            case 0x09: fputs("     ", stdout); break;             // HT: 5 spaces
            case 0x0B: fputs("\n\n\n\n", stdout); break;          // VT: 4 lines
            default:   fputc(c, stdout);
        }
    }

    void put(const std::string &s) { for (unsigned char c : s) put(c); }

    void putNewline() { fputc('\n', stdout); }

    // Reads a zero-terminated string from memory
    std::string cstring(u32 addr) const
    {
        std::string s;
        for (u32 i = 0; i < memSize; i++) {
            u8 c = read8(addr + i);
            if (c == 0) break;
            s += char(c);
        }
        return s;
    }

    // Reads a string of a given length from memory
    std::string string(u32 addr, u32 len) const
    {
        std::string s;
        for (u32 i = 0; i < len; i++) s += char(read8(addr + i));
        return s;
    }

    static std::string toBase(u32 value, int base)
    {
        if (value == 0) return "0";
        std::string s;
        while (value) { s.insert(s.begin(), "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[value % base]); value /= base; }
        return s;
    }

    // Reads a line from the keyboard. Returns false at the end of the input.
    bool readLine(std::string &line)
    {
        fflush(stdout);
        if (std::getline(std::cin, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return true;
        }
        fprintf(stderr, "\nEnd of input at $%06X\n", getPC0());
        stopped = true;
        exitCode = 1;
        return false;
    }

    // Reads a number from the keyboard (task 4)
    void readNumber()
    {
        std::string line;
        if (!readLine(line)) return;
        setD(1, u32(strtol(line.c_str(), nullptr, 10)));
    }

    void warnUnsupported(u8 task, const char *what)
    {
        if (warned[task]) return;
        warned[task] = true;
        fprintf(stderr, "Warning: TRAP #15 task %d (%s) is not supported\n", task, what);
    }

    void trap15()
    {
        u8 task = u8(getD(0));
        u32 a1 = getA(1);
        u32 d1 = getD(1);
        u32 d2 = getD(2);

        switch (task) {

            case 0:     // Display string with length D1.W, with CR/LF
                put(string(a1, std::min<u32>(d1 & 0xFFFF, 255))); putNewline();
                break;

            case 1:     // Display string with length D1.W, without CR/LF
                put(string(a1, std::min<u32>(d1 & 0xFFFF, 255)));
                break;

            case 2: {   // Read a string from the keyboard
                std::string line;
                if (!readLine(line)) break;
                if (line.size() > 80) line.resize(80);
                for (size_t i = 0; i < line.size(); i++) write8(a1 + u32(i), u8(line[i]));
                write8(a1 + u32(line.size()), 0);
                setD(1, (d1 & 0xFFFF0000) | u32(line.size()));
                break;
            }
            case 3:     // Display D1.L as signed decimal
                put(std::to_string(i32(d1)));
                break;

            case 4:     // Read a number from the keyboard
                readNumber();
                break;

            case 5: {   // Read a character from the keyboard
                fflush(stdout);
                int c = std::cin.get();
                if (c == EOF) {
                    fprintf(stderr, "\nEnd of input at $%06X\n", getPC0());
                    stopped = true;
                    exitCode = 1;
                    break;
                }
                setD(1, (d1 & 0xFFFFFF00) | u32(c & 0xFF));
                break;
            }
            case 6:     // Display the character in D1.B
                put(u8(d1));
                break;

            case 7:     // Check if input is pending
                setD(1, (d1 & 0xFFFFFF00) | (std::cin.rdbuf()->in_avail() > 0 ? 1 : 0));
                break;

            case 8: {   // Time in hundredths of a second since midnight
                auto now = std::chrono::system_clock::now();
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
                std::time_t t = std::chrono::system_clock::to_time_t(now);
                std::tm tm = *std::localtime(&t);
                setD(1, u32(((tm.tm_hour * 60 + tm.tm_min) * 60 + tm.tm_sec) * 100 + ms / 10));
                break;
            }
            case 9:     // Terminate the program
                stopped = true;
                break;

            case 11: {  // Position the cursor, or clear the screen
                if ((d1 & 0xFFFF) == 0xFF00) {
                    if (color) fputs("\033[2J\033[H", stdout);
                } else {
                    u32 col = (d1 >> 8) & 0xFF, row = d1 & 0xFF;
                    if (col < 80 && row < 32 && color) printf("\033[%u;%uH", row + 1, col + 1);
                }
                break;
            }
            case 12:    // Keyboard echo (the terminal controls the echo)
            case 16:    // Display properties (input prompt, line feed after input)
                break;

            case 13:    // Display zero-terminated string, with CR/LF
                put(cstring(a1)); putNewline();
                break;

            case 14:    // Display zero-terminated string, without CR/LF
                put(cstring(a1));
                break;

            case 15:    // Display D1.L (unsigned) in the base D2.B
                if ((d2 & 0xFF) >= 2 && (d2 & 0xFF) <= 36) put(toBase(d1, int(d2 & 0xFF)));
                break;

            case 17:    // Display a string, then D1.L as signed decimal
                put(cstring(a1)); put(std::to_string(i32(d1)));
                break;

            case 18:    // Display a string, then read a number
                put(cstring(a1)); readNumber();
                break;

            case 19:    // Key states (no keyboard scan codes on a console)
                warnUnsupported(task, "key states");
                setD(1, 0);
                break;

            case 20: {  // Display D1.L as signed decimal in a field of D2.B columns
                std::string s = std::to_string(i32(d1));
                size_t width = d2 & 0xFF;
                if (s.size() < width) s.insert(0, width - s.size(), ' ');
                put(s);
                break;
            }
            case 21: {  // Set the font color and style
                if (color) {
                    printf("\033[0");
                    if (d2 & 1) printf(";1");
                    if (d2 & 2) printf(";3");
                    if (d2 & 4) printf(";4");
                    if (d2 & 8) printf(";9");
                    printf(";38;2;%u;%u;%um", d1 & 0xFF, (d1 >> 8) & 0xFF, (d1 >> 16) & 0xFF);
                    styled = true;
                }
                break;
            }
            case 10:    warnUnsupported(task, "printer output"); break;
            case 22:    warnUnsupported(task, "read character from screen"); setD(1, d1 & 0xFFFFFF00); break;
            default:    warnUnsupported(task, "unknown task"); break;
        }
    }
};


//
// Built-in example
//

// The source code of the example (Motorola syntax, assembled with vasm)
static const char *helloSource =
"        org     $2000\n"
"\n"
"        lea     msg(pc),a1\n"
"        moveq   #13,d0              ; Display string with CR/LF\n"
"        trap    #15\n"
"        moveq   #9,d0               ; Halt\n"
"        trap    #15\n"
"\n"
"msg:    dc.b    'Hello, world!',0\n";

// The assembled program: vasmm68k_mot -m68000 -Fsrec -s19 hello.s
static const char *helloRecords =
"S00F00006F7267303030313A32303030EB\n"
"S11D200043FA000A700D4E4F70094E4F48656C6C6F2C20776F726C642100C2\n"
"S9032000DC\n";


//
// Main
//

static void usage()
{
    fprintf(stderr,
            "Usage: easy68k [options] [--file program.s68]\n\n"
            "Runs an Easy68K program (Motorola S-record format) on a Moira CPU. TRAP #15\n"
            "is emulated on the console. Program output goes to stdout, everything else\n"
            "to stderr. Without --file, a built-in Hello World program is executed.\n\n"
            "Options:\n"
            "  -f, --file <name>     Run the program in a file of S-records\n"
            "  -t, --trace           Show every executed instruction\n"
            "  -m, --model <model>   CPU model: 68000 (default), 68010, or 68020\n"
            "  -s, --start <addr>    Start address in hex (default: S9 record)\n"
            "  -c, --max-cycles <n>  Stop after n CPU cycles\n"
            "  -h, --help            Show this help\n");
}

int main(int argc, const char *argv[])
{
    Options opt;
    std::string path;           // Name of the program file (--file)

    // Do not synchronize with stdio, so that in_avail() reports buffered input
    std::ios::sync_with_stdio(false);

    for (int i = 1; i < argc; i++) {

        std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) { fprintf(stderr, "Missing value for %s\n", arg.c_str()); exit(2); }
            return argv[++i];
        };

        if (arg == "-h" || arg == "--help") { usage(); return 0; }
        else if (arg == "-f" || arg == "--file") path = value();
        else if (arg == "-t" || arg == "--trace") opt.trace = true;
        else if (arg == "-m" || arg == "--model") {
            std::string m = value();
            if (m == "68000") opt.model = CPUModel::M68000;
            else if (m == "68010") opt.model = CPUModel::M68010;
            else if (m == "68020") opt.model = CPUModel::M68020;
            else { fprintf(stderr, "Unknown model %s\n", m.c_str()); return 2; }
        }
        else if (arg == "-s" || arg == "--start") { opt.hasStart = true; opt.start = u32(strtoul(value().c_str(), nullptr, 16)); }
        else if (arg == "-c" || arg == "--max-cycles") opt.maxCycles = strtoull(value().c_str(), nullptr, 10);
        else if (!arg.empty() && arg[0] == '-') { fprintf(stderr, "Unknown option %s\n\n", arg.c_str()); usage(); return 2; }
        else { fprintf(stderr, "Unexpected argument %s\n\n", arg.c_str()); usage(); return 2; }
    }

    try {

        Image image;
        if (path.empty()) {
            fprintf(stderr, "No program file given. Running the built-in example:\n\n%s\n", helloSource);
            image = parseSRecords(helloRecords, "built-in example");
        } else {
            image = loadSRecords(path);
        }

        // Determine the start address. The S9 record of an assembler that
        // knows no entry point contains 0, which is not part of the program.
        u32 start = image.lowest();
        if (opt.hasStart) {
            start = opt.start;
        } else if (image.hasStart && image.contains(image.start)) {
            start = image.start;
        } else {
            if (opt.trace) fprintf(stderr, "No usable start address in the program. Starting at $%06X.\n\n", start);
        }

        Easy68k cpu;
        cpu.setModel(opt.model);
        cpu.load(image, start);

        cpu.run(opt);

        return cpu.exitCode;

    } catch (const std::exception &e) {

        fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
}
