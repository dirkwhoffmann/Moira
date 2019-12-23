//
//  globals.h
//  Moira
//
//  Created by Dirk Hoffmann on 23.12.19.
//  Copyright © 2019 Dirk Hoffmann. All rights reserved.
//

#ifndef globals_h
#define globals_h

//
// Configure
//

// Set to true to compare Moira against Musashi
#define MUSASHI true

namespace moira { class Moira; }
extern class moira::Moira *moiracpu;
extern class Tester_68k *tester;
extern uint8_t mem[0x10000];

#endif
