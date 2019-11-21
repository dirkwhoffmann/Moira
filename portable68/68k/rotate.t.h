template<bool extend, u8 size> void Core_68k::rotate_left(u8 shiftCount, u32& data) {
    bool carry = false;
    if (extend)
        shiftCount = shiftCount % (bits_(size) + 1);

    if (shiftCount > 0) {
        if (!extend && ( shiftCount > bits_(size) ) )
            shiftCount &= (bits_(size) - 1);

        u32 rotate = data >> (bits_(size) - shiftCount);
        carry = rotate & 1;
        if (extend) {
            data = (((data << 1) | (u8)reg_s.x) << (shiftCount-1)) | (rotate >> 1);
        } else {
            data <<= shiftCount;
            data |= rotate;
        }
    }

    setFlags(flag_logical, size, data);

    if (extend) {
        if (shiftCount > 0) {
            reg_s.x = carry;
        }
        reg_s.c = reg_s.x;
    } else {
        reg_s.c = carry;
    }
}

template<bool extend, u8 size> void Core_68k::rotate_right(u8 shiftCount, u32& data) {
    bool carry = false;
    if (extend)
        shiftCount = shiftCount % (bits_(size) + 1);

    if (shiftCount > 0) {
        if (!extend && ( shiftCount > bits_(size) ) )
            shiftCount &= (bits_(size) - 1);

        u32 rotate = data << (bits_(size) - shiftCount);
        carry = (rotate & msb_(size)) != 0;

        if (extend) {
            data = (((data >> 1) | reg_s.x << (bits_(size) - 1) ) >> (shiftCount-1)) | (rotate << 1);
        } else {
            data >>= shiftCount;
            data |= rotate;
        }
    }

    setFlags(flag_logical, size, data);

    if (extend) {
        if (shiftCount > 0) {
            reg_s.x = carry;
        }
        reg_s.c = reg_s.x;
    } else {
        reg_s.c = carry;
    }
}
