#include <stdio.h>

//asl, asr, lsl, lsr, rol, ror, roxl, roxr								-> shift/rotate instructions
//bchg, bclr, bset, btst												-> bit manipulation instructions
//clr, nbcd, neg, negx, not, scc, tas, tst								-> single operand instructions
//add, adda, and, cmp, cmpa, sub, suba, or, eor, mulu, muls, divu, divs -> standard instructions
//move, movea															-> move instructions
//addi, addq, andi, cmpi, eori, ori, moveq, subi, subq					-> immediate instructions
//bcc, bra, bsr, dbcc													-> specificational instructions
//jmp, jsr, lea, pea, movem												-> misc 1 instructions
//addx, cmpm, subx, abcd, sbcd											-> multiprecision instructions
//andiccr, andisr, chk, eorisr, eoriccr, orisr, oriccr, move from sr
//move to ccr, move to sr, exg, ext, link, moveusp, nop, reset
//rte, rtr, rts, stop, swap, trap, trapv, unlk							-> misc 2 instructions
//movep																	-> peripheral instruction


//shift/rotate instruction
template<bool left, bool memory, bool arithmetic> void Core_68k::op_xsx(u16 opcode) {
	u32 data;
    u8 size = SizeWord;

	if (memory) {
		data = LoadEA(SizeWord, opcode & 0x3F);
        prefetch(false);
		u16 sign = 0x8000 & data;
		bool carry = left ? (0x8000 & data) != 0 : data & 1;
		data = left ? data << 1 : data >> 1;
		if (arithmetic && !left) data |= sign;
		u16 sign2 = 0x8000 & data;
        setFlags(flag_logical, SizeWord, data);
		reg_s.x = reg_s.c = carry;
		if (arithmetic && left) reg_s.v = sign != sign2;
	} else {
        prefetch(true);
        size = (opcode >> 6) & 3;
		bool ir = (opcode >> 5) & 1;
		u8 multi = (opcode >> 9) & 7;
		u8 regPos = opcode & 7;
        u8 shiftCount = !ir ? ( multi == 0 ? 8 : multi ) : reg_d[multi] & 63;
        data = LoadEA(size, regPos); //register direct
		switch (size) {
            case SizeByte: left ? shift_left<arithmetic, SizeByte>(shiftCount, data)
                                : shift_right<arithmetic, SizeByte>(shiftCount, data); break;
            case SizeWord: left ? shift_left<arithmetic, SizeWord>(shiftCount, data)
                                : shift_right<arithmetic, SizeWord>(shiftCount, data); break;
            case SizeLong: left ? shift_left<arithmetic, SizeLong>(shiftCount, data)
                                : shift_right<arithmetic, SizeLong>(shiftCount, data); break;
		}
		sync(2 + (size == SizeLong ? 2 : 0) + shiftCount * 2);
	}

    writeEA(size, data, true);
}

template<bool left, bool memory, bool extend> void Core_68k::op_rox(u16 opcode) {
	u32 data;
    u8 size = SizeWord;
	if (memory) {
		data = LoadEA(SizeWord, opcode & 0x3F);
        prefetch(false);
		bool carry = left ? (0x8000 & data) != 0 : data & 1;
		data = left ? data << 1 : data >> 1;
		if (extend) data = left ? data | (u8)reg_s.x	: data | (reg_s.x << 15);
		else		data = left ? data | (u8)carry		: data | (carry << 15);
		setFlags(flag_logical, SizeWord, data, 0, 0);
		reg_s.c = carry;
		if (extend) reg_s.x = carry;
	} else {
        prefetch(true);
        size = (opcode >> 6) & 3;
		bool ir = (opcode >> 5) & 1;
		u8 multi = (opcode >> 9) & 7;
		u8 regPos = opcode & 7;
        u8 shiftCount = !ir ? ( multi == 0 ? 8 : multi ) : reg_d[multi] & 63;
        data = LoadEA(size, regPos); //register direct
		switch (size) {
            case SizeByte: left ? rotate_left<extend, SizeByte>(shiftCount, data)
                                : rotate_right<extend, SizeByte>(shiftCount, data); break;
            case SizeWord: left ? rotate_left<extend, SizeWord>(shiftCount, data)
                                : rotate_right<extend, SizeWord>(shiftCount, data); break;
            case SizeLong: left ? rotate_left<extend, SizeLong>(shiftCount, data)
                                : rotate_right<extend, SizeLong>(shiftCount, data); break;
		}
		sync(2 + (size == SizeLong ? 2 : 0) + shiftCount * 2);
	}
    writeEA(size, data, true);
}

//bit manipulating instructions
template<bool dynamic> void Core_68k::op_bchg(u16 opcode) {
	u8 reg, bit;
	u8 size = ((opcode >> 3) & 7) == 0 ? SizeLong : SizeByte;

    if (dynamic) {
        reg = (opcode >> 9) & 7;
        bit = (size == SizeLong) ? reg_d[reg] & 31 : reg_d[reg] & 7;
    } else {
        bit = (size == SizeLong) ? reg_irc & 31 : reg_irc & 7;
        readExtensionWord();
    }

	u32 data = LoadEA(size, opcode & 0x3F);

	data ^= (1 << bit);
	reg_s.z = !! ((data & (1 << bit)) >> bit);
    prefetch(isRegisterMode());

    if (size == SizeLong) {
        sync(2);
        if (bit > 15) sync(2);
    }
    writeEA(size, data, true);
}

template<bool dynamic> void Core_68k::op_bclr(u16 opcode) {
	u8 reg, bit;
    u8 size = ((opcode >> 3) & 7) == 0 ? SizeLong : SizeByte;

    if (dynamic) {
        reg = (opcode >> 9) & 7;
        bit = (size == SizeLong) ? reg_d[reg] & 31 : reg_d[reg] & 7;
    } else {
        bit = (size == SizeLong) ? reg_irc & 31 : reg_irc & 7;
        readExtensionWord();
    }

	u32 data = LoadEA(size, opcode & 0x3F);

	reg_s.z = !!( 1 ^ ((data >> bit) & 1) );
	data &= ~(1 << bit);

    prefetch(isRegisterMode());

    if (size == SizeLong) {
        sync(4);
        if (bit > 15) sync(2);
    }
    writeEA(size, data, true);
}

