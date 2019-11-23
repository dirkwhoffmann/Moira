// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

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

template <class T> void
Moira::_dasmIllegal(uint16_t opcode, char *str)
{
    T(str) << "ILLEGAL\0";
}

void
Moira::dasmIllegal(uint16_t opcode, char *str, bool hex)
{
    hex ?
    _dasmIllegal<HexWriter>(opcode, str) :
    _dasmIllegal<DecWriter>(opcode, str) ;
}

template <class T, Instr I, Size S> void
Moira::_dasmRegShift(uint16_t opcode, char *str)
{
    T(str)
    << (const char *)"REG_SHIFT "
    << "size = " << (uint8_t)42
    << "mode = " << (uint16_t)13
    << "\0";
}

template<Instr I, Size S>
void Moira::dasmRegShift(uint16_t opcode, char *str, bool hex) {
    
    hex ?
    _dasmRegShift<HexWriter,I,S>(opcode, str) :
    _dasmRegShift<DecWriter,I,S>(opcode, str) ;
}

template <class T, Instr I, Size S> void
Moira::_dasmImmShift(uint16_t opcode, char *str)
{
    T(str)
    << (const char *)"IMM_SHIFT "
    << "size = " << (uint8_t)42
    << "mode = " << (uint16_t)13
    << "\0";
}

template<Instr I, Size S>
void Moira::dasmImmShift(uint16_t opcode, char *str, bool hex) {
    
    hex ?
    _dasmImmShift<HexWriter,I,S>(opcode, str) :
    _dasmImmShift<DecWriter,I,S>(opcode, str) ;
}

template <class T, Instr I, Mode M> void
Moira::_dasmEaShift(uint16_t opcode, char *str)
{
    T(str)
    << (const char *)"EA_SHIFT "
    << "size = " << (uint8_t)42
    << "mode = " << (uint16_t)13
    << "\0";
}

template<Instr I, Mode M>
void Moira::dasmEaShift(uint16_t opcode, char *str, bool hex) {
    
    hex ?
    _dasmEaShift<HexWriter,I,M>(opcode, str) :
    _dasmEaShift<DecWriter,I,M>(opcode, str) ;
}

template<Mode M>
void Moira::dasmLea(uint16_t opcode, char *str, bool hex) {
    
    HexWriter(str)
    << (const char *)"EA_SHIFT "
    << "size = " << (uint8_t)42
    << "mode = " << (uint16_t)13
    << "\0";
}
