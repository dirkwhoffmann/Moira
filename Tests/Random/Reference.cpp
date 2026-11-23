// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "Reference.h"

namespace runner {

bool
Reference::equalFPR(const Result &ref, const Result &moira) const
{
    for (int i = 0; i < 8; i++) if (!ref.fpr[i].identical(moira.fpr[i])) return false;
    return true;
}

}