template<bool dynamic> void Core_68k::op_bset(u16 opcode) {
    u8 reg, bit;
    u8 size = ((opcode >> 3) & 7) == 0 ? SizeLong : SizeByte;

    if (dynamic) {
        reg = (opcode >> 9) & 7;
        bit = (size == SizeLong) ? reg_d[reg] & 31 : reg_d[reg] & 7;
    } else {
        bit = (size == SizeLong) ? reg_irc & 31 : reg_irc & 7;
        readExtensionWord();
    }

	u32 data = LoadEA(size, opcode & 0x3F);

	reg_s.z = !!( 1 ^ ((data >> bit) & 1) );
	data |= (1 << bit);
    prefetch(isRegisterMode());

    if (size == SizeLong) {
        sync(2);
        if (bit > 15) sync(2);
    }
    writeEA(size, data, true);
}

template<bool dynamic> void Core_68k::op_btst(u16 opcode) {
    u8 reg, bit;
    u8 size = ((opcode >> 3) & 7) == 0 ? SizeLong : SizeByte;

    if (dynamic) {
        reg = (opcode >> 9) & 7;
        bit = (size == SizeLong) ? reg_d[reg] & 31 : reg_d[reg] & 7;
    } else {
        bit = (size == SizeLong) ? reg_irc & 31 : reg_irc & 7;
        readExtensionWord();
    }

	u32 data = LoadEA(size, opcode & 0x3F);

	reg_s.z = !! (1 ^ ((data >> bit) & 1));
    prefetch(true);
	if (size == SizeLong) sync(2);
}

//single operand instructions
template<u8 size> void Core_68k::op_clr(u16 opcode) {
	LoadEA(size, opcode & 0x3F);
	reg_s.z = 1;
	reg_s.n = reg_s.v = reg_s.c = 0;

    prefetch(isRegisterMode());
    if (adm == DR_DIRECT && size == SizeLong) sync(2);
    writeEA(size, 0, true);
}

void Core_68k::op_nbcd(u16 opcode) {
	u8 data = LoadEA(SizeByte, opcode & 0x3F);
	u16 resLo = - (data & 0xF) - reg_s.x;
	u16 resHi = - (data & 0xF0);
    u16 result, tmp_result;
    result = tmp_result = resHi + resLo;
    if (resLo > 9) result -= 6;
	reg_s.x = reg_s.c = (result & 0x1F0) > 0x90;
	if (reg_s.c) result -= 0x60;
    setFlags(flag_zn, SizeByte, result);
    reg_s.v = ( ((tmp_result & 0x80) == 0x80) && ((result & 0x80) == 0) );
    prefetch(isRegisterMode());
    if (adm == DR_DIRECT) sync(2);

    printf("p68k: nbcd result = %x\n", result);

    writeEA(SizeByte, result, true);
}

template<bool negx, u8 size> void Core_68k::op_neg(u16 opcode) {
	u8 diff = negx ? reg_s.x : 0;

	u32 data = LoadEA(size, opcode & 0x3F);
    u64 result = 0 - u64(data) - (u64)diff;

	if (negx) {
		setFlags(flag_subx, size, result, data, 0);
		setFlags(flag_zn, size, result, 0, 0);
	} else {
		setFlags(flag_sub, size, result, data, 0);
	}
    prefetch(isRegisterMode());
    if (adm == DR_DIRECT && size == SizeLong) sync(2);
    writeEA(size, (u32)result, true);
}

template<u8 size> void Core_68k::op_not(u16 opcode) {
	u32 data = LoadEA(size, opcode & 0x3F);
	data = ~data;
    setFlags(flag_logical, size, data);
    prefetch(isRegisterMode());
    if (adm == DR_DIRECT && size == SizeLong) sync(2);
    writeEA(size, data, true);
}

void Core_68k::op_scc(u16 opcode) {
	u8 cc = (opcode >> 8) & 0xF;
	LoadEA(SizeByte, opcode & 0x3F);
    u8 data = (conditionalTest(cc)) ? 0xff : 0;
    prefetch(isRegisterMode());
    if (data && adm == DR_DIRECT) sync(2);
    writeEA(SizeByte, data, true);
}

void Core_68k::op_tas(u16 opcode) {
    u32 data = LoadEA(SizeByte, opcode & 0x3F, true);

    if ( isMemoryOperand() ) {
        rmwCycle = true;
        data = readByte(data);
        updateRegAForIndirectAddressing(SizeByte, opcode & 7 );
    }
    data &= 0xff;

    setFlags(flag_logical, SizeByte, data);
	data |= 0x80;

    if (adm != DR_DIRECT) sync(2);
    writeEA(SizeByte, data, true);

    rmwCycle = false;
    prefetch(true);
}

template<u8 size> void Core_68k::op_tst(u16 opcode) {
	u32 data = LoadEA(size, opcode & 0x3F);
    setFlags(flag_logical, size, data);
    prefetch(true);
}

/* Standard Instructions */
template<u8 size, bool writeEa> void Core_68k::op_add(u16 opcode) {
	u8 dregPos = (opcode >> 9) & 7;
	u32 data = LoadEA(size, opcode & 0x3F);
    u64 result = u64(maskVal_(reg_d[dregPos], size)) + u64(data);

    setFlags(flag_add, size, result, data, reg_d[dregPos]);

    prefetch(!writeEa);

    if ( !writeEa ) {
        eaReg = &reg_d[dregPos];
        if (size == SizeLong) {
            sync(2);
            if ( !isMemoryOperand() ) sync(2);
        }
    }
    writeEA(size, (u32)result, true);
}

