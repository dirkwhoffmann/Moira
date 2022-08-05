// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#define REVERSE_8(x) (u8)(((x) * 0x0202020202ULL & 0x010884422010ULL) % 1023)
#define REVERSE_16(x) (u16)((REVERSE_8((x) & 0xFF) << 8) | REVERSE_8(((x) >> 8) & 0xFF))
#define U32_ADD(x,y) (u32)((i64)(x) + (i64)(y))
#define U32_SUB(x,y) (u32)((i64)(x) - (i64)(y))
#define U32_INC(x,y) x = U32_ADD(x,y)
#define U32_DEC(x,y) x = U32_SUB(x,y)

#define ______________xx(opcode) (u8)((opcode >> 0)  & 0b11)
#define _____________xxx(opcode) (u8)((opcode >> 0)  & 0b111)
#define ____________xxxx(opcode) (u8)((opcode >> 0)  & 0b1111)
#define ___________xxxxx(opcode) (u8)((opcode >> 0)  & 0b11111)
#define __________xxxxxx(opcode) (u8)((opcode >> 0)  & 0b111111)
#define ________xxxxxxxx(opcode) (u8)((opcode >> 0)  & 0b11111111)
#define ____xxxxxxxxxxxx(opcode) (u16)((opcode >> 0) & 0b111111111111)
#define __________xxx___(opcode) (u8)((opcode >> 3)  & 0b111)
#define __________xx____(opcode) (u8)((opcode >> 4)  & 0b11)
#define _______xxx______(opcode) (u8)((opcode >> 6)  & 0b111)
#define ______xxxx______(opcode) (u8)((opcode >> 6)  & 0b1111)
#define _____xxxxx______(opcode) (u8)((opcode >> 6)  & 0b11111)
#define __________x_____(opcode) (u8)((opcode >> 5)  & 0b1)
#define _________x______(opcode) (u8)((opcode >> 6)  & 0b1)
#define ________x_______(opcode) (u8)((opcode >> 7)  & 0b1)
#define _______x________(opcode) (u8)((opcode >> 8)  & 0b1)
#define _____xx_________(opcode) (u8)((opcode >> 9)  & 0b11)
#define ____xxx_________(opcode) (u8)((opcode >> 9)  & 0b111)
#define ____xx__________(opcode) (u8)((opcode >> 10) & 0b11)
#define ____x___________(opcode) (u8)((opcode >> 11) & 0b1)
#define _xxx____________(opcode) (u8)((opcode >> 12) & 0b111)
#define xxxx____________(opcode) (u8)((opcode >> 12) & 0b1111)
