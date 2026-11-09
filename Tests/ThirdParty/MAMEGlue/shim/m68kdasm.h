// Stand-in for MAME's disassembler header (the disassembler is not needed)
#pragma once
#include "emu.h"
class m68k_disassembler : public util::disasm_interface {
public:
    enum { TYPE_68000 };
    m68k_disassembler(int) { }
};
