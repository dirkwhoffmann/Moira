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
void createJumpTable();
void registerInstructions();

template<Instr I> void registerShift(const char *patternReg,
                                     const char *patternImm,
                                     const char *patternEa);

void registerBSET();
void registerBTST();
void registerCLR();
void registerCMP();
void registerCMPA();
void registerDBcc();
void registerDIVx();
void registerEOR();
void registerEXT();
void registerLEA();
void registerLSL();
void registerLSR();
void registerMOVEA();
void registerMOVEQ();
void registerMULx();
void registerNBCD();
void registerNEG();
void registerNEGX();
void registerNOT();
void registerNOP();
void registerOR();
void registerROL();
void registerROR();
void registerROXL();
void registerROXR();
void registerSBCD();
void registerScc();
template <Cond CC, Instr I> void registerS();
void registerSUB();
void registerSUBA();
void registerTAS();
void registerTST();

#endif
