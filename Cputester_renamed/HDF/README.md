# Overview

The HDF files in this directory contain CPU test programs generated with Toni Wilen's cputester. To perform a test run in vAmiga, perform the followings steps:

- Install Kickstart v2.04
- Enable 8MB Fast Ram
- Select the CPU model under test (68000, 68010, 68020, 68030, or 68040)
- Drag the HDF file into the emulator and reboot
- When the CLI prompt appears, type `cputest all` or `cputest all -cycles`

Note:
 - Option `-cycles` enables cycle counting. If this option is used, FastRam MUST be enabled.
 - The test data is located in directory `data/x_default` where x indicates the CPU type (M68x00).

## Troubleshooting:

- "Couldn't allocate tmem area ..."

This error message indicates that no FastRam is enabled. Enable 8MB FastRam and reboot. 

-  "Couldn't list directy 'data/x_default'"

This error message indicates that the HDF does not contain test data for the emulated CPU type. Select the proper CPU and reboot. 

The HDFs have been generated with the following settings:

### simple_0x0.hdf

Supported CPUs: 68000, 68010, 68020, 68030, 68040

```
test_rounds=2
feature_flags_mode=0
feature_sr_mask=0x0000
feature_undefined_ccr=1
feature_full_extension_format=1
mode=all
```

### IRQ_0x0.hdf

Supported CPUs: 68000, 68010

```
feature_sr_mask=0x8000
feature_interrupts=1
feature_undefined_ccr=1
mode=jsr,jmp,bsr,bcc,dbcc,nop,exg,swap,stop,mvsr2,mv2sr,andsr,eorsr,orsr
min_opcode_test_rounds=100
```

### AESRC_0x0.hdf

Supported CPUs: 68000

```
feature_target_src_ea=0x37fff1,0x7111
feature_target_dst_ea=
feature_undefined_ccr=1
mode=all
```

### AEDST_000.hdf

Supported CPUs: 68000, 68010

```
feature_target_src_ea=
feature_target_dst_ea=0x37fff1,0x7111
verbose=0
feature_undefined_ccr=1
mode=move,movea,mvmel,mvmle
```

### AEDST_010.hdf

Supported CPUs: 68000, 68010

```
feature_target_src_ea=
feature_target_dst_ea=0x37fff1,0x7111
verbose=0
feature_undefined_ccr=1
mode=all
```

### ODDSTK_0x0.hdf

Supported CPUs: 68000, 68010

```
feature_usp=2
verbose=0
feature_undefined_ccr=1
mode=rts,rtd,rtr,jsr,bsr,link,unlk,pea
```

### ODDEXC_0x0.hdf

Supported CPUs: 68000, 68010

```
feature_exception_vectors=0x000123
verbose=0
feature_undefined_ccr=1
mode=mv2sr.w,mvusp2r,mvr2usp,illegal,chk,trap,trapv,divu,divs,orsr.w
```

### LOOP*n*_0x0.hdf

Supported CPUs: 68010

```
feature_loop_mode_68010=1
feature_loop_mode_cnt=n
min_opcode_test_rounds=100
feature_undefined_ccr=1
mode=all
```

### ODDIRQ_0x0.hdf

Supported CPUs: 68010

```
mode=nop,ext,swap
feature_interrupts=1
feature_undefined_ccr=1
feature_exception_vectors=0x000123
```

### BASIC1_0x0.hdf, BASIC2_0x0.hdf

Basic tests with all SR T1, T0 and M combinations

Supported CPUs: 68020

```
feature_sr_mask=0xC000 (BASIC1)
mode=all
```

```
feature_sr_mask=0x3000 (BASIC2)
mode=all
```

### FBASIC_0x0.hdf

Basic FPU test. No arithmetic exceptions, unsupported instructions or datatypes, denormals or unnormals.

```
feature_sr_mask=0xc000
exceptions=-48,-49,-50,-51,-52,-53,-54
min_opcode_test_rounds=5000
mode=fmove,fsmove,fdmove,fint,fintrz,fneg,fsneg,fdneg,fabs,fsabs,fdabs,fdiv,fsdiv,fddiv,fadd,fsadd,fdadd,fmul,fsmul,fdmul,fsgldiv,fsglmul,fsub,fssub,fdsub,fcmp,ftst,fsqrt
```
