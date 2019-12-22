#ifndef musashi_hpp
#define musashi_hpp

#include "stdint.h"

extern "C" {
#include "m68k.h"
#include "m68kcpu.h"
#include "m68kops.h"
}

extern "C" int interrupt_handler(int irqLevel);
extern "C" int read_sp_on_reset(void);
extern "C" int read_pc_on_reset(void);

extern uint8_t mem[0x10000];

void setupMusashi();
void setMem16(uint32_t addr, uint16_t word);

// Performs a disassembler test
void dasmTest();

#endif
