
#include "68k.h"
#include "rotate.t.h"
#include "shift.t.h"
#include "opcodes.t.h"
#include <cstdlib>
#include <stdio.h>

#include "Moira.h"
extern Moira *moira;

void Core_68k::process() { //execute next opcode
    if ( doubleFault ) {
        cpuHalted();
        sync(4);
        return; //cpu can recover from reset only
    }

    try {
        group1exceptions();
        if (stop) {
            sampleIrq();
            sync(2);
            return;
        }
        trace = reg_s.trace;
        incrementPc();
        logInstruction(reg_ird, true);

        uint32_t ird = reg_ird;
        (this->*opcodes[ reg_ird ])(reg_ird);

        moira->process(ird);

    } catch(CpuException) {
        //bus or address error, leave opcode or exception handler
        status_code.reset();
    }
}

void Core_68k::power() {
    reg_usp = 0;
    for(int i = 0; i < 8; i++) {
        reg_d[i] = reg_a[i] = 0;
    }
    reset();

    moira->power();
}

void Core_68k::reset() {
    status_code.reset();

    irqPendingLevel = irqSamplingLevel = 0;

    rmwCycle = false;
    stop = false;
    doubleFault = false;
    trace = false;
    illegalMode = NONE;
    reg_s = 0x2700;
    sync(16);
    reg_a[7] = reg_ssp = readLong(0);
    reg_pc = readLong(4);
    fullprefetch();

    moira->reset();
}

void Core_68k::switchToUser() {
    if (!reg_s.s) {
        reg_ssp = reg_a[7];
        reg_a[7] = reg_usp;
    }
}

#define core68k_build_op_with_param(_op, _size, _param) \
    if (_size == SizeByte)       opcodes[opcode] = &Core_68k::op_##_op<SizeByte, _param>; \
    else if (_size == SizeWord)  opcodes[opcode] = &Core_68k::op_##_op<SizeWord, _param>; \
    else if (_size == SizeLong)  opcodes[opcode] = &Core_68k::op_##_op<SizeLong, _param>;

#define core68k_build_op(_op, _size) \
    if (_size == SizeByte)       opcodes[opcode] = &Core_68k::op_##_op<SizeByte>; \
    else if (_size == SizeWord)  opcodes[opcode] = &Core_68k::op_##_op<SizeWord>; \
    else if (_size == SizeLong)  opcodes[opcode] = &Core_68k::op_##_op<SizeLong>;


