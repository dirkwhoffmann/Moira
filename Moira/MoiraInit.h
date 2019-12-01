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

template<Instr I> void registerAddSub(const char *patternXXReg,
                                      const char *patternRegXX);

template<Instr I> void registerLogic(const char *patternXXReg,
                                     const char *patternRegXX);

template<Instr I> void registerClr(const char *pattern);

void registerADD();
void registerAND();
void registerASL();
void registerASR();
void registerCLR();
void registerLEA();
void registerLSL();
void registerLSR();
void registerROL();
void registerROR();
void registerROXL();
void registerROXR();
void registerSUB();

#endif
