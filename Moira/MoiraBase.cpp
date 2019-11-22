// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "MoiraBase.h"

template<> uint32_t clip<Byte>(uint32_t data) { return data & 0xff; }
template<> uint32_t clip<Word>(uint32_t data) { return data & 0xffff; }
template<> uint32_t clip<Long>(uint32_t data) { return data & 0xffffffff; }

template<> uint32_t mask<Byte>() { return 0xff; }
template<> uint32_t mask<Word>() { return 0xffff; }
template<> uint32_t mask<Long>() { return 0xffffffff; }

template<> bool msbit<Byte>(uint32_t data) { return data & 0x80; }
template<> bool msbit<Word>(uint32_t data) { return data & 0x8000; }
template<> bool msbit<Long>(uint32_t data) { return data & 0x80000000; }

template<> uint32_t sign<Byte>(uint32_t data) { return  (int8_t)data; }
template<> uint32_t sign<Word>(uint32_t data) { return (int16_t)data; }
template<> uint32_t sign<Long>(uint32_t data) { return (int32_t)data; }

template<> uint8_t bytes<Byte>() { return 1; }
template<> uint8_t bytes<Word>() { return 2; }
template<> uint8_t bytes<Long>() { return 4; }
