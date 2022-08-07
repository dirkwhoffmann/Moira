// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

/* This file declares all instruction handlers. All handlers follow a common
 * naming scheme:
 *
 *    execXXX : Handler for executing an instruction
 *    dasmXXX : Handler for disassembling an instruction
 */

#define MOIRA_DECLARE_EXEC(x) \
template <Core C, Instr I, Mode M, Size S> void exec##x(u16 op);

#define MOIRA_DECLARE_DASM(x) \
template <DasmCore D, Instr I, Mode M, Size S> void dasm##x(StrWriter &str, u32 &addr, u16 op);

#define MOIRA_DECLARE(x) \
MOIRA_DECLARE_EXEC(x) \
MOIRA_DECLARE_DASM(x)


//
// Handlers
//

MOIRA_DECLARE(LineA)
MOIRA_DECLARE(LineF)
MOIRA_DECLARE(Illegal)

MOIRA_DECLARE(ShiftRg)
MOIRA_DECLARE(ShiftIm)
MOIRA_DECLARE(ShiftEa)

MOIRA_DECLARE(AbcdRg)
MOIRA_DECLARE(AbcdEa)
MOIRA_DECLARE(AddEaRg)
MOIRA_DECLARE(AddRgEa)
MOIRA_DECLARE(Adda)
MOIRA_DECLARE(AddiRg)
MOIRA_DECLARE(AddiEa)
MOIRA_DECLARE(AddqDn)
MOIRA_DECLARE(AddqAn)
MOIRA_DECLARE(AddqEa)
MOIRA_DECLARE(AddxRg)
MOIRA_DECLARE(AddxEa)
MOIRA_DECLARE(AndEaRg)
MOIRA_DECLARE(AndRgEa)
MOIRA_DECLARE(AndiRg)
MOIRA_DECLARE(AndiEa)
MOIRA_DECLARE(Andiccr)
MOIRA_DECLARE(Andisr)

MOIRA_DECLARE(Bcc)
MOIRA_DECLARE(BitDxDy)
MOIRA_DECLARE(BitDxEa)
MOIRA_DECLARE(BitImDy)
MOIRA_DECLARE(BitImEa)
MOIRA_DECLARE(BitFieldDn)
MOIRA_DECLARE(BitFieldEa)
MOIRA_DECLARE(Bkpt)
MOIRA_DECLARE(Bra)
MOIRA_DECLARE(Bsr)

MOIRA_DECLARE(Callm)
MOIRA_DECLARE(Cas)
MOIRA_DECLARE(Cas2)
MOIRA_DECLARE(Chk)
MOIRA_DECLARE(ChkCmp2)
MOIRA_DECLARE(Clr)
MOIRA_DECLARE(Cmp)
MOIRA_DECLARE(Cmpa)
MOIRA_DECLARE(CmpiRg)
MOIRA_DECLARE(CmpiEa)
MOIRA_DECLARE(Cmpm)
MOIRA_DECLARE(CpBcc)
MOIRA_DECLARE(CpDbcc)
MOIRA_DECLARE(CpGen)
MOIRA_DECLARE(CpRestore)
MOIRA_DECLARE(CpSave)
MOIRA_DECLARE(CpScc)
MOIRA_DECLARE(CpTrapcc)

MOIRA_DECLARE(Dbcc)
MOIRA_DECLARE(Div)
MOIRA_DECLARE(Divl)

MOIRA_DECLARE(ExgDxDy)
MOIRA_DECLARE(ExgAxDy)
MOIRA_DECLARE(ExgAxAy)
MOIRA_DECLARE(Ext)
MOIRA_DECLARE(Extb)

MOIRA_DECLARE(Jmp)
MOIRA_DECLARE(Jsr)

MOIRA_DECLARE(Lea)
MOIRA_DECLARE(Link)

MOIRA_DECLARE(Move0)
MOIRA_DECLARE(Move2)
MOIRA_DECLARE(Move3)
MOIRA_DECLARE(Move4)
MOIRA_DECLARE(Move5)
MOIRA_DECLARE(Move6)
MOIRA_DECLARE(Move7)
MOIRA_DECLARE(Move8)
MOIRA_DECLARE(Movea)
MOIRA_DECLARE(MovecRcRx)
MOIRA_DECLARE(MovecRxRc)
MOIRA_DECLARE(MovemEaRg)
MOIRA_DECLARE(MovemRgEa)
MOIRA_DECLARE(MovepDxEa)
MOIRA_DECLARE(MovepEaDx)
MOIRA_DECLARE(Moveq)
MOIRA_DECLARE(MoveFromCcrRg)
MOIRA_DECLARE(MoveFromCcrEa)
MOIRA_DECLARE(MoveToCcr)
MOIRA_DECLARE(MoveFromSrRg)
MOIRA_DECLARE(MoveFromSrEa)
MOIRA_DECLARE(Moves)
MOIRA_DECLARE(MoveToSr)
MOIRA_DECLARE(MoveUspAn)
MOIRA_DECLARE(MoveAnUsp)
MOIRA_DECLARE(Mul)
MOIRA_DECLARE(Mull)

MOIRA_DECLARE(NbcdRg)
MOIRA_DECLARE(NbcdEa)
MOIRA_DECLARE(NegRg)
MOIRA_DECLARE(NegEa)
MOIRA_DECLARE(Nop)

MOIRA_DECLARE(PackDn)
MOIRA_DECLARE(PackPd)
MOIRA_DECLARE(Pea)

MOIRA_DECLARE(Reset)
MOIRA_DECLARE(Rtd)
MOIRA_DECLARE(Rte)
MOIRA_DECLARE(Rtm)
MOIRA_DECLARE(Rtr)
MOIRA_DECLARE(Rts)

MOIRA_DECLARE(SccRg)
MOIRA_DECLARE(SccEa)
MOIRA_DECLARE(Stop)
MOIRA_DECLARE(Swap)

MOIRA_DECLARE(TasRg)
MOIRA_DECLARE(TasEa)
MOIRA_DECLARE(Trap)
MOIRA_DECLARE(Trapv)
MOIRA_DECLARE(Trapcc)
MOIRA_DECLARE(Tst)

MOIRA_DECLARE(Unlk)
MOIRA_DECLARE(UnpkDn)
MOIRA_DECLARE(UnpkPd)

// Musashi compatibility mode
MOIRA_DECLARE_EXEC(MulMusashi)
MOIRA_DECLARE_EXEC(MullMusashi)
MOIRA_DECLARE_EXEC(DivMusashi)
MOIRA_DECLARE_EXEC(DivlMusashi)
