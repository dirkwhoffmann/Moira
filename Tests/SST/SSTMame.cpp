// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "SSTRunner.h"
#include "MameCore.h"

int main(int argc, char **argv)
{
    using namespace sstrunner;

    PlainRunner runner([](Memory &mem, Fault &fault) -> RefCore * { return new MameCore(mem, fault); });

    return runner.run(argc, argv);
}
