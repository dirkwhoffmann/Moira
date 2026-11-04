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

/* Thin wrapper around the floating-point back-end of UAE (GPLv3), used as
 * a reference to validate Moira's FPU arithmetic. UAE's own headers are
 * only visible inside UAERef.cpp, so this interface contains plain types.
 *
 * The back-end is compiled from the UAE source tree (see UAE_DIR in
 * CMakeLists.txt) and never becomes part of the Moira library.
 */
namespace uaeref {

// FPSR exception status byte bits
constexpr uint32_t BSUN  = 0x8000;
constexpr uint32_t SNAN  = 0x4000;
constexpr uint32_t OPERR = 0x2000;
constexpr uint32_t OVFL  = 0x1000;
constexpr uint32_t UNFL  = 0x0800;
constexpr uint32_t DZ    = 0x0400;
constexpr uint32_t INEX2 = 0x0200;
constexpr uint32_t INEX1 = 0x0100;

struct Result {

    uint16_t se = 0;        // Sign and exponent of a floating-point result
    uint64_t man = 0;       // Mantissa of a floating-point result
    uint32_t raw[3] = {};   // Result in a memory format (S, D, P)
    int64_t  ival = 0;      // Result of an integer conversion
    uint32_t exc = 0;       // Exception status bits
    uint32_t quot = 0;      // Quotient byte (FMOD, FREM)
};

// Selects the emulated model (68881, 68882, 68040 or 68060)
void init(int model);

// Executes an arithmetic instruction (the 7-bit command field of the extension word)
bool arith(unsigned cmd, uint16_t dseExp, uint64_t dman, uint16_t sseExp, uint64_t sman,
           uint32_t fpcr, Result &r);

// Conversions from memory formats
Result fromSingle(uint32_t raw, uint32_t fpcr);
Result fromDouble(uint64_t raw, uint32_t fpcr);
Result fromPacked(const uint32_t (&w)[3], uint32_t fpcr);

// Conversions to memory formats
Result toSingle(uint16_t se, uint64_t man, uint32_t fpcr);
Result toDouble(uint16_t se, uint64_t man, uint32_t fpcr);
Result toInteger(uint16_t se, uint64_t man, int bits, uint32_t fpcr);
Result toPacked(uint16_t se, uint64_t man, int k, uint32_t fpcr);

}
