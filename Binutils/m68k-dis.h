#ifndef M68K_DIS_H
#define M68K_DIS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MACH_68000 2
#define MACH_68010 3
#define MACH_68020 4
#define MACH_68030 5
#define MACH_68040 6
#define MACH_68060 7

typedef unsigned char bfd_byte;
typedef unsigned int bfd_vma;
typedef int (*fprintf_ftype)(FILE*, const char*, ...);

enum dis_insn_type {
    dis_noninsn, /* Not a valid instruction.  */
    dis_nonbranch, /* Not a branch instruction.  */
    dis_branch, /* Unconditional branch.  */
    dis_condbranch, /* Conditional branch.  */
    dis_jsr, /* Jump to subroutine.  */
    dis_condjsr, /* Conditional jump to subroutine.  */
    dis_dref, /* Data reference instruction.  */
    dis_dref2 /* Two data references in instruction.  */
};

typedef struct disassemble_info {
    void* private_data;
    FILE* stream;
    unsigned long mach;
    bfd_vma target;
    enum dis_insn_type insn_type;

    void* caller_private;

    int (*read_memory_func)(bfd_vma memaddr, bfd_byte* myaddr, unsigned int length, struct disassemble_info* dinfo);
    void (*memory_error_func)(int status, bfd_vma memaddr, struct disassemble_info* dinfo);
    fprintf_ftype fprintf_func;
    void (*print_address_func)(bfd_vma addr, struct disassemble_info* dinfo);
} disassemble_info;

int print_insn_m68k(bfd_vma memaddr, disassemble_info* info);

#ifdef __cplusplus
}
#endif

#endif
