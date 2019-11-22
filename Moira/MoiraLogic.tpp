// -----------------------------------------------------------------------------
// This file is part of Moira - A Motorola 68k emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

template<Size size, Instr instr> uint32_t
Moira::shift(uint32_t cnt, uint32_t data) {

    assert(cnt > 0);

    bool c = 0;
    bool v = 0;

    switch(instr) {

        case ASL:

            for (int i = 0; i < cnt; i++) {
                c = MSBIT<size>(data);
                v |= data ^ (data << 1);
                data <<= 1;
            }
            flags.c = c;
            flags.v = NEG<size>(v);
            flags.z = ZERO<size>(data);
            flags.n = NEG<size>(data);
            if (cnt) flags.x = c; // IF IS OBSOLETE???

            return CLIP<size>(data);

        case ASR:    return 2; // asr<Size>( data, shift );
        case LSL:    return 3; // lsl<Size>( data, shift );
        case LSR:    return 4; // lsr<Size>( data, shift );
        case ROL:    return 5; // rol<Size>( data, shift );
        case ROR:    return 6; // ror<Size>( data, shift );
        case ROXL:   return 7; // roxl<Size>( data, shift );
        case ROXR:   return 8; // roxr<Size>( data, shift );

        default: assert(false);
    }

    return 0;
}
