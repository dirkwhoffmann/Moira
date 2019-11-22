// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <assert.h>

#include "Moira.h"
#include "MoiraLogic.tpp"
#include "MoiraExec.tpp"
#include "MoiraDasm.tpp"
#include "MoiraSync.tpp"

uint16_t parse(const char *s, uint16_t sum = 0)
{
    return
    *s == ' ' ? parse(s + 1, sum) :
    *s == '-' ? parse(s + 1, sum << 1) :
    *s == '0' ? parse(s + 1, sum << 1) :
    *s == '1' ? parse(s + 1, (sum << 1) + 1) : sum;
}

void Moira::execIllegal(uint16_t opcode) { }
int Moira::syncIllegal(uint16_t opcode, int i) { return 0; }

template<int size, int mode> int Moira::syncRegShift(uint16_t opcode, int i) {
    printf("***** syncRegShift<%d,%d>(%d,%d)\n", size, mode, opcode, i);
    return i;
}

template<int size, int mode> int Moira::syncImmShift(uint16_t opcode, int i) {
    printf("***** syncImmShift<%d,%d>(%d,%d)\n", size, mode, opcode, i);
    return i;
}

Moira::Moira()
{
    init();
}

#define __ ,
#define bind(id, name) { \
printf("id = %X\n", id); \
assert(exec[id] == &Moira::execIllegal); \
assert(dasm[id] == &Moira::dasmIllegal); \
assert(sync[id] == &Moira::syncIllegal); \
exec[id] = &Moira::exec##name; \
dasm[id] = &Moira::dasm##name; \
sync[id] = &Moira::sync##name; \
}

void
Moira::init()
{
    uint16_t opcode;

    // Start with clean tables
    for (int i = 0; i < 65536; i++) {
        exec[i] = &Moira::execIllegal;
        dasm[i] = &Moira::dasmIllegal;
        sync[i] = &Moira::syncIllegal;
    }

    // ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR
    //
    // Modes: (1)   Dx,Dy       8,16,32
    //        (2)   #<data>,Dy  8,16,32
    //        (3)   <ea>          16

    // (1)
    for (int dx = 0; dx < 8; dx++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse("1110 ---1 --10 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ ASL>);
            bind(opcode | 1 << 6, RegShift<Word __ ASL>);
            bind(opcode | 2 << 6, RegShift<Long __ ASL>);

            opcode = parse("1110 ---0 --10 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ ASR>);
            bind(opcode | 1 << 6, RegShift<Word __ ASR>);
            bind(opcode | 2 << 6, RegShift<Long __ ASR>);

            opcode = parse("1110 ---1 --10 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ LSL>);
            bind(opcode | 1 << 6, RegShift<Word __ LSL>);
            bind(opcode | 2 << 6, RegShift<Long __ LSL>);

            opcode = parse("1110 ---0 --10 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ LSR>);
            bind(opcode | 1 << 6, RegShift<Word __ LSR>);
            bind(opcode | 2 << 6, RegShift<Long __ LSR>);

            opcode = parse("1110 ---1 --11 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ ROL>);
            bind(opcode | 1 << 6, RegShift<Word __ ROL>);
            bind(opcode | 2 << 6, RegShift<Long __ ROL>);

            opcode = parse("1110 ---0 --11 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ ROR>);
            bind(opcode | 1 << 6, RegShift<Word __ ROR>);
            bind(opcode | 2 << 6, RegShift<Long __ ROR>);

            opcode = parse("1110 ---1 --11 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ ROXL>);
            bind(opcode | 1 << 6, RegShift<Word __ ROXL>);
            bind(opcode | 2 << 6, RegShift<Long __ ROXL>);

            opcode = parse("1110 ---0 --11 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ ROXR>);
            bind(opcode | 1 << 6, RegShift<Word __ ROXR>);
            bind(opcode | 2 << 6, RegShift<Long __ ROXR>);
        }
    }

    // (2)
    for (int data = 0; data < 8; data++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse("1110 ---1 --00 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ ASL>);
            bind(opcode | 1 << 6, ImmShift<Word __ ASL>);
            bind(opcode | 2 << 6, ImmShift<Long __ ASL>);

            opcode = parse("1110 ---0 --00 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ ASR>);
            bind(opcode | 1 << 6, ImmShift<Word __ ASR>);
            bind(opcode | 2 << 6, ImmShift<Long __ ASR>);

            opcode = parse("1110 ---1 --00 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ LSL>);
            bind(opcode | 1 << 6, ImmShift<Word __ LSL>);
            bind(opcode | 2 << 6, ImmShift<Long __ LSL>);

            opcode = parse("1110 ---0 --00 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ LSR>);
            bind(opcode | 1 << 6, ImmShift<Word __ LSR>);
            bind(opcode | 2 << 6, ImmShift<Long __ LSR>);

            opcode = parse("1110 ---1 --01 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ ROL>);
            bind(opcode | 1 << 6, ImmShift<Word __ ROL>);
            bind(opcode | 2 << 6, ImmShift<Long __ ROL>);

            opcode = parse("1110 ---0 --01 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ ROR>);
            bind(opcode | 1 << 6, ImmShift<Word __ ROR>);
            bind(opcode | 2 << 6, ImmShift<Long __ ROR>);

            opcode = parse("1110 ---1 --01 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ ROXL>);
            bind(opcode | 1 << 6, ImmShift<Word __ ROXL>);
            bind(opcode | 2 << 6, ImmShift<Long __ ROXL>);

            opcode = parse("1110 ---0 --01 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ ROXR>);
            bind(opcode | 1 << 6, ImmShift<Word __ ROXR>);
            bind(opcode | 2 << 6, ImmShift<Long __ ROXR>);
        }
    }
  
}

void
Moira::process(uint16_t reg_ird)
{
    printf("reg_ird: %d\n", reg_ird);

    (this->*exec[reg_ird])(reg_ird);
}

#undef __