void Core_68k::build_optable() {
    interrupt = AUTO_VECTOR;

    for (unsigned x=0; x < 0x10000; x++) {
		opcodes[x] = &Core_68k::op_illegal;
	}
    unsigned i, j, k, l, m;

	/* LSL, LSR, ASL, ASR reg */
	for (i = 0; i <= 7; i++) {
		for (k = 0; k <= 2; k++) {
			for (l = 0; l <= 1; l++) {
				for (m = 0; m <= 7; m++) {
					opcodes[(i << 9) + (0 << 8) + (k << 6) + (l << 5) + m + 0xe008] = &Core_68k::op_xsx<false, false, false>;
					opcodes[(i << 9) + (1 << 8) + (k << 6) + (l << 5) + m + 0xe008] = &Core_68k::op_xsx<true, false, false>;

					opcodes[(i << 9) + (0 << 8) + (k << 6) + (l << 5) + m + 0xe000] = &Core_68k::op_xsx<false, false, true>;
					opcodes[(i << 9) + (1 << 8) + (k << 6) + (l << 5) + m + 0xe000] = &Core_68k::op_xsx<true, false, true>;
				}
			}
		}
	}
	/* LSL, LSR, ASL, ASR mem */
	for (j = 0; j <= 0x3f; j++) {
        if (!CheckEA(j, (char *)"001111111000")) continue;
		opcodes[0xe2c0 + (0 << 8) + j] = &Core_68k::op_xsx<false, true, false>;
		opcodes[0xe2c0 + (1 << 8) + j] = &Core_68k::op_xsx<true, true, false>;

		opcodes[0xe0c0 + (0 << 8) + j] = &Core_68k::op_xsx<false, true, true>;
		opcodes[0xe0c0 + (1 << 8) + j] = &Core_68k::op_xsx<true, true, true>;
	}

	/* ROL, ROR, ROXL, ROXR reg */
	for (i = 0; i <= 7; i++) {
		for (k = 0; k <= 2; k++) {
			for (l = 0; l <= 1; l++) {
				for (m = 0; m <= 7; m++) {
					opcodes[(i << 9) + (0 << 8) + (k << 6) + (l << 5) + m + 0xe018] = &Core_68k::op_rox<false, false, false>;
					opcodes[(i << 9) + (1 << 8) + (k << 6) + (l << 5) + m + 0xe018] = &Core_68k::op_rox<true, false, false>;

					opcodes[(i << 9) + (0 << 8) + (k << 6) + (l << 5) + m + 0xe010] = &Core_68k::op_rox<false, false, true>;
					opcodes[(i << 9) + (1 << 8) + (k << 6) + (l << 5) + m + 0xe010] = &Core_68k::op_rox<true, false, true>;
				}
			}
		}
	}
	/* ROL, ROR, ROXL, ROXR mem */
	for (j = 0; j <= 0x3f; j++) {
        if (!CheckEA(j, (char *)"001111111000")) continue;
		opcodes[0xe6c0 + (0 << 8) + j] = &Core_68k::op_rox<false, true, false>;
		opcodes[0xe6c0 + (1 << 8) + j] = &Core_68k::op_rox<true, true, false>;

		opcodes[0xe4c0 + (0 << 8) + j] = &Core_68k::op_rox<false, true, true>;
		opcodes[0xe4c0 + (1 << 8) + j] = &Core_68k::op_rox<true, true, true>;
	}

	/* BCHG BCLR BSET */
    for (i = 0; i <= 7; i++) {           /* register */
        for (j = 0; j <= 0x3f; j++) {     /* EA */
            if (!CheckEA(j, (char *)"101111111000")) continue;
            opcodes[(i << 9) + j + 0x0140] = &Core_68k::op_bchg<true>;
			opcodes[(i << 9) + j + 0x0180] = &Core_68k::op_bclr<true>;
			opcodes[(i << 9) + j + 0x01c0] = &Core_68k::op_bset<true>;
		}
	}

    for (i = 0; i <= 0x3f; i++) {        /* EA */
        if (!CheckEA(i, (char *)"101111111000")) continue;
		opcodes[i + 0x0840] = &Core_68k::op_bchg<false>;
		opcodes[i + 0x0880] = &Core_68k::op_bclr<false>;
		opcodes[i + 0x08c0] = &Core_68k::op_bset<false>;
	}

	/* BTST */
    for (i = 0; i <= 7; i++) {           /* register */
        for (j = 0; j <= 0x3f; j++) {     /* EA */
            if (!CheckEA(j, (char *)"101111111111")) continue;
            opcodes[(i << 9) + j + 0x0100] = &Core_68k::op_btst<true>;
		}
	}

    for (i = 0; i <= 0x3f; i++) {        /* EA */
        if (!CheckEA(i, (char *)"101111111110")) continue; //manual shows immediate before PC indi.
		opcodes[i + 0x0800] = &Core_68k::op_btst<false>;
	}

	/* CLR */
	for(i = 0; i <= 0x3f; i++) {
        if (!CheckEA(i, (char *)"101111111000")) continue;
        opcodes[(0 << 6) + i + 0x4200] = &Core_68k::op_clr<SizeByte>;
        opcodes[(1 << 6) + i + 0x4200] = &Core_68k::op_clr<SizeWord>;
        opcodes[(2 << 6) + i + 0x4200] = &Core_68k::op_clr<SizeLong>;
	}

	/* NBCD */
	for (i = 0; i <= 0x3f; i++) {    /* EA */
        if (!CheckEA(i, (char *)"101111111000")) continue;
		opcodes[i + 0x4800] = &Core_68k::op_nbcd;
	}

	/* NEG NEGX */
	for (i = 0; i <= 0x3f; i++)	{			/* EA */
        if (!CheckEA(i, (char *)"101111111000")) continue;
        opcodes[((0 << 10) + (0 << 6) + i + 0x4000)] = &Core_68k::op_neg<true, SizeByte>;
        opcodes[((0 << 10) + (1 << 6) + i + 0x4000)] = &Core_68k::op_neg<true, SizeWord>;
        opcodes[((0 << 10) + (2 << 6) + i + 0x4000)] = &Core_68k::op_neg<true, SizeLong>;
        opcodes[((1 << 10) + (0 << 6) + i + 0x4000)] = &Core_68k::op_neg<false, SizeByte>;
        opcodes[((1 << 10) + (1 << 6) + i + 0x4000)] = &Core_68k::op_neg<false, SizeWord>;
        opcodes[((1 << 10) + (2 << 6) + i + 0x4000)] = &Core_68k::op_neg<false, SizeLong>;
	}

	/* NOT */
	for (i = 0; i <= 0x3f; i++) {
        if (!CheckEA(i, (char *)"101111111000")) continue;
        opcodes[(0 << 6) + i + 0x4600] = &Core_68k::op_not<SizeByte>;
        opcodes[(1 << 6) + i + 0x4600] = &Core_68k::op_not<SizeWord>;
        opcodes[(2 << 6) + i + 0x4600] = &Core_68k::op_not<SizeLong>;
	}

	/* SCC */
	for (i = 0; i <= 0x3f; i++) { /* EA */
        if (!CheckEA(i, (char *)"101111111000")) continue;
	    for (j = 0; j <= 0xf; j++) { /* condition */
			opcodes[(j << 8) + i + 0x50c0] = &Core_68k::op_scc;
		}
	}

	/* TAS */
	for (i = 0; i <= 0x3f; i++) {
        if (!CheckEA(i, (char *)"101111111000")) continue;
		opcodes[i + 0x4ac0] = &Core_68k::op_tas;
	}

	/* TST */
	for (i = 0; i <= 0x3f; i++) {
        if (!CheckEA(i, (char *)"101111111000")) continue;
        opcodes[(0 << 6) + i + 0x4a00] = &Core_68k::op_tst<SizeByte>;
        opcodes[(1 << 6) + i + 0x4a00] = &Core_68k::op_tst<SizeWord>;
        opcodes[(2 << 6) + i + 0x4a00] = &Core_68k::op_tst<SizeLong>;
	}

	//ADD, ADDA, AND, CMP, CMPA, EOR, OR, SUB, SUBA
    for (i = 0x8; i <= 0xd; i++) {           /* operation */
        for (j = 0; j <= 7; j++)  {          /* Dn, An */
            for (k = 0; k <= 7; k++) {       /* opmode */
                for (l = 0; l <= 0x3f; l++) {/* EA */
                    Arithmetic((i << 12) + (j << 9) + (k << 6) + l);
				}
			}
		}
	}
    /* MULU, MULS, DIVU, DIVS */
    for (j = 0; j <= 0x3f; j++) {			/* EA */
        if (!CheckEA(j, "101111111111")) continue;
		for (i = 0; i <= 7; i++) {       /* register */
			opcodes[(i << 9) + j + 0xc0c0] = &Core_68k::op_mulu;
			opcodes[(i << 9) + j + 0xc1c0] = &Core_68k::op_muls;
			opcodes[(i << 9) + j + 0x80c0] = &Core_68k::op_divu;
			opcodes[(i << 9) + j + 0x81c0] = &Core_68k::op_divs;
		}
	}
	/* MOVE */
    for (j = 0; j <= 0x3f; j++) {     /* dest. EA */
        u8 checkJ = ( (j & 7) << 3 ) | ( (j >> 3) & 7 );
        if (!CheckEA(checkJ, (char *)"101111111000")) continue;
        for (k = 0; k <= 0x3f; k++) { /* source EA */
            if (!CheckEA(k, (char *)"111111111111")) continue;

            if ((k >> 3) != 1) {
                opcodes[(1 << 12) + (j << 6) + k] = &Core_68k::op_move<SizeByte>;
            }
            opcodes[(3 << 12) + (j << 6) + k] = &Core_68k::op_move<SizeWord>;
            opcodes[(2 << 12) + (j << 6) + k] = &Core_68k::op_move<SizeLong>;
        }
    }
	/* MOVEA */
    for (j = 0; j <= 7; j++) {       /* dest. reg */
        for (k = 0; k <= 0x3f; k++) { /* source EA */
            if (!CheckEA(k, (char *)"111111111111")) continue;
            opcodes[(3 << 12) + (j << 9) + k + 0x0040] = &Core_68k::op_movea<SizeWord>;
            opcodes[(2 << 12) + (j << 9) + k + 0x0040] = &Core_68k::op_movea<SizeLong>;
        }
    }
	/* ADDI, ANDI, CMPI, EORI, ORI, SUBI   */
	for (i = 0; i <= 0xc; i++) {          /* operation */
        for (j = 0; j <= 2; j++) {       /* size */
            for (k = 0; k <= 0x3f; k++) { /* EA */
                ArithmeticI((i << 8) + (j << 6) + k);
			}
		}
	}

	/* ADDQ */
	for (i = 0; i <= 7; i++) {               /* data */
        for (j = 0; j <= 1; j++) {           /* ADDQ/SUBQ */
            for (k = 0; k <= 2; k++) {       /* size */
                for (l = 0; l <= 0x3f; l++) {/* EA */
                    ArithmeticQ((i << 9) + (j << 8) + (k << 6) + l + 0x5000);
				}
			}
		}
	}

	/* MOVEQ */
    for (i = 0; i < 8; i++) {
        for (j = 0; j <= 0xff; j++) {
            opcodes[(i << 9) + 0x7000 + j] = &Core_68k::op_moveq;
        }
	}

	/* bcc */
	for (i = 2; i <= 0xf; i++) {
        for (j = 0; j <= 0xff; j++) {
            opcodes[((i << 8) + j + 0x6000)] = &Core_68k::op_bcc;
		}
	}
	/* bra */
	for (i = 0; i <= 0xff; i++) {
		opcodes[i + 0x6000] = &Core_68k::op_bra;
	}
	/* bsr */
	for (i = 0; i <= 0xff; i++) {
		opcodes[i + 0x6100] = &Core_68k::op_bsr;
	}
	/* dbcc */
	for (i = 0; i <= 0xf; i++) {         /* condition */
		for (j = 0; j <= 7; j++) {       /* register */
			opcodes[(i << 8) + j + 0x50c8] = &Core_68k::op_dbcc;
		}
	}
	/* JMP */
	for (j = 0; j <= 0x3f; j++) {
        if (!CheckEA(j, (char *)"001001111011")) continue;
		opcodes[j + 0x4ec0] = &Core_68k::op_jmp;
	}

	/* JSR */
	for (j = 0; j <= 0x3f; j++) {
        if (!CheckEA(j, (char *)"001001111011")) continue;
		opcodes[j + 0x4e80] = &Core_68k::op_jsr;
	}

	/* LEA */
	for (k = 0; k <= 0x3f; k++)	{			/* EA */
        if (!CheckEA(k, (char *)"001001111011")) continue;
		for (i = 0; i <= 7; i++) {           /* register */
			opcodes[(i << 9) + k + 0x41c0] = &Core_68k::op_lea;
		}
	}
	/* PEA */
	for (j = 0; j <= 0x3f; j++) {
        if (!CheckEA(j, (char *)"001001111011")) continue;
		opcodes[j + 0x4840] = &Core_68k::op_pea;
	}
	/* MOVEM */
    for (k = 0; k <= 0x3f; k++) { /* EA */
        if (CheckEA(k, (char *)"001011111000")) {
            opcodes[(0 << 10) + (0 << 6) + k + 0x4880] = &Core_68k::op_movem<SizeWord, false>;
            opcodes[(0 << 10) + (1 << 6) + k + 0x4880] = &Core_68k::op_movem<SizeLong, false>;
        }
        if (CheckEA(k, (char *)"001101111011")) {
            opcodes[(1 << 10) + (0 << 6) + k + 0x4880] = &Core_68k::op_movem<SizeWord, true>;
            opcodes[(1 << 10) + (1 << 6) + k + 0x4880] = &Core_68k::op_movem<SizeLong, true>;
        }
    }
    /* ADDX SUBX */
    for (i = 0; i <= 7; i++) {       /* Rx */
        for (k = 0; k <= 7; k++) {   /* Ry */
            opcodes[(i << 9) + (0 << 3) + (0 << 6) +  k + 0xd100] = &Core_68k::op_addx<SizeByte, false>;
            opcodes[(i << 9) + (0 << 3) + (1 << 6) +  k + 0xd100] = &Core_68k::op_addx<SizeWord, false>;
            opcodes[(i << 9) + (0 << 3) + (2 << 6) +  k + 0xd100] = &Core_68k::op_addx<SizeLong, false>;
            opcodes[(i << 9) + (1 << 3) + (0 << 6) +  k + 0xd100] = &Core_68k::op_addx<SizeByte, true>;
            opcodes[(i << 9) + (1 << 3) + (1 << 6) +  k + 0xd100] = &Core_68k::op_addx<SizeWord, true>;
            opcodes[(i << 9) + (1 << 3) + (2 << 6) +  k + 0xd100] = &Core_68k::op_addx<SizeLong, true>;

            opcodes[(i << 9) + (0 << 3) + (0 << 6) +  k + 0x9100] = &Core_68k::op_subx<SizeByte, false>;
            opcodes[(i << 9) + (0 << 3) + (1 << 6) +  k + 0x9100] = &Core_68k::op_subx<SizeWord, false>;
            opcodes[(i << 9) + (0 << 3) + (2 << 6) +  k + 0x9100] = &Core_68k::op_subx<SizeLong, false>;
            opcodes[(i << 9) + (1 << 3) + (0 << 6) +  k + 0x9100] = &Core_68k::op_subx<SizeByte, true>;
            opcodes[(i << 9) + (1 << 3) + (1 << 6) +  k + 0x9100] = &Core_68k::op_subx<SizeWord, true>;
            opcodes[(i << 9) + (1 << 3) + (2 << 6) +  k + 0x9100] = &Core_68k::op_subx<SizeLong, true>;
        }
	}
	/* CMPM */
    for (i = 0; i <= 7; i++) {       /* Ax */
        for (k = 0; k <= 7; k++) {   /* Ay */
            opcodes[(i << 9) + (0 << 6) + k + 0xb108] = &Core_68k::op_cmpm<SizeByte>;
            opcodes[(i << 9) + (1 << 6) + k + 0xb108] = &Core_68k::op_cmpm<SizeWord>;
            opcodes[(i << 9) + (2 << 6) + k + 0xb108] = &Core_68k::op_cmpm<SizeLong>;
        }
	}
    /* ABCD SBCD */
    for (i = 0; i <= 7; i++) {       /* Rx */
        for (k = 0; k <= 7; k++) {   /* Ry */
            opcodes[(i << 9) + (0 << 3) + k + 0xc100] = &Core_68k::op_abcd<false>;
            opcodes[(i << 9) + (1 << 3) + k + 0xc100] = &Core_68k::op_abcd<true>;
            opcodes[(i << 9) + (0 << 3) + k + 0x8100] = &Core_68k::op_sbcd<false>;
            opcodes[(i << 9) + (1 << 3) + k + 0x8100] = &Core_68k::op_sbcd<true>;
        }
	}
	/* ANDI to CCR */
	opcodes[0x023c] = &Core_68k::op_andiccr;
	/* ANDI to SR */
	opcodes[0x027c] = &Core_68k::op_andisr;
	/* CHK */
	for (k = 0; k <= 0x3f; k++)	{			/* EA */
        if (!CheckEA(k, (char *)"101111111111")) continue;
		for (i = 0; i <= 7; i++) {           /* register */
			opcodes[(i << 9) + (3 << 7) + k + 0x4000] = &Core_68k::op_chk;
		}
	}
	/* ORI to CCR */
	opcodes[0x003c] = &Core_68k::op_oriccr;
	/* ORI to SR */
	opcodes[0x007c] = &Core_68k::op_orisr;
	/* EORI to CCR */
    opcodes[0x0a3c] = &Core_68k::op_eoriccr;
	/* EORI to SR */
    opcodes[0x0a7c] = &Core_68k::op_eorisr;
	/* MOVE FROM SR */
	for (i = 0; i <= 0x3f; i++) {
        if (!CheckEA(i, (char *)"101111111000")) continue;
		opcodes[i + 0x40c0] = &Core_68k::op_movefromsr;
	}
	/* MOVE TO CCR */
	for (i = 0; i <= 0x3f; i++) {
        if (!CheckEA(i, (char *)"101111111111")) continue;
        opcodes[i + 0x44c0] = &Core_68k::op_movetoccr;
	}
	/* MOVE TO SR */
	for (i = 0; i <= 0x3f; i++) {
        if (!CheckEA(i, (char *)"101111111111")) continue;
		opcodes[i + 0x46c0] = &Core_68k::op_movetosr;
	}
	/* EXG */
	for (i = 0; i <= 7; i++) {           /* Rx */
		for (j = 0; j <= 7; j++) {       /* Ry */
            opcodes[0xc100 + (i << 9) + (8 << 3) + j] = &Core_68k::op_exg<8>;
            opcodes[0xc100 + (i << 9) + (9 << 3) + j] = &Core_68k::op_exg<9>;
            opcodes[0xc100 + (i << 9) + (17 << 3) + j] = &Core_68k::op_exg<17>;
		}
	}
	/* EXT */
	for (i = 0; i <= 7; i++) {           /* Rx */
        opcodes[0x4800 + (2 << 6) + i] = &Core_68k::op_ext<false>;
        opcodes[0x4800 + (3 << 6) + i] = &Core_68k::op_ext<true>;
	}
	/* LINK */
	for (i = 0; i <= 7; i++) {
		opcodes[0x4e50 + i] = &Core_68k::op_link;
	}
	/* MOVE FROM (TO) USP */
	for (i = 0; i <= 7; i++) {
        opcodes[0x4e60 + (1 << 3) + i] = &Core_68k::op_moveusp<true>;
        opcodes[0x4e60 + (0 << 3) + i] = &Core_68k::op_moveusp<false>;
	}
	/* NOP */
	opcodes[0x4e71] = &Core_68k::op_nop;
	/* RESET */
	opcodes[0x4e70] = &Core_68k::op_reset;
	/* RTE */
	opcodes[0x4e73] = &Core_68k::op_rte;
	/* RTR */
	opcodes[0x4e77] = &Core_68k::op_rtr;
	/* RTS */
	opcodes[0x4e75] = &Core_68k::op_rts;
	/* STOP */
	opcodes[0x4e72] = &Core_68k::op_stop;
	/* Swap */
	for (i = 0; i <= 7; i++) {
		opcodes[0x4840 + i] = &Core_68k::op_swap;
	}
	/* TRAP */
	for (i = 0; i <= 0xF; i++) {
        opcodes[0x4e40 + i] = &Core_68k::op_trap;
	}
	/* TRAPV */
	opcodes[0x4e76] = &Core_68k::op_trapv;
	/* UNLK */
	for (i = 0; i <= 7; i++) {
		opcodes[0x4e58 + i] = &Core_68k::op_unlk;
	}
	/* MOVEP */
    for (i = 0; i <= 7; i++) {       /* Dx */
        for (k = 0; k <= 7; k++) {   /* Ax */
            opcodes[(i << 9) + (4 << 6) + k + 0x0008] = &Core_68k::op_movep<4>;
            opcodes[(i << 9) + (5 << 6) + k + 0x0008] = &Core_68k::op_movep<5>;
            opcodes[(i << 9) + (6 << 6) + k + 0x0008] = &Core_68k::op_movep<6>;
            opcodes[(i << 9) + (7 << 6) + k + 0x0008] = &Core_68k::op_movep<7>;
        }
	}
}

