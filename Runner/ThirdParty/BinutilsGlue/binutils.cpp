// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "Shared.h"
#include <cstdarg>

// Output function of the binutils disassembler. The "stream" is a character
// buffer of size BINUTILS_BUFFER_SIZE. Output is appended to its contents.
int binutilsFprintf(FILE* f, const char* format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    char* buf = (char*)f;
    const int l = strlen(buf);
    const int n = vsnprintf(buf + l, BINUTILS_BUFFER_SIZE - l, format, ap);
    va_end(ap);
    return n;
}
