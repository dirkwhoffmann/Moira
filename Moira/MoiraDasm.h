// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <string.h>

//
// Printing numbers in decimal and hexidecimal format
//

template<int digits> static void sprintd(char *s, uint64_t value)
{
    for (int i = digits - 1; i >= 0; i--) {
        uint8_t digit = value % 10;
        s[i] = '0' + digit;
        value /= 10;
    }
    s[digits] = 0;
}

template<int digits> static void sprintx(char *s, uint64_t value)
{
    for (int i = digits - 1; i >= 0; i--) {
        uint8_t digit = value % 16;
        s[i] = (digit <= 9) ? ('0' + digit) : ('A' + digit - 10);
        value /= 16;
    }
    s[digits] = 0;
}

template<int digits> static void sprintb(char *s, uint64_t value)
{
    for (int i = digits - 1; i >= 0; i--) {
        s[i] = (value & 0x01) ? '1' : '0';
        value >>= 1;
    }
    s[digits] = 0;
}

//
// Assembling strings
//

class HexWriter
{
public:

    char *ptr;

    HexWriter(char *p) : ptr(p) { }

    HexWriter& operator<<(const char *str)
     {
         char c = *str; while (c) { *ptr++ = c; c = *++str; }
         return *this;
     }

    HexWriter& operator<<(uint8_t value)
    {
        sprintx<2>(ptr, value);
        return *this;
    }

    HexWriter& operator<<(uint16_t value)
    {
        sprintx<4>(ptr, value);
        return *this;
    }

    HexWriter& operator<<(uint32_t value)
    {
        sprintx<8>(ptr, value);
        return *this;
    }
};

class DecWriter
{
public:

    char *ptr;

    DecWriter(char *p) : ptr(p) { }

    DecWriter& operator<<(const char *str)
      {
          char c = *str; while (c) { *ptr++ = c; c = *++str; }
          return *this;
      }

    DecWriter& operator<<(uint8_t value)
    {
        sprintd<3>(ptr, value);
        return *this;
    }

    DecWriter& operator<<(uint16_t value)
    {
        sprintd<5>(ptr, value);
        return *this;
    }

    DecWriter& operator<<(uint32_t value)
    {
        sprintd<10>(ptr, value);
        return *this;
    }
};

//
// Disassembler handlers for all instructions
//

void Moira::dasmIllegal(uint16_t opcode) { }

template <class T, int size, int mode>
void Moira::dasmRegShift(uint16_t opcode) {

    T(str)
    << (const char *)"REG_SHIFT "
    << "size = " << (uint8_t)42
    << "mode = " << (uint16_t)13
    << "\0";
}

template<int size, int mode> void Moira::dasmRegShift(uint16_t opcode) {

    hex ?
    dasmRegShift<HexWriter, size, mode>(opcode) :
    dasmRegShift<DecWriter, size, mode>(opcode) ;
}

template<int size, int mode> void Moira::dasmImmShift(uint16_t opcode) {
    printf("***** dasmImmShift<%d,%d>(%d)\n", size, mode, opcode);
}