void Core_68k::ArithmeticQ(u16 opcode) {
	u8 ea = opcode & 0x3f;
    if (!CheckEA(ea, (char *)"111111111000")) return;
	u8 size = (opcode >> 6) & 3;
	if (size == SizeByte && ((ea >> 3) == 1) ) return;

    if ( ( ( opcode >> 3) & 7 ) == 1 ) {
        size = SizeLong; //A reg, word behaves like long
    }

    switch ((opcode >> 8) & 1) {
        case 0: core68k_build_op(addq, size); return;
        case 1: core68k_build_op(subq, size); return;
	}
}

void Core_68k::ArithmeticI(u16 opcode) {
	u8 ea = opcode & 0x3f;

    if (!CheckEA(ea, (char *)"101111111000")) return;
    u8 size = (opcode >> 6) & 3;

	switch ((opcode >> 8) & 0xf) {
        case 0x0: core68k_build_op(ori, size); return;
        case 0x2: core68k_build_op(andi, size); return;
        case 0x4: core68k_build_op(subi, size); return;
        case 0x6: core68k_build_op(addi, size); return;
        case 0xa: core68k_build_op(eori, size); return;
        case 0xc: core68k_build_op(cmpi, size); return;
	}
}

void Core_68k::Arithmetic(u16 opcode) {
	u8 opmode = (opcode >> 6) & 7;
	u8 ea = opcode & 0x3f;

	switch (opmode & 3) {
		default: break;
		case 3: { //ADDA, SUBA, CMPA
            switch (opcode >> 12) {
                case 0x9:
                    if (opmode & 4) opcodes[opcode] = &Core_68k::op_suba<SizeLong>;
                    else            opcodes[opcode] = &Core_68k::op_suba<SizeWord>;
                    return;
                case 0xB:
                    if (opmode & 4) opcodes[opcode] = &Core_68k::op_cmpa<SizeLong>;
                    else            opcodes[opcode] = &Core_68k::op_cmpa<SizeWord>;
                    return;
                case 0xD:
                    if (opmode & 4) opcodes[opcode] = &Core_68k::op_adda<SizeLong>;
                    else            opcodes[opcode] = &Core_68k::op_adda<SizeWord>;
                    return;
                default: return;
            }
        }
	}

	if ( opmode & 4 ) { //ea is destination
        opmode &= 3;
		switch (opcode >> 12) {
			default: return;
			case 0x8: //or
                if (!CheckEA(ea, (char *)"001111111000")) return;
                core68k_build_op_with_param(or, opmode, true);
				return;
			case 0x9: //sub
                if (!CheckEA(ea, (char *)"001111111000")) return;
                core68k_build_op_with_param(sub, opmode, true);
				return;
			case 0xB: //eor
                if (!CheckEA(ea, (char *)"101111111000")) return;
                core68k_build_op(eor, opmode);
				return;
			case 0xC: //and
                if (!CheckEA(ea, (char *)"001111111000")) return;
                core68k_build_op_with_param(and, opmode, true);
				return;
			case 0xD: //add
                if (!CheckEA(ea, (char *)"001111111000")) return;
                core68k_build_op_with_param(add, opmode, true);
				return;
		}
	} else {  //ea is source
        opmode &= 3;
		switch (opcode >> 12) {
			default: return;
			case 0x8: //or
                if (!CheckEA(ea, (char *)"101111111111")) return;
                core68k_build_op_with_param(or, opmode, false);
				return;
			case 0x9: //sub
                if (!CheckEA(ea, (char *)"111111111111")) return;
				if ( ((ea >> 3) == 1) && ((opmode & 3) == 0) ) return;
                core68k_build_op_with_param(sub, opmode, false);
				return;
			case 0xB: //cmp
                if (!CheckEA(ea, (char *)"111111111111")) return;
				if ( ((ea >> 3) == 1) && ((opmode & 3) == 0) ) return;
                core68k_build_op(cmp, opmode);
				return;
			case 0xC: //and
                if (!CheckEA(ea, (char *)"101111111111")) return;
                core68k_build_op_with_param(and, opmode, false);
				return;
			case 0xD: //add
                if (!CheckEA(ea, (char *)"111111111111")) return;
				if ( ((ea >> 3) == 1) && ((opmode & 3) == 0) ) return;
                core68k_build_op_with_param(add, opmode, false);
				return;
		}
	}
}

