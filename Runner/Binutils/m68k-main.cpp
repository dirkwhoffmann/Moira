#if 0

#include <iostream>
#include <string.h>
#include "m68k-dis.h"

// Code extracted from the files linked at https://github.com/dirkwhoffmann/Moira/blob/34dfd0607334f1b116ddda3c0e6da3bfd7f1d9ed/Bartman/dasm.ts
// Exact commit https://github.com/BartmanAbyss/binutils-gdb/commit/b363b9daabc9f16707aab444f69ad1ef71bd9b84

struct meminfo {
    unsigned char bytes[32];
    unsigned len;
};

int main()
{
    meminfo mi;
    memcpy(mi.bytes, "\xf2\x00\x5c\x1a\x03\x33", 6);
    mi.len = 6;
    //memcpy(mi.bytes, "\xF2\x00\x5c\x1a\x40\x05\x00\x00\xF6\x00\x00\x00\x00\x00\x00\x00", 16);
    //mi.len = 16;

    disassemble_info di;
    di.mach = MACH_68000;
    di.stream = stdout;
    di.caller_private = &mi;
    di.memory_error_func = [](int, bfd_vma, struct disassemble_info*) {};
    di.read_memory_func = [](bfd_vma memaddr, bfd_byte* myaddr, unsigned int length, struct disassemble_info* dinfo) -> int {
        meminfo* mi = static_cast<meminfo*>(dinfo->caller_private);
        while (length--)
            *myaddr++ = memaddr < mi->len ? mi->bytes[memaddr++] : 0;
        return 0; // return <> 0 to indicate error
    };
    di.fprintf_func = &fprintf;
    di.print_address_func = [](bfd_vma addr, struct disassemble_info* dinfo) {
        dinfo->fprintf_func(dinfo->stream, "$%X", addr);
    };

    int len = print_insn_m68k(0, &di);
    fprintf(di.stream, "\nInstruction length: %d\n", len);
    return 0;
}
#endif
