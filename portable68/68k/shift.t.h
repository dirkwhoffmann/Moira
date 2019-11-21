
template<bool arithmetic, u8 size> void Core_68k::shift_left(u8 shiftCount, u32& data) {
    bool carry = false, overflow = false;

    if (shiftCount >= bits_(size)) {
        overflow = data != 0;
        carry = shiftCount == bits_(size) ? data & 1 : 0;
        data = 0;
    } else if (shiftCount > 0 ) {
        u32 mask = (mask_(size) << ((bits_(size)-1) - shiftCount)) & mask_(size);
        overflow = (data & mask) != mask && (data & mask) != 0;
        data <<= shiftCount - 1;
        carry = (data & msb_(size)) != 0;
        data <<= 1;
    }

    setFlags(flag_logical, size, data);
    reg_s.c = carry;

    if (arithmetic) reg_s.v = overflow;

    if (shiftCount != 0)
        reg_s.x = reg_s.c;
}

template<bool arithmetic, u8 size> void Core_68k::shift_right(u8 shiftCount, u32& data) {
    bool carry = false;
    bool sign = (data & msb_(size)) != 0;

    if (shiftCount >=  bits_(size)) {
        data = arithmetic ? (sign ? mask_(size) : 0) : 0;
        if (arithmetic) carry = sign;
        else carry = shiftCount == bits_(size) ? sign : false;
    } else if (shiftCount > 0 ) {
        data >>= shiftCount-1;
        carry = data & 1;
        data >>= 1;
        if (arithmetic) data |= (mask_(size) << (bits_(size) - shiftCount)) & (sign ? mask_(size) : 0);
    }

    setFlags(flag_logical, size, data);
    reg_s.c = carry;

    if (shiftCount != 0)
        reg_s.x = reg_s.c;
}