void Core_68k::setFlags(u8 type, u8 size, u64 result, u32 src, u32 dest) {

	bool ResN = !! (result & msb_(size));
	bool DestN = !! (dest & msb_(size));
	bool SrcN = !! (src & msb_(size));

	switch(type) {
		case flag_logical:
			reg_s.v = reg_s.c = 0;
            reg_s.z = maskVal_((u32)result, size) == 0;
			reg_s.n = ResN; return;
        case flag_sub:
        case flag_cmp:
            reg_s.z = maskVal_((u32)result, size) == 0;
            reg_s.n = ResN;
        case flag_subx:
            reg_s.c = (result >> bits_(size)) & 1;
            if (type != flag_cmp) reg_s.x = reg_s.c;
            reg_s.v = ( SrcN ^ DestN ) & ( ResN ^ DestN ); return;
		case flag_add:
            reg_s.z = maskVal_((u32)result, size) == 0;
			reg_s.n = ResN;
        case flag_addx:
            reg_s.c = (result >> bits_(size)) & 1;
			reg_s.x = reg_s.c;
			reg_s.v = ( SrcN ^ ResN ) & ( DestN ^ ResN ); return;
		case flag_zn:
            reg_s.z = reg_s.z & ( maskVal_((u32)result, size) == 0 );
			reg_s.n = ResN; return;
	}
}

