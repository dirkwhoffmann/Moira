// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "SoftFloat.h"

namespace MOIRA_NAMESPACE {

SoftFloat &
SoftFloat::standard()
{
    thread_local SoftFloat instance;
    return instance;
}

RoundingMode
SoftFloat::roundingMode() const
{
    switch (env.roundingMode) {

        case softfloat_round_minMag:    return RoundingMode::ZERO;
        case softfloat_round_min:       return RoundingMode::DOWN;
        case softfloat_round_max:       return RoundingMode::UP;
        default:                        return RoundingMode::NEAREST;
    }
}

void
SoftFloat::setRoundingMode(RoundingMode mode)
{
    switch (mode) {

        case RoundingMode::NEAREST: env.roundingMode = softfloat_round_near_even; break;
        case RoundingMode::ZERO:    env.roundingMode = softfloat_round_minMag; break;
        case RoundingMode::DOWN:    env.roundingMode = softfloat_round_min; break;
        case RoundingMode::UP:      env.roundingMode = softfloat_round_max; break;
    }
}

Precision
SoftFloat::precision() const
{
    switch (env.precision) {

        case 32:    return Precision::SINGLE;
        case 64:    return Precision::DOUBLE;
        default:    return Precision::EXTENDED;
    }
}

void
SoftFloat::setPrecision(Precision precision)
{
    env.precision = (uint_fast8_t)precision;
}

SoftFloat::Scope::Scope(SoftFloat &sf, bool restoreFlags) : sf(sf), restoreFlags(restoreFlags)
{
    savedMode = sf.roundingMode();
    savedPrecision = sf.precision();
    savedLimits = sf.limitsExponent();
    savedFlags = sf.flags();
}

SoftFloat::Scope::~Scope()
{
    sf.setRoundingMode(savedMode);
    sf.setPrecision(savedPrecision);
    sf.setLimitsExponent(savedLimits);

    if (restoreFlags) {
        sf.clearFlags();
        sf.raiseFlags(savedFlags);
    }
}

}
