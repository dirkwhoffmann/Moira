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
static int binutilsFprintf(FILE* f, const char* format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    char* buf = (char*)f;
    const int l = strlen(buf);
    const int n = vsnprintf(buf + l, BINUTILS_BUFFER_SIZE - l, format, ap);
    va_end(ap);
    return n;
}

void setupBinutils(moira::Model model)
{
    switch (model) {

        case moira::Model::M68000:    di.mach = MACH_68000; break;
        case moira::Model::M68010:    di.mach = MACH_68010; break;
        case moira::Model::M68EC020:  // Not supported by binutils
        case moira::Model::M68020:    di.mach = MACH_68020; break;
        case moira::Model::M68EC030:  // Not supported by binutils
        case moira::Model::M68030:    di.mach = MACH_68030; break;
        case moira::Model::M68EC040:  // Not supported by binutils
        case moira::Model::M68LC040:  // Not supported by binutils
        case moira::Model::M68040:    di.mach = MACH_68040; break;
    }

    di.caller_private = &mi;
    di.memory_error_func = [](int, bfd_vma, struct disassemble_info*) {};
    di.read_memory_func = [](bfd_vma memaddr, bfd_byte* myaddr, unsigned int length, struct disassemble_info* dinfo) -> int {
        meminfo* mi = static_cast<meminfo*>(dinfo->caller_private);
        while (length--)
            *myaddr++ = memaddr < mi->len ? mi->bytes[memaddr++] : 0;
        return 0; // return <> 0 to indicate error
    };
    di.fprintf_func = &binutilsFprintf;
    di.print_address_func = [](bfd_vma addr, struct disassemble_info* dinfo) {
        dinfo->fprintf_func(dinfo->stream, "%u", addr);
    };
}

void disassembleBinutils(runner::Result &r)
{
    memcpy(mi.bytes, moiraMem, sizeof(mi.bytes));
    mi.len = 0x10000;

    di.stream = (FILE*)r.dasmBinutils;
    r.dasmBinutils[0] = 0;
    r.dasmCntBinutils = print_insn_m68k(runner::pc, &di);
}
