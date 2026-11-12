// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "SSTRunner.h"
#include "UaeCore.h"

int main(int argc, char **argv)
{
    using namespace sstrunner;

    // The runner needs a reference core to be built, but does not use it
    RetryRunner runner([](Memory &mem, Fault &fault) -> RefCore * { return new UaeCore(mem, fault); });

    return runner.run(argc, argv);
}