template<u8 size> void Core_68k::op_adda(u16 opcode) {
	u8 aregPos = (opcode >> 9) & 7;

	u32 data = LoadEA(size, opcode & 0x3F);
    u32 result;

    prefetch(true);

    if (size == SizeLong) {
        result = reg_a[aregPos] + data;
        sync(2);
        if (!isMemoryOperand()) sync(2);
    } else {
        result = reg_a[aregPos] + (i32)(i16)(data & 0xffff);
        sync(4);
    }
	eaReg = &reg_a[aregPos];
	writeEA(SizeLong, result);
}

template<u8 size, bool writeEa> void Core_68k::op_and(u16 opcode) {
	u8 dregPos = (opcode >> 9) & 7;
	u32 data = LoadEA(size, opcode & 0x3F);

	data &= reg_d[dregPos];
    setFlags(flag_logical, size, data);

    prefetch(!writeEa);
    if ( !writeEa ) {
        eaReg = &reg_d[dregPos];
        if (size == SizeLong) {
            sync(2);
            if ( !isMemoryOperand() ) sync(2);
        }
    }
    writeEA(size, data, true);
}

template<u8 size> void Core_68k::op_cmp(u16 opcode) {
	u8 dregPos = (opcode >> 9) & 7;
	u32 data = LoadEA(size, opcode & 0x3F);
    u64 result = u64(maskVal_(reg_d[dregPos], size)) - u64(data);

    setFlags(flag_cmp, size, result, data, reg_d[dregPos]);

    prefetch(true);
    if (size == SizeLong) sync(2);
}

template<u8 size> void Core_68k::op_cmpa(u16 opcode) {
	u8 aregPos = (opcode >> 9) & 7;
    u32 data = LoadEA(size, opcode & 0x3F);
    if (size == SizeWord) {
        data = (i32)(i16)(data & 0xffff);
    }
    u64 result = u64(reg_a[aregPos]) - (u64)data;

    setFlags(flag_cmp, SizeLong, result, data, reg_a[aregPos]);
    prefetch(true);
    sync(2);
}

template<u8 size, bool writeEa> void Core_68k::op_sub(u16 opcode) {
	u8 dregPos = (opcode >> 9) & 7;
    u64 result;

	u32 data = LoadEA(size, opcode & 0x3F);

    if ( !writeEa ) {
        result = u64(maskVal_(reg_d[dregPos], size)) - u64(data);
        setFlags(flag_sub, size, result, data, reg_d[dregPos]);
    } else {
        result = u64(data) - u64(maskVal_(reg_d[dregPos], size));
        setFlags(flag_sub, size, result, reg_d[dregPos], data);
    }

    prefetch(!writeEa);
    if ( !writeEa ) {
        eaReg = &reg_d[dregPos];
        if (size == SizeLong) {
            sync(2);
            if (!isMemoryOperand()) sync(2);
        }
    }
    writeEA(size, (u32)result, true);
}

template<u8 size> void Core_68k::op_suba(u16 opcode) {
	u8 aregPos = (opcode >> 9) & 7;
    u64 result;

	u32 data = LoadEA(size, opcode & 0x3F);
    prefetch(true);

    if (size == SizeLong) {
        result = reg_a[aregPos] - data;
        sync(2);
        if (!isMemoryOperand()) sync(2);
    } else {
        result = reg_a[aregPos] - (i32)(i16)(data & 0xffff);
        sync(4);
    }

	eaReg = &reg_a[aregPos];
	writeEA(SizeLong, (u32)result);
}

template<u8 size, bool writeEa> void Core_68k::op_or(u16 opcode) {
	u8 dregPos = (opcode >> 9) & 7;
	u32 data = LoadEA(size, opcode & 0x3F);

	data |= reg_d[dregPos];
	setFlags(flag_logical, size, data, 0, 0);

    prefetch(!writeEa);
    if ( !writeEa ) {
        eaReg = &reg_d[dregPos];
        if (size == SizeLong) {
            sync(2);
            if (!isMemoryOperand()) sync(2);
        }
    }

    writeEA(size, data, true);
}

template<u8 size> void Core_68k::op_eor(u16 opcode) {
	u8 dregPos = (opcode >> 9) & 7;
	u32 data = LoadEA(size, opcode & 0x3F);

	data ^= reg_d[dregPos];
	setFlags(flag_logical, size, data, 0, 0);

    prefetch(isRegisterMode());
    if ((adm == DR_DIRECT) && (size == SizeLong)) sync(4);
    writeEA(size, data, !isRegisterMode());
}

void Core_68k::op_mulu(u16 opcode) {
	u8 regPos = (opcode >> 9) & 7;
	u32 data = LoadEA(SizeWord, opcode & 0x3F);
    data &= 0xFFFF;
    u32 result = data * reg_d[regPos].w;
	setFlags(flag_logical, SizeLong, result, 0, 0);
    prefetch(true);
    sync(34); //subtract prefetch time

    while(data) {
        if (data & 1) sync(2);
        data >>= 1;
    }

	eaReg = &reg_d[regPos];
	writeEA(SizeLong, result);
}

void Core_68k::op_muls(u16 opcode) {
	u8 regPos = (opcode >> 9) & 7;
	u32 data = LoadEA(SizeWord, opcode & 0x3F);
    u32 result = (i16)data * (i16)reg_d[regPos].w;
	setFlags(flag_logical, SizeLong, result, 0, 0);
    prefetch(true);
    sync(34); //subtract prefetch time
    data = ( (data << 1) ^ data ) & 0xffff;

    while(data) {
        if (data & 1) sync(2);
        data >>= 1;
    }

    eaReg = &reg_d[regPos];
	writeEA(SizeLong, result);
}

