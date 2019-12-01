// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#ifndef MOIRA_INIT_H
#define MOIRA_INIT_H

void init();
void registerUnimplemented();

template<Instr I> void registerShift(const char *patternReg,
                                     const char *patternImm,
                                     const char *patternEa);

template<Instr I> void registerAddSub(const char *pattern1,
                                      const char *pattern2);

void registerADD();
void registerASL();
void registerASR();
void registerLEA();
void registerLSL();
void registerLSR();
void registerROL();
void registerROR();
void registerROXL();
void registerROXR();
void registerSUB();

#endif