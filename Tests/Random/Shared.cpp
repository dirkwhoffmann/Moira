// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Published under the terms of the MIT License
// -----------------------------------------------------------------------------

#include "Shared.h"

Sandbox sandbox;
Randomizer randomizer;
u8 moiraMem[0x10000];
u8 refMem[0x10000];
u32 musashiFC = 0;
meminfo mi;
disassemble_info di;