void Core_68k::op_divu(u16 opcode) {
    u8 regPos = (opcode >> 9) & 7;
    reg_s.v = reg_s.n = reg_s.c = reg_s.z = 0;
    u32 data = LoadEA(SizeWord, opcode & 0x3F);
    if (data == 0) {
        sync(8);
        trapException( 5 );
        return;
    }
    u32 result = reg_d[regPos] / (data & 0xFFFF);
    u16 remainder = reg_d[regPos] % (data & 0xFFFF);
    sync( getDivu68kCycles (reg_d[regPos], data & 0xFFFF) - 4 ); //subtract prefetch time
    prefetch(true);
    if (result > 0xFFFF) {
        reg_s.v = reg_s.n = 1;
        return;
    }
    setFlags(flag_logical, SizeWord, result, 0, 0);
    result = (result & 0xFFFF) | (remainder << 16);
    eaReg = &reg_d[regPos];
    writeEA(SizeLong, result);
}

void Core_68k::op_divs(u16 opcode) {
	u8 regPos = (opcode >> 9) & 7;
	reg_s.v = reg_s.n = reg_s.c = reg_s.z = 0;
	u32 data = LoadEA(SizeWord, opcode & 0x3F);
	if (data == 0) {
		sync(8);
        trapException( 5 );
		return;
	}
    i32 result = (i32)((i64)(i32)(reg_d[regPos]) / (i64)(i16)(data & 0xFFFF));
    u16 remainder = (i64)(i32)(reg_d[regPos]) % (i64)(i16)(data & 0xFFFF);

    sync( getDivs68kCycles ((i32)(reg_d[regPos]), (i16)(data & 0xFFFF)) - 4); //subtract prefetch time
    prefetch(true);

	if ((result & 0xffff8000) != 0 && (result & 0xffff8000) != 0xffff8000) {
		reg_s.v = reg_s.n = 1;
        return;
    }
    setFlags(flag_logical, SizeWord, result, 0, 0);
    eaReg = &reg_d[regPos];
    writeEA(SizeLong, (result & 0xFFFF) | (remainder << 16));
}

template<u8 size> void Core_68k::op_move(u16 opcode) {
    u8 destEA = (((opcode >> 6) & 7) << 3) | ((opcode >> 9) & 7);
	u32 data = LoadEA(size, opcode & 0x3F);
    bool isClass2 = isMemoryOperand() && destEA == ABS_LONG;

    LoadEA(size, destEA, true, !isClass2, true);
	setFlags(flag_logical, size, data, 0, 0);
    if (adm == AR_INDIRECT_DEC) { //dest adm
        prefetch();
    }
    writeEA(size, data, adm == AR_INDIRECT_DEC);
    updateRegAForIndirectAddressing(size, (opcode >> 9) & 7 );

    if (adm != AR_INDIRECT_DEC) { //dest adm
        isClass2 ? fullprefetch(true) : prefetch(true);
    }
}

template<u8 size> void Core_68k::op_movea(u16 opcode) {
	u8 regPos = (opcode >> 9) & 7;
	u32 data = LoadEA(size, opcode & 0x3F);
    reg_a[regPos] = size == SizeWord ? (i32)(i16)data : data;
    prefetch(true);
}
//Immediate instructions
template<u8 size> void Core_68k::op_addi(u16 opcode) {
	u32 immediate = LoadEA(size, 0x3C);
	u32 data = LoadEA(size, opcode & 0x3F);

    u64 result = u64(immediate) + u64(data);

    setFlags(flag_add, size, result, immediate, data);

    prefetch(isRegisterMode());
    if ((size == SizeLong) && (adm == DR_DIRECT)) sync(4);
    writeEA(size, (u32)result, true);
}

template<u8 size> void Core_68k::op_addq(u16 opcode) {
	u8 immediate = (opcode >> 9) & 7;
	if (immediate == 0) immediate = 8;

	u32 data = LoadEA(size, opcode & 0x3F);
    u64 result = u64(immediate) + u64(data);

	if (adm != AR_DIRECT) setFlags(flag_add, size, result, immediate, data);

    prefetch(isRegisterMode());
    if (adm == AR_DIRECT) sync(4);
    else if (size == SizeLong && adm == DR_DIRECT) sync(4);

    writeEA(size, (u32)result, true);
}

template<u8 size> void Core_68k::op_subq(u16 opcode) {
    u8 immediate = (opcode >> 9) & 7;
    if (immediate == 0) immediate = 8;

    u32 data = LoadEA(size, opcode & 0x3F);
    u64 result = (u64)data - (u64)immediate;

    if (adm != AR_DIRECT) setFlags(flag_sub, size, result, immediate, data);

    prefetch(isRegisterMode());
    if (adm == AR_DIRECT) sync(4);
    else if (size == SizeLong && adm == DR_DIRECT) sync(4);

    writeEA(size, (u32)result, true);
}

template<u8 size> void Core_68k::op_andi(u16 opcode) {
	u32 immediate = LoadEA(size, 0x3C);
	u32 data = LoadEA(size, opcode & 0x3F);
	data &= immediate;
    setFlags(flag_logical, size, data, 0, 0);

    prefetch(isRegisterMode());
    if ((size == SizeLong) && (adm == DR_DIRECT)) sync(4);

    writeEA(size, data, true);
}

template<u8 size> void Core_68k::op_cmpi(u16 opcode) {
	u32 immediate = LoadEA(size, 0x3C);
	u32 data = LoadEA(size, opcode & 0x3F);
    u64 result = (u64)data - (u64)immediate;
	setFlags(flag_cmp, size, result, immediate, data);
    prefetch(true);
    if ((size == SizeLong) && (adm == DR_DIRECT)) sync(2);
}

template<u8 size> void Core_68k::op_eori(u16 opcode) {
	u32 immediate = LoadEA(size, 0x3C);
	u32 data = LoadEA(size, opcode & 0x3F);
	data ^= immediate;
	setFlags(flag_logical, size, data, 0, 0);

    prefetch(isRegisterMode());
    if ((size == SizeLong) && (adm == DR_DIRECT)) sync(4);

    writeEA(size, data, true);
}