void Core_68k::writeEA(u8 size, u32 value, bool lastBusCycle) {
	if (eaReg) {
		switch(size) {
			case SizeByte: eaReg->l = value & 0xFF; return;
			case SizeWord: eaReg->w = value & 0xFFFF; return;
            case SizeLong: eaReg->d = value; return;
			default: return;
		}
	}

	switch(size) {
        case SizeByte: writeByte(eaAddr, value & 0xFF, lastBusCycle); return;
        case SizeWord: writeWord(eaAddr, value & 0xFFFF, lastBusCycle); return;
        case SizeLong: writeLong(eaAddr, value, lastBusCycle); return;
	}
}

u32 Core_68k::LoadEA(u8 size, u8 ea, bool noReadFromEA, bool fetchLastExtension, bool noSyncDec) {
	u32 operand = 0;
	u8 displacement;
	eaAddr = 0;
	eaReg = 0;
    adm = UNSELECT;
    u32 dispReg;
	u8 regPos = ea & 7;
	u8 mode = (ea >> 3) & 7;

	switch(mode) {
		// Register direct
		case 0: //Dn
			adm = DR_DIRECT;
			eaReg = &reg_d[regPos];
            return maskVal_(reg_d[regPos], size);
		case 1: //An
			adm = AR_DIRECT;
			eaReg = &reg_a[regPos];
            return  maskVal_(reg_a[regPos], size);
		// Register indirect
		case 2: //(An)
            adm = AR_INDIRECT;
            eaAddr = reg_a[regPos];
            break;
		case 3: //(An)+
            adm = AR_INDIRECT_INC;
            eaAddr = reg_a[regPos];
            break;
        case 4: //-(An)
            adm = AR_INDIRECT_DEC;
            eaAddr = reg_a[regPos];

            if (size == SizeByte) {
                if (regPos == 7) eaAddr -= 2;
                else eaAddr -= 1;
            } else if (size == SizeWord) {
                eaAddr -= 2;
            } else {
                eaAddr -= 4;
            }

            if (!noSyncDec) sync(2);
            break;
		case 5: //(d16, An)
            adm = AR_INDIRECT_D16;
            eaAddr = reg_a[regPos] + (i32)(i16)reg_irc;
            if (fetchLastExtension) readExtensionWord();
            else { logInstruction(reg_irc, false); incrementPc(); }
			break;
		// Address Register Indirect with Index
		case 6:
			adm = AR_INDIRECT_D8;
            eaAddr = reg_a[regPos];
        d8Xn:
			displacement = reg_irc & 0xFF;
            dispReg = reg_irc & 0x8000 ? reg_a[(reg_irc & 0x7000) >> 12] : reg_d[(reg_irc & 0x7000) >> 12];
            eaAddr += !(reg_irc & 0x800) ? (i32)( (i16)(dispReg) ) : dispReg;
			eaAddr += (i32)(i8)displacement;
			sync(2);
            if (fetchLastExtension) readExtensionWord();
            else { logInstruction(reg_irc, false); incrementPc(); }
			break;

		case 7:
			switch(regPos) {
				case 0:  //absolute Word
					adm = ABS_SHORT;
                    eaAddr = (i32)(i16)reg_irc;
                    if (fetchLastExtension) {
                        // printf("DIRK: fetchLastExtension = %d\n", fetchLastExtension);
                        readExtensionWord();
                    } else {
                        // printf("DIRK: fetchLastExtension = %d\n", fetchLastExtension);
                        logInstruction(reg_irc, false); incrementPc();
                    }

					break;

				case 1: //absolute Long
					adm = ABS_LONG;
					eaAddr = reg_irc << 16;
                    readExtensionWord();
					eaAddr |= reg_irc;
                    if (fetchLastExtension) readExtensionWord();
                    else { logInstruction(reg_irc, false); incrementPc(); }
					break;

				case 2: //d16 PC, Program Counter Indirect with Displacement Mode
					adm = PC_INDIRECT_D16;
                    eaAddr = reg_pc + (i16)reg_irc;
                    if (fetchLastExtension) readExtensionWord();
                    else { logInstruction(reg_irc, false); incrementPc(); }
					break;

				case 3: //d8 PC Xn, Program Counter Indirect with Index (8-Bit Displacement) Mode
					adm = PC_INDIRECT_D8;
                    eaAddr = reg_pc;
					goto d8Xn;

				case 4: //immediate
					adm = IMMEDIATE;
					if (size == SizeByte) {
                        operand = reg_irc & 0xff;
					} else if (size == SizeWord) {
						operand = reg_irc;
					} else {
						operand = reg_irc << 16;
                        readExtensionWord();
						operand |= reg_irc;
					}
                    readExtensionWord();
					return operand;
			}
			break;
	}

    if (noReadFromEA) return eaAddr;

	switch(size) {
		case SizeByte: operand = readByte(eaAddr); break;
		case SizeWord: operand = readWord(eaAddr); break;
		case SizeLong: operand = readLong(eaAddr); break;
	}

	updateRegAForIndirectAddressing(size, regPos);

	return operand;
}

