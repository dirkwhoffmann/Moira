// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#define DASM(x) \
template<Instr I, Mode M, Size S> void x(StrWriter &str, u32 &addr, u16 op);

DASM(dasmShiftRg)
DASM(dasmShiftIm)
DASM(dasmShiftEa)

DASM(dasmAbcd);
DASM(dasmAddEaRg);
DASM(dasmAddRgEa);
DASM(dasmAdda);
DASM(dasmAddiRg);
DASM(dasmAddiEa);
DASM(dasmAddqDn);
DASM(dasmAddqAn);
DASM(dasmAddqEa);
DASM(dasmAddxRg);
DASM(dasmAddxEa);
DASM(dasmAndEaRg);
DASM(dasmAndRgEa);
DASM(dasmAndiRg);
DASM(dasmAndiEa);
DASM(dasmAndiccr);
DASM(dasmAndisr);

DASM(dasmBcc);
DASM(dasmBitDxEa);
DASM(dasmBitImEa);
DASM(dasmBsr);

DASM(dasmChk);
DASM(dasmClr);
DASM(dasmCmp);
DASM(dasmCmpa);
DASM(dasmCmpiRg);
DASM(dasmCmpiEa);
DASM(dasmCmpm);

DASM(dasmDbcc);

DASM(dasmExgDxDy);
DASM(dasmExgAxDy);
DASM(dasmExgAxAy);
DASM(dasmExt);

DASM(dasmJmp);
DASM(dasmJsr);

DASM(dasmLea);
DASM(dasmLink);

DASM(dasmMove0);
DASM(dasmMove2);
DASM(dasmMove3);
DASM(dasmMove4);
DASM(dasmMove5);
DASM(dasmMove6);
DASM(dasmMove7);
DASM(dasmMove8);
DASM(dasmMovea);
DASM(dasmMovemEaRg);
DASM(dasmMovemRgEa);
DASM(dasmMovepDxEa);
DASM(dasmMovepEaDx);
DASM(dasmMoveq);
DASM(dasmMoveToCcr);
DASM(dasmMoveFromSrRg);
DASM(dasmMoveFromSrEa);
DASM(dasmMoveToSr);
DASM(dasmMoveUspAn);
DASM(dasmMoveAnUsp);
DASM(dasmMul);
DASM(dasmDiv);

DASM(dasmNbcd);
DASM(dasmNegRg);
DASM(dasmNegEa);
DASM(dasmNop);

DASM(dasmPea);

DASM(dasmReset);
DASM(dasmRte);
DASM(dasmRtr);
DASM(dasmRts);

DASM(dasmSccRg);
DASM(dasmSccEa);
DASM(dasmStop);
DASM(dasmSwap);

DASM(dasmTasRg);
DASM(dasmTasEa);
DASM(dasmTrap);
DASM(dasmTrapv);
DASM(dasmTst);

DASM(dasmUnlk);
