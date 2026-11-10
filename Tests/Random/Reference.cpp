// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Reference.h"
#include "MusashiRef.h"
#include "UAERef.h"
#include "MAMERef.h"

namespace runner {

bool
Reference::equalFPR(const Result &ref, const Result &moira) const
{
    for (int i = 0; i < 8; i++) if (!ref.fpr[i].identical(moira.fpr[i])) return false;
    return true;
}

//
// Factory
//

Reference &
ref()
{
    static MusashiRef musashi;
    static UAERef uae;
    static MAMERef mame;

    switch (compat) {

        case Compatibility::MUSASHI: return musashi;
        case Compatibility::MAME:    return mame;
        default:                     return uae;
    }
}

}
