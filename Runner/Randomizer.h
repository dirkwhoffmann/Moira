// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "MoiraTypes.h"

using namespace moira;

class Randomizer {

    int mode;

public:

    int init();
    void prepare(int testrun);
    u32 rand();

private:

    u32 rand(int numberType);
};