void Core_68k::updateRegAForIndirectAddressing(u8 size, u8 regPos) {

    if (adm == AR_INDIRECT_INC) {
        if (size == SizeByte) {
            if (regPos == 7) reg_a[regPos] += 2;
            else reg_a[regPos] += 1;
        } else if (size == SizeWord) {
            reg_a[regPos] += 2;
        } else {
            reg_a[regPos] += 4;
        }
	} else if (adm == AR_INDIRECT_DEC) {
        reg_a[regPos] = eaAddr;
	}
}

int Core_68k::CheckEA(u8 ea, const char* valid) {  /* EA = MMMRRR (mode/reg) */
	if (((ea >> 3) & 7) != 7) {
        if (valid[(ea >> 3) & 7] != '1')    return 0;
        else                                return 1;
    } else {
        if ((ea & 7) >= 5)  return 0;
        else {
            int count = ea & 7;
            //to easier match with documentation
            if (count == 2) count = 3;
            else if (count == 3) count = 4;
            else if (count == 4) count = 2;

            if (valid[7 + count] != '1')	return 0;
			else							return 1;
        }
    }
	return 0;
}


bool Core_68k::conditionalTest(u8 code) {
	switch(code & 0xF) {
		case 0: return true;
		case 1: return false;
		case 2: return !reg_s.c & !reg_s.z;
		case 3: return reg_s.c | reg_s.z;
		case 4: return !reg_s.c;
		case 5: return reg_s.c;
		case 6: return !reg_s.z;
		case 7: return reg_s.z;
		case 8: return !reg_s.v;
		case 9: return reg_s.v;
		case 10: return !reg_s.n;
		case 11: return reg_s.n;
        case 12: return !(reg_s.n ^ reg_s.v);
        case 13: return reg_s.n ^ reg_s.v;
        case 14: return (reg_s.n & reg_s.v & !reg_s.z) | (!reg_s.n & !reg_s.v & !reg_s.z);
        case 15: return reg_s.z | (reg_s.n & !reg_s.v) | (!reg_s.n & reg_s.v);
	}

    return false; 
}

