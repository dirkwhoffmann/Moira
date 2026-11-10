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

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* Portable pseudo-random number generator. It reproduces the sequence of glibc's
 * rand() (TYPE_3 additive feedback generator, 31-bit results). The C library's
 * rand() is implementation defined, so using it would make the test runs depend
 * on the platform (Linux, macOS and Windows would execute different test cases).
 */
void portableSrand(unsigned seed);
int portableRand();

class Randomizer {

public:

    u32 rand();
};
