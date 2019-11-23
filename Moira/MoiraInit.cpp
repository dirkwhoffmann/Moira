// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

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

static uint16_t
parse(const char *s, uint16_t sum = 0)
{
    return
    *s == ' ' ? parse(s + 1, sum) :
    *s == '-' ? parse(s + 1, sum << 1) :
    *s == '0' ? parse(s + 1, sum << 1) :
    *s == '1' ? parse(s + 1, (sum << 1) + 1) : sum;
}

void
Moira::init()
{
    // Start with clean tables
    for (int i = 0; i < 65536; i++) {
        exec[i] = &Moira::execIllegal;
        dasm[i] = &Moira::dasmIllegal;
        sync[i] = &Moira::syncIllegal;
    }

    registerShiftCmds();
}

void
Moira::registerShiftCmds()
{
    uint16_t opcode;

    // ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR
    //
    // Modes: (1)   Dx,Dy       8,16,32
    //        (2)   #<data>,Dy  8,16,32
    //        (3)   <ea>          16
    //               --------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
    //               --------------------------------------------------
    //                X       X   X   X   X   X   X   X

    // (1)
    for (int dx = 0; dx < 8; dx++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse("1110 ---1 --10 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<ASL __ Byte>);
            bind(opcode | 1 << 6, RegShift<ASL __ Word>);
            bind(opcode | 2 << 6, RegShift<ASL __ Long>);

            opcode = parse("1110 ---0 --10 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<ASR __ Byte>);
            bind(opcode | 1 << 6, RegShift<ASR __ Word>);
            bind(opcode | 2 << 6, RegShift<ASR __ Long>);

            opcode = parse("1110 ---1 --10 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<LSL __ Byte>);
            bind(opcode | 1 << 6, RegShift<LSL __ Word>);
            bind(opcode | 2 << 6, RegShift<LSL __ Long>);

            opcode = parse("1110 ---0 --10 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<LSR __ Byte>);
            bind(opcode | 1 << 6, RegShift<LSR __ Word>);
            bind(opcode | 2 << 6, RegShift<LSR __ Long>);

            opcode = parse("1110 ---1 --11 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<ROL __ Byte>);
            bind(opcode | 1 << 6, RegShift<ROL __ Word>);
            bind(opcode | 2 << 6, RegShift<ROL __ Long>);

            opcode = parse("1110 ---0 --11 1---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<ROR __ Byte>);
            bind(opcode | 1 << 6, RegShift<ROR __ Word>);
            bind(opcode | 2 << 6, RegShift<ROR __ Long>);

            opcode = parse("1110 ---1 --11 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<ROXL __ Byte>);
            bind(opcode | 1 << 6, RegShift<ROXL __ Word>);
            bind(opcode | 2 << 6, RegShift<ROXL __ Long>);

            opcode = parse("1110 ---0 --11 0---") | dx << 9 | dy;
            bind(opcode | 0 << 6, RegShift<ROXR __ Byte>);
            bind(opcode | 1 << 6, RegShift<ROXR __ Word>);
            bind(opcode | 2 << 6, RegShift<ROXR __ Long>);
        }
    }

    // (2)
    for (int data = 0; data < 8; data++) {
        for (int dy = 0; dy < 8; dy++) {

            opcode = parse("1110 ---1 --00 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<ASL __ Byte>);
            bind(opcode | 1 << 6, ImmShift<ASL __ Word>);
            bind(opcode | 2 << 6, ImmShift<ASL __ Long>);

            opcode = parse("1110 ---0 --00 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<ASR __ Byte>);
            bind(opcode | 1 << 6, ImmShift<ASR __ Word>);
            bind(opcode | 2 << 6, ImmShift<ASR __ Long>);

            opcode = parse("1110 ---1 --00 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<LSL __ Byte>);
            bind(opcode | 1 << 6, ImmShift<LSL __ Word>);
            bind(opcode | 2 << 6, ImmShift<LSL __ Long>);

            opcode = parse("1110 ---0 --00 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<LSR __ Byte>);
            bind(opcode | 1 << 6, ImmShift<LSR __ Word>);
            bind(opcode | 2 << 6, ImmShift<LSR __ Long>);

            opcode = parse("1110 ---1 --01 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<ROL __ Byte>);
            bind(opcode | 1 << 6, ImmShift<ROL __ Word>);
            bind(opcode | 2 << 6, ImmShift<ROL __ Long>);

            opcode = parse("1110 ---0 --01 1---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<ROR __ Byte>);
            bind(opcode | 1 << 6, ImmShift<ROR __ Word>);
            bind(opcode | 2 << 6, ImmShift<ROR __ Long>);

            opcode = parse("1110 ---1 --01 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<ROXL __ Byte>);
            bind(opcode | 1 << 6, ImmShift<ROXL __ Word>);
            bind(opcode | 2 << 6, ImmShift<ROXL __ Long>);

            opcode = parse("1110 ---0 --01 0---") | data << 9 | dy;
            bind(opcode | 0 << 6, ImmShift<ROXR __ Byte>);
            bind(opcode | 1 << 6, ImmShift<ROXR __ Word>);
            bind(opcode | 2 << 6, ImmShift<ROXR __ Long>);
        }
    }

    // (3)
    opcode = parse("1110 0001 11-- ----");
    for (int ax = 0; ax < 8; ax++) {
        bind(opcode | 2 << 3 | ax, EaShift<ASL __ 2>);
        bind(opcode | 3 << 3 | ax, EaShift<ASL __ 3>);
        bind(opcode | 4 << 3 | ax, EaShift<ASL __ 4>);
        bind(opcode | 5 << 3 | ax, EaShift<ASL __ 5>);
        bind(opcode | 6 << 3 | ax, EaShift<ASL __ 6>);
    }
    bind(opcode | 7 << 3 | 0, EaShift<ASL __ 7>);
    bind(opcode | 7 << 3 | 1, EaShift<ASL __ 8>);

    opcode = parse("1110 0000 11-- ----");
    for (int ax = 0; ax < 8; ax++) {
        bind(opcode | 2 << 3 | ax, EaShift<ASR __ 2>);
        bind(opcode | 3 << 3 | ax, EaShift<ASR __ 3>);
        bind(opcode | 4 << 3 | ax, EaShift<ASR __ 4>);
        bind(opcode | 5 << 3 | ax, EaShift<ASR __ 5>);
        bind(opcode | 6 << 3 | ax, EaShift<ASR __ 6>);
    }
    bind(opcode | 7 << 3 | 0, EaShift<ASR __ 7>);
    bind(opcode | 7 << 3 | 1, EaShift<ASR __ 8>);

    opcode = parse("1110 0011 11-- ----");
    for (int ax = 0; ax < 8; ax++) {
        bind(opcode | 2 << 3 | ax, EaShift<LSL __ 2>);
        bind(opcode | 3 << 3 | ax, EaShift<LSL __ 3>);
        bind(opcode | 4 << 3 | ax, EaShift<LSL __ 4>);
        bind(opcode | 5 << 3 | ax, EaShift<LSL __ 5>);
        bind(opcode | 6 << 3 | ax, EaShift<LSL __ 6>);
    }
    bind(opcode | 7 << 3 | 0, EaShift<LSL __ 7>);
    bind(opcode | 7 << 3 | 1, EaShift<LSL __ 8>);

    opcode = parse("1110 0010 11-- ----");
    for (int ax = 0; ax < 8; ax++) {
        bind(opcode | 2 << 3 | ax, EaShift<LSR __ 2>);
        bind(opcode | 3 << 3 | ax, EaShift<LSR __ 3>);
        bind(opcode | 4 << 3 | ax, EaShift<LSR __ 4>);
        bind(opcode | 5 << 3 | ax, EaShift<LSR __ 5>);
        bind(opcode | 6 << 3 | ax, EaShift<LSR __ 6>);
    }
    bind(opcode | 7 << 3 | 0, EaShift<LSR __ 7>);
    bind(opcode | 7 << 3 | 1, EaShift<LSR __ 8>);

    opcode = parse("1110 0111 11-- ----");
    for (int ax = 0; ax < 8; ax++) {
        bind(opcode | 2 << 3 | ax, EaShift<ROL __ 2>);
        bind(opcode | 3 << 3 | ax, EaShift<ROL __ 3>);
        bind(opcode | 4 << 3 | ax, EaShift<ROL __ 4>);
        bind(opcode | 5 << 3 | ax, EaShift<ROL __ 5>);
        bind(opcode | 6 << 3 | ax, EaShift<ROL __ 6>);
    }
    bind(opcode | 7 << 3 | 0, EaShift<ROL __ 7>);
    bind(opcode | 7 << 3 | 1, EaShift<ROL __ 8>);

    opcode = parse("1110 0110 11-- ----");
    for (int ax = 0; ax < 8; ax++) {
        bind(opcode | 2 << 3 | ax, EaShift<ROR __ 2>);
        bind(opcode | 3 << 3 | ax, EaShift<ROR __ 3>);
        bind(opcode | 4 << 3 | ax, EaShift<ROR __ 4>);
        bind(opcode | 5 << 3 | ax, EaShift<ROR __ 5>);
        bind(opcode | 6 << 3 | ax, EaShift<ROR __ 6>);
    }
    bind(opcode | 7 << 3 | 0, EaShift<ROR __ 7>);
    bind(opcode | 7 << 3 | 1, EaShift<ROR __ 8>);

    opcode = parse("1110 0101 11-- ----");
    for (int ax = 0; ax < 8; ax++) {
        bind(opcode | 2 << 3 | ax, EaShift<ROXL __ 2>);
        bind(opcode | 3 << 3 | ax, EaShift<ROXL __ 3>);
        bind(opcode | 4 << 3 | ax, EaShift<ROXL __ 4>);
        bind(opcode | 5 << 3 | ax, EaShift<ROXL __ 5>);
        bind(opcode | 6 << 3 | ax, EaShift<ROXL __ 6>);
    }
    bind(opcode | 7 << 3 | 0, EaShift<ROXL __ 7>);
    bind(opcode | 7 << 3 | 1, EaShift<ROXL __ 8>);

    opcode = parse("1110 0100 11-- ----");
    for (int ax = 0; ax < 8; ax++) {
        bind(opcode | 2 << 3 | ax, EaShift<ROXR __ 2>);
        bind(opcode | 3 << 3 | ax, EaShift<ROXR __ 3>);
        bind(opcode | 4 << 3 | ax, EaShift<ROXR __ 4>);
        bind(opcode | 5 << 3 | ax, EaShift<ROXR __ 5>);
        bind(opcode | 6 << 3 | ax, EaShift<ROXR __ 6>);
    }
    bind(opcode | 7 << 3 | 0, EaShift<ROXR __ 7>);
    bind(opcode | 7 << 3 | 1, EaShift<ROXR __ 8>);
}

void
Moira::process(uint16_t reg_ird)
{
    printf("reg_ird: %d\n", reg_ird);

    (this->*exec[reg_ird])(reg_ird);
}

void
Moira::registerMoveCmds()
{
    uint16_t opcode;

    // LEA
    //
    // Modes:       LEA <ea>,An
    //
    //               --------------------------------------------------
    //              | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 |
    //               --------------------------------------------------
    //                        X           X   X   X   X   X   X

    for (int an = 0; an < 8; an++) {

        opcode = parse("0100 ---1 11-- ----") | an << 9;
        for (int reg = 0; reg < 8; reg++) {
            bind(opcode | 2 << 3 | reg, Lea<2>);
            bind(opcode | 5 << 3 | reg, Lea<5>);
            bind(opcode | 6 << 3 | reg, Lea<6>);
        }
        bind(opcode | 7 << 3 | 0, Lea<7>);
        bind(opcode | 7 << 3 | 1, Lea<8>);
        bind(opcode | 7 << 3 | 2, Lea<9>);
        bind(opcode | 7 << 3 | 3, Lea<10>);
    }
}

#undef __