template<u8 size> void Core_68k::op_ori(u16 opcode) {
	u32 immediate = LoadEA(size, 0x3C);
	u32 data = LoadEA(size, opcode & 0x3F);
	data |= immediate;
	setFlags(flag_logical, size, data, 0, 0);

    prefetch(isRegisterMode());
    if ((size == SizeLong) && (adm == DR_DIRECT)) sync(4);

    writeEA(size, data, true);
}

void Core_68k::op_moveq(u16 opcode) {
	u8 regPos = (opcode >> 9) & 7;
    u32 data = (i32)(i8)(opcode & 0xFF);
    reg_d[regPos] = data;
    setFlags(flag_logical, SizeLong, data, 0, 0);
    prefetch(true);
}

template<u8 size> void Core_68k::op_subi(u16 opcode) {
	u32 immediate = LoadEA(size, 0x3C);
	u32 data = LoadEA(size, opcode & 0x3F);
    u64 result = (u64)data - (u64)immediate;

	setFlags(flag_sub, size, result, immediate, data);

    prefetch(isRegisterMode());
    if ((size == SizeLong) && (adm == DR_DIRECT)) sync(4);

    writeEA(size, (u32)result, true);
}

/* specificational instructions */
void Core_68k::op_bcc(u16 opcode) {
	u8 displacement = opcode & 0xFF;
	u8 cond = (opcode >> 8) & 0xF;

	if (conditionalTest(cond)) {
        reg_pc += ( displacement == 0 ? (i16)reg_irc : (i8)displacement );
        printf("68k: Take branch reg_pc = %x reg_irc = %d displacement = %d\n", reg_pc, (i16)reg_irc, displacement);
		sync(2);
		if (displacement == 0) logInstruction(reg_irc, false);
        fullprefetch(true);
	} else {
        printf("68k: Fallthrough\n");
		sync(4);
        if (displacement == 0) {
            readExtensionWord();
        }
        prefetch(true);
	}
}

void Core_68k::op_bra(u16 opcode) {
	u8 displacement = opcode & 0xFF;
    reg_pc += ( displacement == 0 ? (i16)reg_irc : (i8)displacement );
	sync(2);
	if (displacement == 0) logInstruction(reg_irc, false);
    fullprefetch(true);
}

void Core_68k::op_bsr(u16 opcode) { //todo check stacked reg_pc at address error
	u8 displacement = opcode & 0xFF;

    sync(2);
    writeStack(displacement == 0 ? reg_pc + 2 : reg_pc);
    u32 reg_pc_new = reg_pc + ( displacement == 0 ? (i16)reg_irc : (i8)displacement );

    if (displacement == 0) logInstruction(reg_irc, false);
    reg_irc = readWord(reg_pc_new);
    reg_pc = reg_pc_new;

    prefetch(true);
}

void Core_68k::op_dbcc(u16 opcode) {
	u8 reg = opcode & 7;
	u8 cond = (opcode >> 8) & 0xF;
	logInstruction(reg_irc, false);

	if (!conditionalTest(cond)) {
        sync(2);
		reg_d[reg].w -= 1;
        if (i16(reg_d[reg].w) != -1) {
            reg_pc += (i16)reg_irc;
            fullprefetch(true);
            return;
        }
        readWord(reg_pc+2); //todo: dummy read
	} else {
	    sync(4);
	}

    incrementPc();
    fullprefetch(true);
}
/* misc 1 instructions */
void Core_68k::op_jmp(u16 opcode) {
	u32 addr = LoadEA(SizeLong, opcode & 0x3F, true, false);
    if ( (adm == AR_INDIRECT_D8) || (adm == PC_INDIRECT_D8) ) sync(4); //todo: dummy read?
    else if ( (adm == AR_INDIRECT_D16) || (adm == ABS_SHORT) || (adm == PC_INDIRECT_D16) ) sync(2);
	reg_pc = addr;
    fullprefetch(true);
}

void Core_68k::op_jsr(u16 opcode) {
	u32 addr = LoadEA(SizeLong, opcode & 0x3F, true, false);
    if ( (adm == AR_INDIRECT_D8) || (adm == PC_INDIRECT_D8) ) sync(4); //todo: dummy read?
    else if ( (adm == AR_INDIRECT_D16) || (adm == ABS_SHORT) || (adm == PC_INDIRECT_D16) ) sync(2);

    u32 pcNextOp = reg_pc;
    reg_pc = addr;
    fullprefetchFirstStep();
    writeStack(pcNextOp);
    prefetch(true);
}

void Core_68k::op_lea(u16 opcode) {
	u8 regPos = (opcode >> 9) & 7;
    reg_a[regPos] = LoadEA(SizeLong, opcode & 0x3F, true);
	if ( (adm == AR_INDIRECT_D8) || (adm == PC_INDIRECT_D8) ) sync(2);
    prefetch(true);
}

void Core_68k::op_pea(u16 opcode) {
	u32 addr = LoadEA(SizeLong, opcode & 0x3F, true);
	if ( (adm == AR_INDIRECT_D8) || (adm == PC_INDIRECT_D8) ) sync(2);
    bool isAbs = (adm == ABS_SHORT) || (adm == ABS_LONG);
    if (isAbs) {
        writeStack(addr);
        prefetch(true);
    } else {
        prefetch();
        writeStack(addr, true);
    }
}

