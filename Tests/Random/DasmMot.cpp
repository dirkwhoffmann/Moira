// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "DasmRunner.h"

int main(int argc, char **argv)
{
    runner::DasmRunner dasm(false);

    return runner::runnerMain(dasm, argc, argv);
}
