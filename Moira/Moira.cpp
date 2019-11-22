// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include <stdio.h>
#include "Moira.h"
#include "MoiraDasm.h"

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

template<int size, int mode> void Moira::execRegShift(uint16_t opcode) {
    printf("***** execRegShift<%d,%d>(%d)\n", size, mode, opcode);
}
template<int size, int mode> int Moira::syncRegShift(uint16_t opcode, int i) {
    printf("***** syncRegShift<%d,%d>(%d,%d)\n", size, mode, opcode, i);
    return i;
}

template<int size, int mode> void Moira::execImmShift(uint16_t opcode) {
    printf("***** execImmShift<%d,%d>(%d)\n", size, mode, opcode);
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
            bind(opcode | 0 << 6, RegShift<Byte __ Asl>);
            bind(opcode | 1 << 6, RegShift<Word __ Asl>);
            bind(opcode | 2 << 6, RegShift<Long __ Asl>);

            opcode = parse("1110 ---0 --10 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ Asr>);
            bind(opcode | 1 << 6, RegShift<Word __ Asr>);
            bind(opcode | 2 << 6, RegShift<Long __ Asr>);

            opcode = parse("1110 ---1 --10 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ Lsl>);
            bind(opcode | 1 << 6, RegShift<Word __ Lsl>);
            bind(opcode | 2 << 6, RegShift<Long __ Lsl>);

            opcode = parse("1110 ---0 --10 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ Lsr>);
            bind(opcode | 1 << 6, RegShift<Word __ Lsr>);
            bind(opcode | 2 << 6, RegShift<Long __ Lsr>);

            opcode = parse("1110 ---1 --11 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ Rol>);
            bind(opcode | 1 << 6, RegShift<Word __ Rol>);
            bind(opcode | 2 << 6, RegShift<Long __ Rol>);

            opcode = parse("1110 ---0 --11 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ Ror>);
            bind(opcode | 1 << 6, RegShift<Word __ Ror>);
            bind(opcode | 2 << 6, RegShift<Long __ Ror>);

            opcode = parse("1110 ---1 --11 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ Roxl>);
            bind(opcode | 1 << 6, RegShift<Word __ Roxl>);
            bind(opcode | 2 << 6, RegShift<Long __ Roxl>);

            opcode = parse("1110 ---0 --11 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<Byte __ Roxr>);
            bind(opcode | 1 << 6, RegShift<Word __ Roxr>);
            bind(opcode | 2 << 6, RegShift<Long __ Roxr>);
        }
    }

    // (2)
    for (int data = 0; data < 8; data++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse("1110 ---1 --00 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ Asl>);
            bind(opcode | 1 << 6, ImmShift<Word __ Asl>);
            bind(opcode | 2 << 6, ImmShift<Long __ Asl>);

            opcode = parse("1110 ---0 --00 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ Asr>);
            bind(opcode | 1 << 6, ImmShift<Word __ Asr>);
            bind(opcode | 2 << 6, ImmShift<Long __ Asr>);

            opcode = parse("1110 ---1 --00 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ Lsl>);
            bind(opcode | 1 << 6, ImmShift<Word __ Lsl>);
            bind(opcode | 2 << 6, ImmShift<Long __ Lsl>);

            opcode = parse("1110 ---0 --00 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ Lsr>);
            bind(opcode | 1 << 6, ImmShift<Word __ Lsr>);
            bind(opcode | 2 << 6, ImmShift<Long __ Lsr>);

            opcode = parse("1110 ---1 --01 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ Rol>);
            bind(opcode | 1 << 6, ImmShift<Word __ Rol>);
            bind(opcode | 2 << 6, ImmShift<Long __ Rol>);

            opcode = parse("1110 ---0 --01 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ Ror>);
            bind(opcode | 1 << 6, ImmShift<Word __ Ror>);
            bind(opcode | 2 << 6, ImmShift<Long __ Ror>);

            opcode = parse("1110 ---1 --01 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ Roxl>);
            bind(opcode | 1 << 6, ImmShift<Word __ Roxl>);
            bind(opcode | 2 << 6, ImmShift<Long __ Roxl>);

            opcode = parse("1110 ---0 --01 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<Byte __ Roxr>);
            bind(opcode | 1 << 6, ImmShift<Word __ Roxr>);
            bind(opcode | 2 << 6, ImmShift<Long __ Roxr>);
        }
    }
    // exec[opcode] = &Moira::foo<8,8>;

    //shift/rotate immediate

    /*
     for ( uint8_t count : range(8) )
     for ( uint8_t dreg : range(8) ) {
     auto shift = count ? count : 8;
     DataRegister modify{dreg};

     opcode = _parse("1110 ---1 ss00 0---") | count << 9 | dreg;
     bind(opcode | 0 << 6, ImmShift<Byte __ Asl>, shift, modify);
     bind(opcode | 1 << 6, ImmShift<Word __ Asl>, shift, modify);
     bind(opcode | 2 << 6, ImmShift<Long __ Asl>, shift, modify);

     */


    /*
     for (int i = 0; i < 8; i++) {
     for (int j = 0; j < 8; j++) {
     printf("(%d,%d): %d\n", i, j, (this->*exec[8*i+j])());
     }
     }
     */
}

void
Moira::process(uint16_t reg_ird)
{
    printf("reg_ird: %d\n", reg_ird);

    (this->*exec[reg_ird])(reg_ird);
}

#undef __