template<u8 size, bool memToReg> void Core_68k::op_movem(u16 opcode) {
	u8 regPos = opcode & 7;
    u8 byteCount = size == SizeWord ? 2 : 4;
	u16 regMask = reg_irc;
    readExtensionWord();
	reg_32* eaReg;
    bool isBitSet;

	LoadEA(size, opcode & 0x3F, true, true, true);

    if (memToReg && (size == SizeLong) ) readWord(eaAddr);

	for (int i = 0; i < 16; i++) {
        if (!regMask) break;

        isBitSet = regMask & 1;
        regMask >>= 1;

        if (!isBitSet) continue;

		if (adm == AR_INDIRECT_DEC) eaReg = i > 7 ? &reg_d[(~(i-8)) & 7] : &reg_a[(~i) & 7];
		else eaReg = i > 7 ? &reg_a[i-8] : &reg_d[i];

        u32 _val = *eaReg;

        if (adm == AR_INDIRECT_DEC) {
            if (eaReg == &reg_a[regPos]) _val = reg_a[regPos];
            size == SizeWord ? writeWord(eaAddr, _val) : writeLong(eaAddr, _val);
            if (!regMask) {
                reg_a[regPos] = eaAddr;
                break;
            }
            eaAddr -= byteCount;
        } else if (adm == AR_INDIRECT_INC) {
            *eaReg = size == SizeWord ? (i32)(i16)readWord(eaAddr) : readLong(eaAddr);
            eaAddr += byteCount;

            if (!regMask) {
                reg_a[regPos] = eaAddr;
                break;
            }
        } else {
            if (memToReg) *eaReg = size == SizeWord ? (i32)(i16)readWord(eaAddr) : readLong(eaAddr);
            else size == SizeWord ? writeWord(eaAddr, *eaReg) : writeLong(eaAddr, *eaReg);

            eaAddr += byteCount;
            if (!regMask) break;
        }
	}
    if (memToReg && (size == SizeWord) ) readWord(eaAddr); //dummy read, but can cause a bus error
    prefetch(true);
}
//multiprecision instructions
template<u8 size, bool memTomem> void Core_68k::op_addx(u16 opcode) {
	u8 Ry = opcode & 7;
	u8 Rx = (opcode >> 9) & 7; //dest
	u32 mem_src, mem_dest;
    u64 result;

    if (!memTomem) {
        result = u64(maskVal_(reg_d[Rx], size)) + u64(maskVal_(reg_d[Ry], size)) + (u64)reg_s.x;
		setFlags(flag_addx, size, result, reg_d[Ry], reg_d[Rx]);
		eaReg = &reg_d[Rx];
	} else {
		mem_src = LoadEA(size, (4 << 3) | Ry );
		mem_dest = LoadEA(size, (4 << 3) | Rx, false, true, true);
        result = (u64)mem_dest + (u64)mem_src + (u64)reg_s.x;
		setFlags(flag_addx, size, result, mem_src, mem_dest);
	}

	setFlags(flag_zn, size, result, 0, 0);
    if (memTomem && (size == SizeLong) ) {
        writeWord(eaAddr + 2, result & 0xFFFF, false);
    }
    prefetch(!memTomem);
    if (!memTomem && (size == SizeLong) ) sync(4);

    if (memTomem && (size == SizeLong) ) {
        writeWord(eaAddr, (result >> 16) & 0xFFFF, true);
    } else {
        writeEA(size, (u32)result, true);
    }
}

template<u8 size> void Core_68k::op_cmpm(u16 opcode) {
	u8 RegAy = opcode & 7;
	u8 RegAx = (opcode >> 9) & 7;

	u32 src = LoadEA(size, (3 << 3) | RegAy);
	u32 dest = LoadEA(size, (3 << 3) | RegAx);
    u64 result = (u64)dest - (u64)src;

	setFlags(flag_cmp, size, result, src, dest);
    prefetch(true);
}

template<u8 size, bool memTomem> void Core_68k::op_subx(u16 opcode) {
	u8 Rx = opcode & 7;
	u8 Ry = (opcode >> 9) & 7; //dest
    u64 result;

    if (!memTomem) { //data - data
        result = u64(maskVal_(reg_d[Ry], size)) - u64(maskVal_(reg_d[Rx], size)) - (u64)reg_s.x;
		setFlags(flag_subx, size, result, reg_d[Rx], reg_d[Ry]);
		eaReg = &reg_d[Ry];
	} else {
        u32 mem_src = LoadEA(size, (4 << 3) | Rx );
        u32 mem_dest = LoadEA(size, (4 << 3) | Ry, false, true, true);

        result = (u64)mem_dest - (u64)mem_src - (u64)reg_s.x;
		setFlags(flag_subx, size, result, mem_src, mem_dest);
	}
    setFlags(flag_zn, size, result, 0, 0);

    if (memTomem && (size == SizeLong) ) {
        writeWord(eaAddr + 2, result & 0xFFFF, false);
    }
    prefetch(!memTomem);
    if (!memTomem && (size == SizeLong) ) sync(4);

    if (memTomem && (size == SizeLong) ) {
        writeWord(eaAddr, (result >> 16) & 0xFFFF, true);
    } else {
        writeEA(size, (u32)result, true);
    }
}

template<bool memTomem> void Core_68k::op_abcd(u16 opcode) {
	u8 Ry = opcode & 7;
	u8 Rx = (opcode >> 9) & 7; //dest
	u32 mem_src, mem_dest;
	u16 result, tmp_result;
	u16 result_lo, result_hi;

    if (!memTomem) { //data - data
        result_lo = (reg_d[Rx] & 0xF) + (reg_d[Ry] & 0xF) + reg_s.x;
        result_hi = (reg_d[Rx] & 0xF0) + (reg_d[Ry] & 0xF0);
		eaReg = &reg_d[Rx];
        prefetch(true);
		sync(2);
	} else {
		mem_src = LoadEA(SizeByte, (4 << 3) | Ry );
		mem_dest = LoadEA(SizeByte, (4 << 3) | Rx, false, true, true);
        result_lo = (mem_src & 0xF) + (mem_dest & 0xF) + reg_s.x;
        result_hi = (mem_src & 0xF0) + (mem_dest & 0xF0);
        prefetch(false);
	}
    result = tmp_result = result_hi + result_lo;
	if (result_lo > 9) result += 6;
	reg_s.c = (result & 0x3f0) > 0x90;
	if (reg_s.c) result += 0x60;
	reg_s.x = reg_s.c;
    reg_s.v = ((tmp_result & 0x80) == 0) && ((result & 0x80) == 0x80);
	setFlags(flag_zn, SizeByte, result, 0, 0);

    writeEA(SizeByte, result, true);
}

