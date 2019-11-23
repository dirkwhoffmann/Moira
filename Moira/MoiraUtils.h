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

#include "stdint.h"

enum Size { Byte, Word, Long };

template<Size> uint32_t CLIP(uint32_t data);
template<Size> uint32_t MASK();
template<Size> bool     MSBIT(uint32_t data);
template<Size> uint32_t SIGN(uint32_t data);
template<Size> uint8_t  BYTES();
template<Size S> bool   ZERO(uint32_t data) { return CLIP<S>(data) == 0; }
template<Size S> bool   NEG(uint32_t data) { return SIGN<S>(data) < 0; }


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

typedef uint8_t Mode;

// enum Mode { DRD, ARD, ARIPI, ARIPD, ARID, ARIDI, ABS16, ABS32, PCID, PCII };

#endif
