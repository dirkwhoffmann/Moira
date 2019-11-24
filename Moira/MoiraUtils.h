// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef MOIRA_BASE_H
#define MOIRA_BASE_H

#include "types.h"

using namespace Moira;

enum Size { Byte, Word, Long };

template<Size> u32 CLIP(u32 data);
template<Size> u32 MASK();
template<Size> bool     MSBIT(u32 data);
template<Size> u32 SIGN(u32 data);
template<Size> u8  BYTES();
template<Size S> bool   ZERO(u32 data) { return CLIP<S>(data) == 0; }
template<Size S> bool   NEG(u32 data) { return SIGN<S>(data) < 0; }


enum Instr { ASL, ASR, LSL, LSR, ROL, ROR, ROXL, ROXR };


/* Adressing modes
 *
 *   0:        Dn : Data register direct
 *   1:        An : Address register direct
 *   2:      (An) : Address register indirect
 *   3:     (An)+ : Address register indirect with post increment
 *   4:     -(An) : Address register indirect with pre decrement
 *   5:    (d,An) : Address register indirect with displacement
 *   6: (d,An,Xi) : Address register indirect with displacement and indexing
 *   7:    ####.w : Absolute addressing short
 *   8:    ####.l : Absolute addressing long
 *   9:    (d,PC) : Program counter indirect with displacement
 *  10: (d,PC,Xi) : Program counter indirect with displacement and indexing
 */

typedef u8 Mode;

// enum Mode { DRD, ARD, ARIPI, ARIPD, ARID, ARIDI, ABS16, ABS32, PCID, PCII };


struct Reg {

    u32 value;

    template<Size S> u32 read() { return CLIP<S>(value); }
    template<Size S> void write(u32 v) { value = CLIP<S>(v); }

    Reg & operator=(const u32 & rhs) { value = rhs; return *this; }
};


#endif
