// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace sst {

/* Reader for the binary test files of the SingleStepTests 68000 suite
 * (the v1 folder). The files are read directly. No conversion step is needed.
 *
 * File layout (all values are little endian, every block starts with its size
 * in bytes, including the two header words, followed by a magic number):
 *
 *   File:         magic 0x1A3F5D71, number of tests, tests
 *   Test:         size, magic 0xABC12367, name, initial state, final state, transactions
 *   Name:         size, magic 0x89ABCDEF, string length, string
 *   State:        size, magic 0x01234567, D0-D7, A0-A6, USP, SSP, SR, PC,
 *                 two prefetch words, number of RAM entries, (address, word) pairs
 *   Transactions: size, magic 0x456789AB, number of cycles, number of transactions, ...
 *
 *   A transaction record is: type (1 byte), duration in cycles, and for all types
 *   but IDLE: function code, address bus, data bus, UDS, LDS (all 32 bit).
 */

struct State {

    uint32_t d[8] = { };
    uint32_t a[7] = { };
    uint32_t usp = 0;
    uint32_t ssp = 0;
    uint32_t sr = 0;
    uint32_t pc = 0;                                // Address of the next prefetch
    uint32_t prefetch[2] = { };                     // Instruction queue (IRD, IRC)
    std::vector<std::pair<uint32_t, uint8_t>> ram;  // Bytes that are set or checked
};

/* A bus transaction. Idle periods are stored as transactions of type IDLE. */
struct Bus {

    enum Type : uint8_t { IDLE = 0, WRITE = 1, READ = 2, TAS = 3, READ_AE = 4, WRITE_AE = 5 };

    Type type = IDLE;
    uint32_t cycles = 0;                            // Duration
    uint32_t fc = 0;                                // Function code pins
    uint32_t addr = 0;                              // Address bus (A0 is not part of it)
    uint32_t data = 0;                              // Data bus
    bool uds = false, lds = false;                  // Data strobes
};

struct Test {

    std::string name;
    State initial;
    State final;
    uint32_t cycles = 0;                            // Total number of CPU cycles
    std::vector<Bus> bus;                           // Bus transactions
};

class File {

    std::vector<uint8_t> data;
    size_t pos = 0;
    uint32_t remaining = 0;

public:

    // Loads a file. Returns false on failure.
    bool open(const std::string &path);

    // Returns the number of tests that have not been read yet
    uint32_t tests() const { return remaining; }

    // Reads the next test. Returns false if there are no more tests or if the file is damaged.
    bool next(Test &test);
};

}
