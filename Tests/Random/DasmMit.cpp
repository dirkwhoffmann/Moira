// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "DasmRunner.h"

int main(int argc, char **argv)
{
    runner::DasmRunner dasm(true);

    return runner::runnerMain(dasm, argc, argv);
}
