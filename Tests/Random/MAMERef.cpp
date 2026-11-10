// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "MAMERef.h"

namespace runner {

void
MAMERef::prepare(Setup &s) const
{
    for (u32 vector = 2; vector < 256; vector++) set32(s.mem, vector * 4, 0x4000);
}

bool
MAMERef::equalPC(const Result &ref, const Result &moira) const
{
    return (ref.pc & 0xFFFFFF) == (moira.pc & 0xFFFFFF);
}

}