template<bool memTomem> void Core_68k::op_sbcd(u16 opcode) {
	u8 Rx = opcode & 7;
	u8 Ry = (opcode >> 9) & 7; //dest
	u32 mem_src, mem_dest;
	u16 result, tmp_result;
	u16 result_lo, result_hi;

    if (!memTomem) {
		mem_src = reg_d[Rx];
		mem_dest = reg_d[Ry];
		eaReg = &reg_d[Ry];
        prefetch(true);
        sync(2);
	} else {
		mem_src = LoadEA(SizeByte, (4 << 3) | Rx );
		mem_dest = LoadEA(SizeByte, (4 << 3) | Ry, false, true, true);
        prefetch(false);
	}

    result_lo = (mem_dest & 0xF) - (mem_src & 0xF) - reg_s.x;
    result_hi = (mem_dest & 0xF0) - (mem_src & 0xF0);
	result = tmp_result = result_hi + result_lo;
	int bcd = 0;
	if (result_lo & 0xF0) { result -= 6; bcd = 6; }
	if ((((mem_dest & 0xFF) - (mem_src & 0xFF) - reg_s.x) & 0x100) > 0xFF) { result -= 0x60; }
	reg_s.c = (((mem_dest & 0xFF) - (mem_src & 0xFF) - bcd - reg_s.x) & 0x300) > 0xFF;
	reg_s.x = reg_s.c;
    reg_s.v = ((tmp_result & 0x80) == 0x80) && ((result & 0x80) == 0);
	setFlags(flag_zn, SizeByte, result, 0, 0);

    // printf("p68k: SBCD result = %x\n", result);

    writeEA(SizeByte, result, true);
}
// misc 2 instructions
void Core_68k::op_chk(u16 opcode) {
	u8 regPos = (opcode >> 9) & 7;
	u32 data = LoadEA(SizeWord, opcode & 0x3F);
    prefetch(true);
    reg_s.v = reg_s.c = reg_s.n = 0;
    reg_s.z = (reg_d[regPos] & 0xffff) == 0;
    sync(4);
    if ((i16)reg_d[regPos] > (i16)data) {
		reg_s.n = 0;
		sync(4);
        trapException(6);
		return;
	}
    sync(2);
    if ((i16)reg_d[regPos] < 0 ) {
		reg_s.n = 1;
		sync(4);
        trapException(6);
		return;
    }
}

void Core_68k::op_eoriccr(u16 opcode) {
	u32 data = LoadEA(SizeByte, 7 << 3 | 4);
    sync(8);
	reg_s.l ^= (data & 0x1F);
    readWord(reg_pc+2); //dummy read
    prefetch(true);
}

void Core_68k::op_andiccr(u16 opcode) {
    u32 data = LoadEA(SizeByte, 7 << 3 | 4);
    sync(8);
    reg_s.l &= (data & 0x1F);
    readWord(reg_pc+2); //dummy read
    prefetch(true);
}

void Core_68k::op_oriccr(u16 opcode) {
    u32 data = LoadEA(SizeByte, 7 << 3 | 4);
    sync(8);
    reg_s.l |= (data & 0x1F);
    readWord(reg_pc+2); //dummy read
    prefetch(true);
}

void Core_68k::op_orisr(u16 opcode) {
    if (!reg_s.s) {
        setPrivilegeException();
        return;
    }
    u32 data = LoadEA(SizeWord, 7 << 3 | 4);
    sync(8);

    reg_s |= data & 0xFFFF;
    reg_s &= 0xA71F;
    switchToUser();

    readWord(reg_pc+2); //dummy read
    prefetch(true);
}

void Core_68k::op_eorisr(u16 opcode) {
	if (!reg_s.s) {
        setPrivilegeException();
		return;
	}
	u32 data = LoadEA(SizeWord, 7 << 3 | 4);
    sync(8);

	reg_s ^= data & 0xFFFF;
	reg_s &= 0xA71F;

    switchToUser();
    readWord(reg_pc+2); //dummy read
    prefetch(true);
}

void Core_68k::op_andisr(u16 opcode) {
    if (!reg_s.s) {
        setPrivilegeException();
        return;
    }
    u32 data = LoadEA(SizeWord, 7 << 3 | 4);
    sync(8);

    reg_s &= data & 0xFFFF;
    reg_s &= 0xA71F;

    switchToUser();
    readWord(reg_pc+2); //dummy read
    prefetch(true);
}

void Core_68k::op_movefromsr(u16 opcode) {
    LoadEA(SizeWord, opcode & 0x3f);
    prefetch( isRegisterMode() );
	if (adm == DR_DIRECT) sync(2);
    writeEA(SizeWord, reg_s & 0xa71f, true);
}

void Core_68k::op_movetoccr(u16 opcode) {
	u32 data = LoadEA(SizeWord, opcode & 0x3f);
    sync(4);
	reg_s.l = data & 0x1f;
    readWord(reg_pc+2); //dummy read
    prefetch(true);
}

void Core_68k::op_movetosr(u16 opcode) {
	if (!reg_s.s) {
        setPrivilegeException();
		return;
	}
	u32 data = LoadEA(SizeWord, opcode & 0x3f);
    sync(4);
    reg_s = data & 0xa71f;

    switchToUser();
    readWord(reg_pc+2); //dummy read
    prefetch(true);
}

template<u8 opmode> void Core_68k::op_exg(u16 opcode) {
	u8 Rx = (opcode >> 9) & 7;
	u8 Ry = opcode & 7;
	u32 temp;
    prefetch(true);

	switch(opmode) {
        default: break;
		case 8: //data registers
			temp = reg_d[Rx];
			reg_d[Rx] = reg_d[Ry];
			reg_d[Ry] = temp;
			break;
		case 9: //address registers
			temp = reg_a[Rx];
			reg_a[Rx] = reg_a[Ry];
			reg_a[Ry] = temp;
			break;
		case 17: //data and address register
			temp = reg_d[Rx];
			reg_d[Rx] = reg_a[Ry];
			reg_a[Ry] = temp;
			break;
	}
	sync(2);
}

