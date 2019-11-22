// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

enum : uint8_t { Asl, Asr, Lsl, Lsr, Rol, Ror, Roxl, Roxr };
enum : uint8_t {Byte, Word, Long };

class Moira {


    // Jump table storing all instruction handlers
    void (Moira::*exec[65536])(uint16_t);

    // Jump table storing all disassebler handlers
    void (Moira::*dasm[65536])(uint16_t);

    // Jump table storing all time information handlers
    int (Moira::*sync[65536])(uint16_t, int);

    // Disassembler
    bool hex = true;
    char str[256];


    // Instruction handlers

    void execIllegal(uint16_t opcode);
    void dasmIllegal(uint16_t opcode);
    int syncIllegal(uint16_t opcode, int i);

    template <class T, int size, int mode> void dasmRegShift(uint16_t opcode);

    template<int size, int mode> void execRegShift(uint16_t opcode);
    template<int size, int mode> void dasmRegShift(uint16_t opcode);
    template<int size, int mode> int syncRegShift(uint16_t opcode, int i);

    template<int size, int mode> void execImmShift(uint16_t opcode);
    template<int size, int mode> void dasmImmShift(uint16_t opcode);
    template<int size, int mode> int syncImmShift(uint16_t opcode, int i);


public:
    
    Moira();

    void init();
    void process(uint16_t reg_ird);
};
