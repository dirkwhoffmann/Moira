// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "ExecRunner.h"
#include "MAMERef.h"

int main(int argc, char **argv)
{
    runner::MAMERef reference;
    runner::ExecRunner exec(reference);

    return runner::runnerMain(exec, argc, argv);
}
