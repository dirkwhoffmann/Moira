// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "UAERef.h"
#include "sysconfig.h"
#include "sysdeps.h"
#include "options.h"
#include "events.h"
#include "uae.h"
#include "memory.h"
#include "custom.h"
#include "newcpu.h"
#include "fpp.h"

namespace uaeref {

namespace {

fpdata make(uint16_t se, uint64_t man)
{
    fpdata d;
    memset(&d, 0, sizeof(d));
    d.fpx.high = se;
    d.fpx.low = man;
    return d;
}

// Prepares a computation: loads rounding mode and precision and clears the status
void begin(uint32_t fpcr)
{
    fpp_set_mode(fpcr);
    fpp_clear_status();
}

// Finishes a computation: fills in the result and the exception bits
Result end(const fpdata &d)
{
    Result r;
    r.se = d.fpx.high;
    r.man = d.fpx.low;
    uae_u32 status = 0;
    fpp_get_status(&status);
    r.exc = status & 0xFF00;
    return r;
}

Result endRaw()
{
    Result r;
    uae_u32 status = 0;
    fpp_get_status(&status);
    r.exc = status & 0xFF00;
    return r;
}

}

void init(int model)
{
    fp_init_softfloat(model);
}

bool arith(unsigned cmd, uint16_t dse, uint64_t dman, uint16_t sse, uint64_t sman,
           uint32_t fpcr, Result &r)
{
    fpdata dst = make(dse, dman), src = make(sse, sman);
    uae_u64 q = 0;
    uae_u8 s = 0;

    begin(fpcr);

    // This mirrors fp_arithmetic() in UAE's fpp.cpp
    switch (cmd & 0x7f) {

        case 0x00: fpp_move(&dst, &src, PREC_NORMAL); break;
        case 0x40: fpp_move(&dst, &src, PREC_FLOAT); break;
        case 0x44: fpp_move(&dst, &src, PREC_DOUBLE); break;
        case 0x01: fpp_int(&dst, &src); break;
        case 0x02: fpp_sinh(&dst, &src); break;
        case 0x03: fpp_intrz(&dst, &src); break;
        case 0x04: fpp_sqrt(&dst, &src, PREC_NORMAL); break;
        case 0x41: fpp_sqrt(&dst, &src, PREC_FLOAT); break;
        case 0x45: fpp_sqrt(&dst, &src, PREC_DOUBLE); break;
        case 0x06: fpp_lognp1(&dst, &src); break;
        case 0x08: fpp_etoxm1(&dst, &src); break;
        case 0x09: fpp_tanh(&dst, &src); break;
        case 0x0a: fpp_atan(&dst, &src); break;
        case 0x0c: fpp_asin(&dst, &src); break;
        case 0x0d: fpp_atanh(&dst, &src); break;
        case 0x0e: fpp_sin(&dst, &src); break;
        case 0x0f: fpp_tan(&dst, &src); break;
        case 0x10: fpp_etox(&dst, &src); break;
        case 0x11: fpp_twotox(&dst, &src); break;
        case 0x12: fpp_tentox(&dst, &src); break;
        case 0x14: fpp_logn(&dst, &src); break;
        case 0x15: fpp_log10(&dst, &src); break;
        case 0x16: fpp_log2(&dst, &src); break;
        case 0x18: fpp_abs(&dst, &src, PREC_NORMAL); break;
        case 0x58: fpp_abs(&dst, &src, PREC_FLOAT); break;
        case 0x5c: fpp_abs(&dst, &src, PREC_DOUBLE); break;
        case 0x19: fpp_cosh(&dst, &src); break;
        case 0x1a: fpp_neg(&dst, &src, PREC_NORMAL); break;
        case 0x5a: fpp_neg(&dst, &src, PREC_FLOAT); break;
        case 0x5e: fpp_neg(&dst, &src, PREC_DOUBLE); break;
        case 0x1c: fpp_acos(&dst, &src); break;
        case 0x1d: fpp_cos(&dst, &src); break;
        case 0x1e: fpp_getexp(&dst, &src); break;
        case 0x1f: fpp_getman(&dst, &src); break;
        case 0x20: fpp_div(&dst, &src, PREC_NORMAL); break;
        case 0x60: fpp_div(&dst, &src, PREC_FLOAT); break;
        case 0x64: fpp_div(&dst, &src, PREC_DOUBLE); break;
        case 0x21: fpp_mod(&dst, &src, &q, &s); break;
        case 0x22: fpp_add(&dst, &src, PREC_NORMAL); break;
        case 0x62: fpp_add(&dst, &src, PREC_FLOAT); break;
        case 0x66: fpp_add(&dst, &src, PREC_DOUBLE); break;
        case 0x23: fpp_mul(&dst, &src, PREC_NORMAL); break;
        case 0x63: fpp_mul(&dst, &src, PREC_FLOAT); break;
        case 0x67: fpp_mul(&dst, &src, PREC_DOUBLE); break;
        case 0x24: fpp_sgldiv(&dst, &src); break;
        case 0x25: fpp_rem(&dst, &src, &q, &s); break;
        case 0x26: fpp_scale(&dst, &src); break;
        case 0x27: fpp_sglmul(&dst, &src); break;
        case 0x28: fpp_sub(&dst, &src, PREC_NORMAL); break;
        case 0x68: fpp_sub(&dst, &src, PREC_FLOAT); break;
        case 0x6c: fpp_sub(&dst, &src, PREC_DOUBLE); break;

        default:
            return false;
    }

    r = end(dst);
    r.quot = (q & 0x7f) | (s ? 0x80 : 0);
    return true;
}

Result fromSingle(uint32_t raw, uint32_t fpcr)
{
    fpdata d = make(0, 0);
    begin(fpcr);
    fpp_to_single(&d, raw);
    return end(d);
}

Result fromDouble(uint64_t raw, uint32_t fpcr)
{
    fpdata d = make(0, 0);
    begin(fpcr);
    fpp_to_double(&d, uint32_t(raw >> 32), uint32_t(raw));
    return end(d);
}

Result fromPacked(const uint32_t (&w)[3], uint32_t fpcr)
{
    fpdata d = make(0, 0);
    uae_u32 wrd[3] = { w[0], w[1], w[2] };
    begin(fpcr);
    fpp_to_pack(&d, wrd, 0);
    return end(d);
}

Result toSingle(uint16_t se, uint64_t man, uint32_t fpcr)
{
    fpdata d = make(se, man);
    begin(fpcr);
    uae_u32 raw = fpp_from_single(&d);
    Result r = endRaw();
    r.raw[0] = raw;
    return r;
}

Result toDouble(uint16_t se, uint64_t man, uint32_t fpcr)
{
    fpdata d = make(se, man);
    uae_u32 hi = 0, lo = 0;
    begin(fpcr);
    fpp_from_double(&d, &hi, &lo);
    Result r = endRaw();
    r.raw[0] = hi;
    r.raw[1] = lo;
    return r;
}

Result toInteger(uint16_t se, uint64_t man, int bits, uint32_t fpcr)
{
    fpdata d = make(se, man);
    begin(fpcr);
    int64_t v = fpp_to_int(&d, bits == 8 ? 0 : bits == 16 ? 1 : 2);
    Result r = endRaw();
    r.ival = v;
    return r;
}

Result toPacked(uint16_t se, uint64_t man, int k, uint32_t fpcr)
{
    fpdata d = make(se, man);
    uae_u32 wrd[3] = { 0, 0, 0 };
    begin(fpcr);
    fpp_normalize(&d);
    fpp_from_pack(&d, wrd, k);
    Result r = endRaw();
    r.raw[0] = wrd[0]; r.raw[1] = wrd[1]; r.raw[2] = wrd[2];
    return r;
}

}
