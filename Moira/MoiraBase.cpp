// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "MoiraBase.h"

template<> uint32_t CLIP<Byte>(uint32_t data) { return data & 0xff; }
template<> uint32_t CLIP<Word>(uint32_t data) { return data & 0xffff; }
template<> uint32_t CLIP<Long>(uint32_t data) { return data & 0xffffffff; }

template<> uint32_t MASK<Byte>() { return 0xff; }
template<> uint32_t MASK<Word>() { return 0xffff; }
template<> uint32_t MASK<Long>() { return 0xffffffff; }

template<> bool MSBIT<Byte>(uint32_t data) { return data & 0x80; }
template<> bool MSBIT<Word>(uint32_t data) { return data & 0x8000; }
template<> bool MSBIT<Long>(uint32_t data) { return data & 0x80000000; }

template<> uint32_t SIGN<Byte>(uint32_t data) { return  (int8_t)data; }
template<> uint32_t SIGN<Word>(uint32_t data) { return (int16_t)data; }
template<> uint32_t SIGN<Long>(uint32_t data) { return (int32_t)data; }

template<> uint8_t BYTES<Byte>() { return 1; }
template<> uint8_t BYTES<Word>() { return 2; }
template<> uint8_t BYTES<Long>() { return 4; }