int Core_68k::getDivu68kCycles (u32 dividend, u16 divisor) {

    if ((dividend >> 16) >= divisor) return 10; //quotient is bigger than 16bit
    u32 hdivisor = divisor << 16;

    int mcycles = 38;
    for (int i = 0; i < 15; i++) {
        if ((i32)dividend < 0) {
            dividend <<= 1;
            dividend -= hdivisor;
        } else {
            dividend <<= 1;
            mcycles += 2;
            if (dividend >= hdivisor) {
                dividend -= hdivisor;
                mcycles--;
            }
        }
    }
    return mcycles * 2;
}

int Core_68k::getDivs68kCycles (i32 dividend, i16 divisor) {
    int mcycles = 6;
    if (dividend < 0) mcycles++;

    if ((abs(dividend) >> 16) >= abs(divisor)) return (mcycles + 2) * 2;

    mcycles += 55;

    if (divisor >= 0) {
        if (dividend >= 0) mcycles--;
        else mcycles++;
    }

    u32 aquot = abs(dividend) / abs(divisor);
    for (int i = 0; i < 15; i++) {
        if ((i16)aquot >= 0) mcycles++;
        aquot <<= 1;
    }

    return mcycles * 2;
}

#undef core68k_build_op_with_param
#undef core68k_build_op
