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

template<Instr I> void registerAbcdSbcd(const char *patternReg,
                                        const char *patternInd);

template<Instr I> void registerAddSubEaRg(const char *pattern);
template<Instr I> void registerAddSubRgEa(const char *pattern);

template<Instr I> void registerMulDiv(const char *pattern);

template<Instr I> void registerNegNot(const char *pattern);

template<Instr I> void registerClr(const char *pattern);

void registerABCD();
void registerADD();
void registerADDA();
void registerAND();
void registerASL();
void registerASR();
void registerBCHG();
void registerBCLR();
void registerBSET();
void registerBTST();
void registerCLR();
void registerCMP();
void registerCMPA();
void registerDBcc();
void registerDIVS();
void registerDIVU();
void registerEOR();
void registerEXT();
void registerLEA();
void registerLSL();
void registerLSR();
void registerMOVEA();
void registerMOVEQ();
void registerMULS();
void registerMULU();
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