template<bool _longSize> void Core_68k::op_ext(u16 opcode) {
    u8 Rx = opcode & 7;

    // u32 old = reg_d[Rx].d;

    if (_longSize) {
        reg_d[Rx].d &= 0x0000FFFF;
        reg_d[Rx].d = reg_d[Rx].w | (((reg_d[Rx].w & 0x8000) ? 0xFFFF << 16 : 0));
        setFlags(flag_logical, SizeLong, reg_d[Rx], 0, 0);
    } else {
        // printf("WORD SIZE\n");
        reg_d[Rx].w &= 0x00FF;
        reg_d[Rx].w = reg_d[Rx].l | (((reg_d[Rx].l & 0x80) ? 0xFF << 8 : 0));
        setFlags(flag_logical, SizeWord, reg_d[Rx], 0, 0);
    }

    // printf("reg_d[Rx].d = %x %x\n", old, reg_d[Rx].d);
    prefetch(true);
}

void Core_68k::op_link(u16 opcode) {
	u8 regPos = opcode & 7;
    i32 val = (i32)(i16)reg_irc;
    readExtensionWord();
    u32 _decSP = reg_a[7] - 4;
    u32 data = regPos == 7 ? _decSP : reg_a[regPos];
    writeLong(_decSP, data);

    reg_a[7] = _decSP;
	reg_a[regPos] = reg_a[7];
    reg_a[7] += val;
    prefetch(true);
}

template<bool dr> void Core_68k::op_moveusp(u16 opcode) {
	if (!reg_s.s) {
        setPrivilegeException();
		return;
	}
	u8 regPos = opcode & 7;
    if (dr) {
		reg_a[regPos] = reg_usp;
	} else {
		reg_usp = reg_a[regPos];
	}
    prefetch(true);
}

void Core_68k::op_nop(u16 opcode) {
    prefetch(true);
}

void Core_68k::op_reset(u16 opcode) {
	if (!reg_s.s) {
        setPrivilegeException();
		return;
	}
    sync(128);
    resetInstruction();
    prefetch(true);
}

void Core_68k::op_rte(u16 opcode) {
	if (!reg_s.s) {
        setPrivilegeException();
		return;
	}
	reg_s = readWord(reg_a[7]);
	reg_s &= 0xA71F;

	reg_a[7] += 2;
	reg_pc = readLong(reg_a[7]);
	reg_a[7] += 4;

    switchToUser();
    fullprefetch(true);
}

void Core_68k::op_rtr(u16 opcode) {
	reg_s.l = readWord(reg_a[7]) & 0x1f;
	reg_a[7] += 2;
	reg_pc = readLong(reg_a[7]);
	reg_a[7] += 4;
    fullprefetch(true);
}

void Core_68k::op_rts(u16 opcode) {
	reg_pc = readLong(reg_a[7]);
	reg_a[7] += 4;
    fullprefetch(true);
}

void Core_68k::op_stop(u16 opcode) {
    if (!reg_s.s) {
        setPrivilegeException();
        return;
    }
    reg_s = reg_irc;
    reg_s &= 0xA71F;

    sync(2);
    stop = true;
    incrementPc();
    sampleIrq();
    sync(2);
}

void Core_68k::op_swap(u16 opcode) {
	u8 regPos = opcode & 7;
    reg_d[regPos] = (reg_d[regPos] >> 16) | ( (reg_d[regPos] & 0xFFFF) << 16);
	setFlags(flag_logical, SizeLong, reg_d[regPos], 0, 0);
    prefetch(true);
}

void Core_68k::op_trap(u16 opcode) {
	sync(4);
    trapException( (opcode & 0xF) + 32 );
}

void Core_68k::op_trapv(u16 opcode) {
    prefetch(true);
    if (reg_s.v) {
        trapException(7);
    }
}

void Core_68k::op_unlk(u16 opcode) {
	u8 regPos = opcode & 7;
	reg_a[7] = reg_a[regPos];
	reg_a[regPos] = LoadEA(SizeLong, (2 << 3) | 7);
    if (regPos != 7) reg_a[7] += 4;
    prefetch(true);
}

template<u8 opmode> void Core_68k::op_movep(u16 opcode) {
	u32 data;
	u8 regPosA = opcode & 7;
	u8 regPosD = (opcode >> 9) & 7;
    LoadEA(SizeByte, 5 << 3 | regPosA, true);

	switch (opmode) {
		case 4: //word: mem -> reg
            data = readByte(eaAddr); eaAddr += 2;
			reg_d[regPosD].no_use_h = data & 0xFF;
            data = readByte(eaAddr);
			reg_d[regPosD].l = data & 0xFF;
			break;
		case 6: //word: reg -> mem
			writeByte(eaAddr, reg_d[regPosD].no_use_h); eaAddr += 2;
			writeByte(eaAddr, reg_d[regPosD].l);
			break;
		case 5: //long: mem -> reg
			data = readByte(eaAddr); eaAddr += 2;
			reg_d[regPosD].no_use_hb = data & 0xFF;
			data = readByte(eaAddr); eaAddr += 2;
			reg_d[regPosD].no_use_lb = data & 0xFF;
			data = readByte(eaAddr); eaAddr += 2;
			reg_d[regPosD].no_use_h = data & 0xFF;
			data = readByte(eaAddr);
			reg_d[regPosD].l = data & 0xFF;
			break;
		case 7: //long: reg -> mem
			writeByte(eaAddr, reg_d[regPosD].no_use_hb); eaAddr += 2;
			writeByte(eaAddr, reg_d[regPosD].no_use_lb); eaAddr += 2;
			writeByte(eaAddr, reg_d[regPosD].no_use_h); eaAddr += 2;
			writeByte(eaAddr, reg_d[regPosD].l);
			break;
	}
    prefetch(true);
}
