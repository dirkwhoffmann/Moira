"use strict";
// ported from binutils-gdb/include/opcode/m68k.h, Copyright (C) 1989-2021 Free Software Foundation, Inc. GPLv3
// removed all references to coldfire
exports.__esModule = true;
exports.get_all_insn_m68k = exports.print_insn_m68k = void 0;
// see https://github.com/BartmanAbyss/binutils-gdb/blob/master/opcodes/m68k-dis.c
// see https://github.com/BartmanAbyss/binutils-gdb/blob/master/opcodes/m68k-opc.c
// see https://github.com/BartmanAbyss/binutils-gdb/blob/master/include/opcode/m68k.h
// see http://www.xgc-tek.com/manuals/m68k-ada-gs/x1425.html for pseudo opcodes (jbsr, jra, jcc, dbcc, fjcc)
// 68000 programmer's manual: https://www.nxp.com/docs/en/reference-manual/M68000PRM.pdf
// 68000 user's manual: https://www.nxp.com/docs/en/reference-manual/MC68000UM.pdf
// 68020 user's manual: https://www.nxp.com/docs/en/data-sheet/MC68020UM.pdf
// 68030 user's manual: https://www.nxp.com/docs/en/reference-manual/MC68030UM-P1.pdf
// 68040 user's manual: https://www.nxp.com/docs/en/reference-manual/MC68040UM.pdf
// 68060 user's manual: https://www.nxp.com/docs/en/data-sheet/MC68060UM.pdf
// 68881/68882 user's manual: http://bitsavers.trailing-edge.com/components/motorola/68000/MC68881_MC68882_Floating-Point_Coprocessor_Users_Manual_1ed_1987.pdf
/* eslint-disable @typescript-eslint/naming-convention,no-underscore-dangle,id-denylist,id-match, @typescript-eslint/naming-convention */
var dis;
(function (dis) {
    dis[dis["noninsn"] = 0] = "noninsn";
    dis[dis["nonbranch"] = 1] = "nonbranch";
    dis[dis["branch"] = 2] = "branch";
    dis[dis["condbranch"] = 3] = "condbranch";
    dis[dis["jsr"] = 4] = "jsr";
    dis[dis["condjsr"] = 5] = "condjsr";
    dis[dis["dref"] = 6] = "dref";
    dis[dis["dref2"] = 7] = "dref2"; /* Two data references in instruction.  */
})(dis || (dis = {}));
var _m68k_undef = 0;
var m68000 = 0x001;
var m68010 = 0x002;
var m68020 = 0x004;
var m68030 = 0x008;
var m68040 = 0x010;
var m68060 = 0x020;
var m68881 = 0x040;
var m68851 = 0x080;
var m68k_mask = 0x3ff;
/* Handy aliases.  */
var m68040up = (m68040 | m68060);
var m68030up = (m68030 | m68040up);
var m68020up = (m68020 | m68030up);
var m68010up = (m68010 | m68020up);
var m68000up = (m68000 | m68010up);
var mfloat = (m68881 | m68040 | m68060);
var mmmu = (m68851 | m68030 | m68040 | m68060);
var one = function (x) { return x << 16 >>> 0; }; // >>> 0: make unsigned
var two = function (x, y) { return ((x << 16) + y) >>> 0; };
var SCOPE_LINE = (0x1 << 3);
var SCOPE_PAGE = (0x2 << 3);
var SCOPE_ALL = (0x3 << 3);
// ported from binutils-gdb/opcodes/m68k-opc.c, Copyright (C) 1989-2021 Free Software Foundation, Inc. GPLv3
// removed all coldfire opcodes, FPU opcodes missing
var m68k_opcodes = [
    { name: "abcd", size: 2, opcode: one(49408), match: one(61944), args: "DsDd", arch: m68000up, type: dis.nonbranch },
    { name: "abcd", size: 2, opcode: one(49416), match: one(61944), args: "-s-d", arch: m68000up, type: dis.nonbranch },
    { name: "adda.w", size: 2, opcode: one(53440), match: one(61888), args: "*wAd", arch: m68000up, type: dis.nonbranch },
    { name: "adda.l", size: 2, opcode: one(53696), match: one(61888), args: "*lAd", arch: m68000up, type: dis.nonbranch },
    { name: "addi.b", size: 4, opcode: one(1536), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "addi.w", size: 4, opcode: one(1600), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "addi.l", size: 6, opcode: one(1664), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "addq.b", size: 2, opcode: one(20480), match: one(61888), args: "Qd$b", arch: m68000up, type: dis.nonbranch },
    { name: "addq.w", size: 2, opcode: one(20544), match: one(61888), args: "Qd%w", arch: m68000up, type: dis.nonbranch },
    { name: "addq.l", size: 2, opcode: one(20608), match: one(61888), args: "Qd%l", arch: m68000up, type: dis.nonbranch },
    /* The add opcode can generate the adda, addi, and addq instructions.  */
    { name: "add.b", size: 2, opcode: one(20480), match: one(61888), args: "Qd$b", arch: m68000up, type: dis.nonbranch },
    { name: "add.b", size: 4, opcode: one(1536), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "add.b", size: 2, opcode: one(53248), match: one(61888), args: ";bDd", arch: m68000up, type: dis.nonbranch },
    { name: "add.b", size: 2, opcode: one(53504), match: one(61888), args: "Dd~b", arch: m68000up, type: dis.nonbranch },
    { name: "add.w", size: 2, opcode: one(20544), match: one(61888), args: "Qd%w", arch: m68000up, type: dis.nonbranch },
    { name: "add.w", size: 2, opcode: one(53440), match: one(61888), args: "*wAd", arch: m68000up, type: dis.nonbranch },
    { name: "add.w", size: 4, opcode: one(1600), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "add.w", size: 2, opcode: one(53312), match: one(61888), args: "*wDd", arch: m68000up, type: dis.nonbranch },
    { name: "add.w", size: 2, opcode: one(53568), match: one(61888), args: "Dd~w", arch: m68000up, type: dis.nonbranch },
    { name: "add.l", size: 2, opcode: one(20608), match: one(61888), args: "Qd%l", arch: m68000up, type: dis.nonbranch },
    { name: "add.l", size: 6, opcode: one(1664), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "add.l", size: 2, opcode: one(53696), match: one(61888), args: "*lAd", arch: m68000up, type: dis.nonbranch },
    { name: "add.l", size: 2, opcode: one(53376), match: one(61888), args: "*lDd", arch: m68000up, type: dis.nonbranch },
    { name: "add.l", size: 2, opcode: one(53632), match: one(61888), args: "Dd~l", arch: m68000up, type: dis.nonbranch },
    { name: "addx.b", size: 2, opcode: one(53504), match: one(61944), args: "DsDd", arch: m68000up, type: dis.nonbranch },
    { name: "addx.b", size: 2, opcode: one(53512), match: one(61944), args: "-s-d", arch: m68000up, type: dis.nonbranch },
    { name: "addx.w", size: 2, opcode: one(53568), match: one(61944), args: "DsDd", arch: m68000up, type: dis.nonbranch },
    { name: "addx.w", size: 2, opcode: one(53576), match: one(61944), args: "-s-d", arch: m68000up, type: dis.nonbranch },
    { name: "addx.l", size: 2, opcode: one(53632), match: one(61944), args: "DsDd", arch: m68000up, type: dis.nonbranch },
    { name: "addx.l", size: 2, opcode: one(53640), match: one(61944), args: "-s-d", arch: m68000up, type: dis.nonbranch },
    { name: "andi.b", size: 4, opcode: one(512), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "andi.b", size: 4, opcode: one(572), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "andi.w", size: 4, opcode: one(576), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "andi.w", size: 4, opcode: one(636), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "andi.l", size: 6, opcode: one(640), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "andi", size: 4, opcode: one(576), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "andi", size: 4, opcode: one(572), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "andi", size: 4, opcode: one(636), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    /* The and opcode can generate the andi instruction.  */
    { name: "and.b", size: 4, opcode: one(512), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "and.b", size: 4, opcode: one(572), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "and.b", size: 2, opcode: one(49152), match: one(61888), args: ";bDd", arch: m68000up, type: dis.nonbranch },
    { name: "and.b", size: 2, opcode: one(49408), match: one(61888), args: "Dd~b", arch: m68000up, type: dis.nonbranch },
    { name: "and.w", size: 4, opcode: one(576), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "and.w", size: 4, opcode: one(636), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "and.w", size: 2, opcode: one(49216), match: one(61888), args: ";wDd", arch: m68000up, type: dis.nonbranch },
    { name: "and.w", size: 2, opcode: one(49472), match: one(61888), args: "Dd~w", arch: m68000up, type: dis.nonbranch },
    { name: "and.l", size: 6, opcode: one(640), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "and.l", size: 2, opcode: one(49280), match: one(61888), args: ";lDd", arch: m68000up, type: dis.nonbranch },
    { name: "and.l", size: 2, opcode: one(49536), match: one(61888), args: "Dd~l", arch: m68000up, type: dis.nonbranch },
    { name: "and", size: 4, opcode: one(576), match: one(65472), args: "#w$w", arch: m68000up, type: dis.nonbranch },
    { name: "and", size: 4, opcode: one(572), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "and", size: 4, opcode: one(636), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "and", size: 2, opcode: one(49216), match: one(61888), args: ";wDd", arch: m68000up, type: dis.nonbranch },
    { name: "and", size: 2, opcode: one(49472), match: one(61888), args: "Dd~w", arch: m68000up, type: dis.nonbranch },
    { name: "asl.b", size: 2, opcode: one(57600), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asl.b", size: 2, opcode: one(57632), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asl.w", size: 2, opcode: one(57664), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asl.w", size: 2, opcode: one(57696), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asl.w", size: 2, opcode: one(57792), match: one(65472), args: "~s", arch: m68000up, type: dis.nonbranch },
    { name: "asl.l", size: 2, opcode: one(57728), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asl.l", size: 2, opcode: one(57760), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asr.b", size: 2, opcode: one(57344), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asr.b", size: 2, opcode: one(57376), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asr.w", size: 2, opcode: one(57408), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asr.w", size: 2, opcode: one(57440), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asr.w", size: 2, opcode: one(57536), match: one(65472), args: "~s", arch: m68000up, type: dis.nonbranch },
    { name: "asr.l", size: 2, opcode: one(57472), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "asr.l", size: 2, opcode: one(57504), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "bhi.w", size: 2, opcode: one(25088), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bls.w", size: 2, opcode: one(25344), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bcc.w", size: 2, opcode: one(25600), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bcs.w", size: 2, opcode: one(25856), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bne.w", size: 2, opcode: one(26112), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "beq.w", size: 2, opcode: one(26368), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bvc.w", size: 2, opcode: one(26624), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bvs.w", size: 2, opcode: one(26880), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bpl.w", size: 2, opcode: one(27136), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bmi.w", size: 2, opcode: one(27392), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bge.w", size: 2, opcode: one(27648), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "blt.w", size: 2, opcode: one(27904), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bgt.w", size: 2, opcode: one(28160), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "ble.w", size: 2, opcode: one(28416), match: one(65535), args: "BW", arch: m68000up, type: dis.condbranch },
    { name: "bhi.l", size: 2, opcode: one(25343), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bls.l", size: 2, opcode: one(25599), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bcc.l", size: 2, opcode: one(25855), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bcs.l", size: 2, opcode: one(26111), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bne.l", size: 2, opcode: one(26367), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "beq.l", size: 2, opcode: one(26623), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bvc.l", size: 2, opcode: one(26879), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bvs.l", size: 2, opcode: one(27135), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bpl.l", size: 2, opcode: one(27391), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bmi.l", size: 2, opcode: one(27647), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bge.l", size: 2, opcode: one(27903), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "blt.l", size: 2, opcode: one(28159), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bgt.l", size: 2, opcode: one(28415), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "ble.l", size: 2, opcode: one(28671), match: one(65535), args: "BL", arch: m68020up, type: dis.condbranch },
    { name: "bhi.s", size: 2, opcode: one(25088), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bls.s", size: 2, opcode: one(25344), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bcc.s", size: 2, opcode: one(25600), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bcs.s", size: 2, opcode: one(25856), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bne.s", size: 2, opcode: one(26112), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "beq.s", size: 2, opcode: one(26368), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bvc.s", size: 2, opcode: one(26624), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bvs.s", size: 2, opcode: one(26880), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bpl.s", size: 2, opcode: one(27136), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bmi.s", size: 2, opcode: one(27392), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bge.s", size: 2, opcode: one(27648), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "blt.s", size: 2, opcode: one(27904), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "bgt.s", size: 2, opcode: one(28160), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "ble.s", size: 2, opcode: one(28416), match: one(65280), args: "BB", arch: m68000up, type: dis.condbranch },
    { name: "jhi", size: 2, opcode: one(25088), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jls", size: 2, opcode: one(25344), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jcc", size: 2, opcode: one(25600), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jcs", size: 2, opcode: one(25856), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jne", size: 2, opcode: one(26112), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jeq", size: 2, opcode: one(26368), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jvc", size: 2, opcode: one(26624), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jvs", size: 2, opcode: one(26880), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jpl", size: 2, opcode: one(27136), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jmi", size: 2, opcode: one(27392), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jge", size: 2, opcode: one(27648), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jlt", size: 2, opcode: one(27904), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jgt", size: 2, opcode: one(28160), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "jle", size: 2, opcode: one(28416), match: one(65280), args: "Bg", arch: m68000up, type: dis.condbranch },
    { name: "bchg", size: 2, opcode: one(320), match: one(61888), args: "Dd$s", arch: m68000up, type: dis.nonbranch },
    { name: "bchg", size: 4, opcode: one(2112), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "bclr", size: 2, opcode: one(384), match: one(61888), args: "Dd$s", arch: m68000up, type: dis.nonbranch },
    { name: "bclr", size: 4, opcode: one(2176), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "bfchg", size: 4, opcode: two(60096, 0), match: two(65472, 61440), args: "?sO2O3", arch: m68020up, type: dis.nonbranch },
    { name: "bfclr", size: 4, opcode: two(60608, 0), match: two(65472, 61440), args: "?sO2O3", arch: m68020up, type: dis.nonbranch },
    { name: "bfexts", size: 4, opcode: two(60352, 0), match: two(65472, 32768), args: "/sO2O3D1", arch: m68020up, type: dis.nonbranch },
    { name: "bfextu", size: 4, opcode: two(59840, 0), match: two(65472, 32768), args: "/sO2O3D1", arch: m68020up, type: dis.nonbranch },
    { name: "bfffo", size: 4, opcode: two(60864, 0), match: two(65472, 32768), args: "/sO2O3D1", arch: m68020up, type: dis.nonbranch },
    { name: "bfins", size: 4, opcode: two(61376, 0), match: two(65472, 32768), args: "D1?sO2O3", arch: m68020up, type: dis.nonbranch },
    { name: "bfset", size: 4, opcode: two(61120, 0), match: two(65472, 61440), args: "?sO2O3", arch: m68020up, type: dis.nonbranch },
    { name: "bftst", size: 4, opcode: two(59584, 0), match: two(65472, 61440), args: "/sO2O3", arch: m68020up, type: dis.nonbranch },
    { name: "bkpt", size: 2, opcode: one(18504), match: one(65528), args: "ts", arch: m68010up, type: dis.nonbranch },
    { name: "bra.w", size: 2, opcode: one(24576), match: one(65535), args: "BW", arch: m68000up, type: dis.branch },
    { name: "bra.l", size: 2, opcode: one(24831), match: one(65535), args: "BL", arch: m68020up, type: dis.branch },
    { name: "bra.s", size: 2, opcode: one(24576), match: one(65280), args: "BB", arch: m68000up, type: dis.branch },
    { name: "bset", size: 2, opcode: one(448), match: one(61888), args: "Dd$s", arch: m68000up, type: dis.nonbranch },
    { name: "bset", size: 4, opcode: one(2240), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "bsr.w", size: 2, opcode: one(24832), match: one(65535), args: "BW", arch: m68000up, type: dis.jsr },
    { name: "bsr.l", size: 2, opcode: one(25087), match: one(65535), args: "BL", arch: m68020up, type: dis.jsr },
    { name: "bsr.s", size: 2, opcode: one(24832), match: one(65280), args: "BB", arch: m68000up, type: dis.jsr },
    { name: "btst", size: 2, opcode: one(256), match: one(61888), args: "Dd;b", arch: m68000up, type: dis.nonbranch },
    { name: "btst", size: 4, opcode: one(2048), match: one(65472), args: "#b@s", arch: m68000up, type: dis.nonbranch },
    { name: "callm", size: 4, opcode: one(1728), match: one(65472), args: "#b!s", arch: m68020, type: dis.nonbranch },
    { name: "cas2.w", size: 6, opcode: two(3324, 0), match: two(65535, 3640), args: "D3D6D2D5r1r4", arch: m68020up, type: dis.nonbranch },
    { name: "cas2.w", size: 6, opcode: two(3324, 0), match: two(65535, 3640), args: "D3D6D2D5R1R4", arch: m68020up, type: dis.nonbranch },
    { name: "cas2.l", size: 6, opcode: two(3836, 0), match: two(65535, 3640), args: "D3D6D2D5r1r4", arch: m68020up, type: dis.nonbranch },
    { name: "cas2.l", size: 6, opcode: two(3836, 0), match: two(65535, 3640), args: "D3D6D2D5R1R4", arch: m68020up, type: dis.nonbranch },
    { name: "cas.b", size: 4, opcode: two(2752, 0), match: two(65472, 65080), args: "D3D2~s", arch: m68020up, type: dis.nonbranch },
    { name: "cas.w", size: 4, opcode: two(3264, 0), match: two(65472, 65080), args: "D3D2~s", arch: m68020up, type: dis.nonbranch },
    { name: "cas.l", size: 4, opcode: two(3776, 0), match: two(65472, 65080), args: "D3D2~s", arch: m68020up, type: dis.nonbranch },
    { name: "chk2.b", size: 4, opcode: two(192, 2048), match: two(65472, 4095), args: "!sR1", arch: m68020up, type: dis.nonbranch },
    { name: "chk2.w", size: 4, opcode: two(704, 2048), match: two(65472, 4095), args: "!sR1", arch: m68020up, type: dis.nonbranch },
    { name: "chk2.l", size: 4, opcode: two(1216, 2048), match: two(65472, 4095), args: "!sR1", arch: m68020up, type: dis.nonbranch },
    { name: "chk.l", size: 2, opcode: one(16640), match: one(61888), args: ";lDd", arch: m68020up, type: dis.nonbranch },
    { name: "chk.w", size: 2, opcode: one(16768), match: one(61888), args: ";wDd", arch: m68020up, type: dis.nonbranch },
    { name: "cinva", size: 2, opcode: one(0xf400 | SCOPE_ALL), match: one(0xff38), args: "ce", arch: m68040up, type: dis.nonbranch },
    { name: "cinvl", size: 2, opcode: one(0xf400 | SCOPE_LINE), match: one(0xff38), args: "ceas", arch: m68040up, type: dis.nonbranch },
    { name: "cinvp", size: 2, opcode: one(0xf400 | SCOPE_PAGE), match: one(0xff38), args: "ceas", arch: m68040up, type: dis.nonbranch },
    { name: "cpusha", size: 2, opcode: one(0xf420 | SCOPE_ALL), match: one(0xff38), args: "ce", arch: m68040up, type: dis.nonbranch },
    { name: "cpushl", size: 2, opcode: one(0xf420 | SCOPE_LINE), match: one(0xff38), args: "ceas", arch: m68040up, type: dis.nonbranch },
    { name: "cpushp", size: 2, opcode: one(0xf420 | SCOPE_PAGE), match: one(0xff38), args: "ceas", arch: m68040up, type: dis.nonbranch },
    { name: "clr.b", size: 2, opcode: one(16896), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "clr.w", size: 2, opcode: one(16960), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "clr.l", size: 2, opcode: one(17024), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "cmp2.b", size: 4, opcode: two(192, 0), match: two(65472, 4095), args: "!sR1", arch: m68020up, type: dis.nonbranch },
    { name: "cmp2.w", size: 4, opcode: two(704, 0), match: two(65472, 4095), args: "!sR1", arch: m68020up, type: dis.nonbranch },
    { name: "cmp2.l", size: 4, opcode: two(1216, 0), match: two(65472, 4095), args: "!sR1", arch: m68020up, type: dis.nonbranch },
    { name: "cmpa.w", size: 2, opcode: one(45248), match: one(61888), args: "*wAd", arch: m68000up, type: dis.nonbranch },
    { name: "cmpa.l", size: 2, opcode: one(45504), match: one(61888), args: "*lAd", arch: m68000up, type: dis.nonbranch },
    { name: "cmpi.b", size: 4, opcode: one(3072), match: one(65472), args: "#b$s", arch: m68000 | m68010, type: dis.nonbranch },
    { name: "cmpi.b", size: 4, opcode: one(3072), match: one(65472), args: "#b@s", arch: m68020up, type: dis.nonbranch },
    { name: "cmpi.w", size: 4, opcode: one(3136), match: one(65472), args: "#w$s", arch: m68000 | m68010, type: dis.nonbranch },
    { name: "cmpi.w", size: 4, opcode: one(3136), match: one(65472), args: "#w@s", arch: m68020up, type: dis.nonbranch },
    { name: "cmpi.l", size: 6, opcode: one(3200), match: one(65472), args: "#l$s", arch: m68000 | m68010, type: dis.nonbranch },
    { name: "cmpi.l", size: 6, opcode: one(3200), match: one(65472), args: "#l@s", arch: m68020up, type: dis.nonbranch },
    { name: "cmpm.b", size: 2, opcode: one(45320), match: one(61944), args: "+s+d", arch: m68000up, type: dis.nonbranch },
    { name: "cmpm.w", size: 2, opcode: one(45384), match: one(61944), args: "+s+d", arch: m68000up, type: dis.nonbranch },
    { name: "cmpm.l", size: 2, opcode: one(45448), match: one(61944), args: "+s+d", arch: m68000up, type: dis.nonbranch },
    /* The cmp opcode can generate the cmpa, cmpm, and cmpi instructions.  */
    { name: "cmp.b", size: 4, opcode: one(3072), match: one(65472), args: "#b$s", arch: m68000 | m68010, type: dis.nonbranch },
    { name: "cmp.b", size: 4, opcode: one(3072), match: one(65472), args: "#b@s", arch: m68020up, type: dis.nonbranch },
    { name: "cmp.b", size: 2, opcode: one(45320), match: one(61944), args: "+s+d", arch: m68000up, type: dis.nonbranch },
    { name: "cmp.b", size: 2, opcode: one(45056), match: one(61888), args: ";bDd", arch: m68000up, type: dis.nonbranch },
    { name: "cmp.w", size: 2, opcode: one(45248), match: one(61888), args: "*wAd", arch: m68000up, type: dis.nonbranch },
    { name: "cmp.w", size: 4, opcode: one(3136), match: one(65472), args: "#w$s", arch: m68000 | m68010, type: dis.nonbranch },
    { name: "cmp.w", size: 4, opcode: one(3136), match: one(65472), args: "#w@s", arch: m68020up, type: dis.nonbranch },
    { name: "cmp.w", size: 2, opcode: one(45384), match: one(61944), args: "+s+d", arch: m68000up, type: dis.nonbranch },
    { name: "cmp.w", size: 2, opcode: one(45120), match: one(61888), args: "*wDd", arch: m68000up, type: dis.nonbranch },
    { name: "cmp.l", size: 2, opcode: one(45504), match: one(61888), args: "*lAd", arch: m68000up, type: dis.nonbranch },
    { name: "cmp.l", size: 6, opcode: one(3200), match: one(65472), args: "#l$s", arch: m68000 | m68010, type: dis.nonbranch },
    { name: "cmp.l", size: 6, opcode: one(3200), match: one(65472), args: "#l@s", arch: m68020up, type: dis.nonbranch },
    { name: "cmp.l", size: 2, opcode: one(45448), match: one(61944), args: "+s+d", arch: m68000up, type: dis.nonbranch },
    { name: "cmp.l", size: 2, opcode: one(45184), match: one(61888), args: "*lDd", arch: m68000up, type: dis.nonbranch },
    { name: "dbcc", size: 2, opcode: one(21704), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbcs", size: 2, opcode: one(21960), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbeq", size: 2, opcode: one(22472), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbf", size: 2, opcode: one(20936), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbge", size: 2, opcode: one(23752), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbgt", size: 2, opcode: one(24264), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbhi", size: 2, opcode: one(21192), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dble", size: 2, opcode: one(24520), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbls", size: 2, opcode: one(21448), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dblt", size: 2, opcode: one(24008), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbmi", size: 2, opcode: one(23496), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbne", size: 2, opcode: one(22216), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbpl", size: 2, opcode: one(23240), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbt", size: 2, opcode: one(20680), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbvc", size: 2, opcode: one(22728), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "dbvs", size: 2, opcode: one(22984), match: one(65528), args: "DsBw", arch: m68000up, type: dis.condbranch },
    { name: "divs.w", size: 2, opcode: one(33216), match: one(61888), args: ";wDd", arch: m68000up, type: dis.nonbranch },
    { name: "divs.l", size: 4, opcode: two(19520, 3072), match: two(65472, 36856), args: ";lD3D1", arch: m68020up, type: dis.nonbranch },
    { name: "divs.l", size: 4, opcode: two(19520, 2048), match: two(65472, 36856), args: ";lDD", arch: m68020up, type: dis.nonbranch },
    { name: "divsl.l", size: 4, opcode: two(19520, 2048), match: two(65472, 36856), args: ";lD3D1", arch: m68020up, type: dis.nonbranch },
    { name: "divsl.l", size: 4, opcode: two(19520, 2048), match: two(65472, 36856), args: ";lDD", arch: m68020up, type: dis.nonbranch },
    { name: "divu.w", size: 2, opcode: one(32960), match: one(61888), args: ";wDd", arch: m68000up, type: dis.nonbranch },
    { name: "divu.l", size: 4, opcode: two(19520, 1024), match: two(65472, 36856), args: ";lD3D1", arch: m68020up, type: dis.nonbranch },
    { name: "divu.l", size: 4, opcode: two(19520, 0), match: two(65472, 36856), args: ";lDD", arch: m68020up, type: dis.nonbranch },
    { name: "divul.l", size: 4, opcode: two(19520, 0), match: two(65472, 36856), args: ";lD3D1", arch: m68020up, type: dis.nonbranch },
    { name: "divul.l", size: 4, opcode: two(19520, 0), match: two(65472, 36856), args: ";lDD", arch: m68020up, type: dis.nonbranch },
    { name: "eori.b", size: 4, opcode: one(2560), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "eori.b", size: 4, opcode: one(2620), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "eori.w", size: 4, opcode: one(2624), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "eori.w", size: 4, opcode: one(2684), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "eori.l", size: 6, opcode: one(2688), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "eori", size: 4, opcode: one(2620), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "eori", size: 4, opcode: one(2684), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "eori", size: 4, opcode: one(2624), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    // The eor opcode can generate the eori instruction.
    { name: "eor.b", size: 4, opcode: one(2560), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "eor.b", size: 4, opcode: one(2620), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "eor.b", size: 2, opcode: one(45312), match: one(61888), args: "Dd$s", arch: m68000up, type: dis.nonbranch },
    { name: "eor.w", size: 4, opcode: one(2624), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "eor.w", size: 4, opcode: one(2684), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "eor.w", size: 2, opcode: one(45376), match: one(61888), args: "Dd$s", arch: m68000up, type: dis.nonbranch },
    { name: "eor.l", size: 6, opcode: one(2688), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "eor.l", size: 2, opcode: one(45440), match: one(61888), args: "Dd$s", arch: m68000up, type: dis.nonbranch },
    { name: "eor", size: 4, opcode: one(2620), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "eor", size: 4, opcode: one(2684), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "eor", size: 4, opcode: one(2624), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "eor", size: 2, opcode: one(45376), match: one(61888), args: "Dd$s", arch: m68000up, type: dis.nonbranch },
    { name: "exg", size: 2, opcode: one(49472), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "exg", size: 2, opcode: one(49480), match: one(61944), args: "AdAs", arch: m68000up, type: dis.nonbranch },
    { name: "exg", size: 2, opcode: one(49544), match: one(61944), args: "DdAs", arch: m68000up, type: dis.nonbranch },
    { name: "exg", size: 2, opcode: one(49544), match: one(61944), args: "AsDd", arch: m68000up, type: dis.nonbranch },
    { name: "ext.w", size: 2, opcode: one(18560), match: one(65528), args: "Ds", arch: m68000up, type: dis.nonbranch },
    { name: "ext.l", size: 2, opcode: one(18624), match: one(65528), args: "Ds", arch: m68000up, type: dis.nonbranch },
    { name: "extb.l", size: 2, opcode: one(18880), match: one(65528), args: "Ds", arch: m68020up, type: dis.nonbranch },
    // FLOAT starts here
    { name: "fabs.b", size: 4, opcode: two(0xF000, 0x5818), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fabs.d", size: 4, opcode: two(0xF000, 0x5418), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fabs.l", size: 4, opcode: two(0xF000, 0x4018), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fabs.p", size: 4, opcode: two(0xF000, 0x4C18), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fabs.s", size: 4, opcode: two(0xF000, 0x4418), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fabs.w", size: 4, opcode: two(0xF000, 0x5018), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fabs.x", size: 4, opcode: two(0xF000, 0x0018), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fabs.x", size: 4, opcode: two(0xF000, 0x4818), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fabs.x", size: 4, opcode: two(0xF000, 0x0018), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fsabs.b", size: 4, opcode: two(0xF000, 0x5858), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsabs.d", size: 4, opcode: two(0xF000, 0x5458), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsabs.l", size: 4, opcode: two(0xF000, 0x4058), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsabs.p", size: 4, opcode: two(0xF000, 0x4C58), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsabs.s", size: 4, opcode: two(0xF000, 0x4458), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsabs.w", size: 4, opcode: two(0xF000, 0x5058), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsabs.x", size: 4, opcode: two(0xF000, 0x0058), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fsabs.x", size: 4, opcode: two(0xF000, 0x4858), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsabs.x", size: 4, opcode: two(0xF000, 0x0058), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: m68040up, type: dis.nonbranch },
    { name: "fdabs.b", size: 4, opcode: two(0xF000, 0x585c), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdabs.d", size: 4, opcode: two(0xF000, 0x545c), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdabs.l", size: 4, opcode: two(0xF000, 0x405c), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdabs.p", size: 4, opcode: two(0xF000, 0x4C5c), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdabs.s", size: 4, opcode: two(0xF000, 0x445c), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdabs.w", size: 4, opcode: two(0xF000, 0x505c), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdabs.x", size: 4, opcode: two(0xF000, 0x005c), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fdabs.x", size: 4, opcode: two(0xF000, 0x485c), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdabs.x", size: 4, opcode: two(0xF000, 0x005c), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: m68040up, type: dis.nonbranch },
    { name: "facos.b", size: 4, opcode: two(0xF000, 0x581C), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "facos.d", size: 4, opcode: two(0xF000, 0x541C), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "facos.l", size: 4, opcode: two(0xF000, 0x401C), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "facos.p", size: 4, opcode: two(0xF000, 0x4C1C), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "facos.s", size: 4, opcode: two(0xF000, 0x441C), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "facos.w", size: 4, opcode: two(0xF000, 0x501C), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "facos.x", size: 4, opcode: two(0xF000, 0x001C), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "facos.x", size: 4, opcode: two(0xF000, 0x481C), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "facos.x", size: 4, opcode: two(0xF000, 0x001C), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fadd.b", size: 4, opcode: two(0xF000, 0x5822), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fadd.d", size: 4, opcode: two(0xF000, 0x5422), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fadd.l", size: 4, opcode: two(0xF000, 0x4022), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fadd.p", size: 4, opcode: two(0xF000, 0x4C22), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fadd.s", size: 4, opcode: two(0xF000, 0x4422), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fadd.w", size: 4, opcode: two(0xF000, 0x5022), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fadd.x", size: 4, opcode: two(0xF000, 0x0022), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fadd.x", size: 4, opcode: two(0xF000, 0x4822), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsadd.b", size: 4, opcode: two(0xF000, 0x5862), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsadd.d", size: 4, opcode: two(0xF000, 0x5462), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsadd.l", size: 4, opcode: two(0xF000, 0x4062), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsadd.p", size: 4, opcode: two(0xF000, 0x4C62), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsadd.s", size: 4, opcode: two(0xF000, 0x4462), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsadd.w", size: 4, opcode: two(0xF000, 0x5062), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsadd.x", size: 4, opcode: two(0xF000, 0x0062), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fsadd.x", size: 4, opcode: two(0xF000, 0x4862), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdadd.b", size: 4, opcode: two(0xF000, 0x5866), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdadd.d", size: 4, opcode: two(0xF000, 0x5466), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdadd.l", size: 4, opcode: two(0xF000, 0x4066), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdadd.p", size: 4, opcode: two(0xF000, 0x4C66), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdadd.s", size: 4, opcode: two(0xF000, 0x4466), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdadd.w", size: 4, opcode: two(0xF000, 0x5066), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdadd.x", size: 4, opcode: two(0xF000, 0x0066), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fdadd.x", size: 4, opcode: two(0xF000, 0x4866), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fasin.b", size: 4, opcode: two(0xF000, 0x580C), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fasin.d", size: 4, opcode: two(0xF000, 0x540C), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fasin.l", size: 4, opcode: two(0xF000, 0x400C), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fasin.p", size: 4, opcode: two(0xF000, 0x4C0C), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fasin.s", size: 4, opcode: two(0xF000, 0x440C), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fasin.w", size: 4, opcode: two(0xF000, 0x500C), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fasin.x", size: 4, opcode: two(0xF000, 0x000C), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fasin.x", size: 4, opcode: two(0xF000, 0x480C), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fasin.x", size: 4, opcode: two(0xF000, 0x000C), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fatan.b", size: 4, opcode: two(0xF000, 0x580A), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatan.d", size: 4, opcode: two(0xF000, 0x540A), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatan.l", size: 4, opcode: two(0xF000, 0x400A), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatan.p", size: 4, opcode: two(0xF000, 0x4C0A), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatan.s", size: 4, opcode: two(0xF000, 0x440A), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatan.w", size: 4, opcode: two(0xF000, 0x500A), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatan.x", size: 4, opcode: two(0xF000, 0x000A), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fatan.x", size: 4, opcode: two(0xF000, 0x480A), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatan.x", size: 4, opcode: two(0xF000, 0x000A), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fatanh.b", size: 4, opcode: two(0xF000, 0x580D), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatanh.d", size: 4, opcode: two(0xF000, 0x540D), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatanh.l", size: 4, opcode: two(0xF000, 0x400D), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatanh.p", size: 4, opcode: two(0xF000, 0x4C0D), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatanh.s", size: 4, opcode: two(0xF000, 0x440D), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatanh.w", size: 4, opcode: two(0xF000, 0x500D), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatanh.x", size: 4, opcode: two(0xF000, 0x000D), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fatanh.x", size: 4, opcode: two(0xF000, 0x480D), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fatanh.x", size: 4, opcode: two(0xF000, 0x000D), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    // This is the same as `fbf opcode: .+2'.
    { name: "fnop", size: 4, opcode: two(0xF280, 0x0000), match: two(0xFFFF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "fbeq", size: 2, opcode: one(0xF081), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbf", size: 2, opcode: one(0xF080), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbge", size: 2, opcode: one(0xF093), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbgl", size: 2, opcode: one(0xF096), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbgle", size: 2, opcode: one(0xF097), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbgt", size: 2, opcode: one(0xF092), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fble", size: 2, opcode: one(0xF095), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fblt", size: 2, opcode: one(0xF094), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbne", size: 2, opcode: one(0xF08E), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbnge", size: 2, opcode: one(0xF09C), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbngl", size: 2, opcode: one(0xF099), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbngle", size: 2, opcode: one(0xF098), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbngt", size: 2, opcode: one(0xF09D), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbnle", size: 2, opcode: one(0xF09A), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbnlt", size: 2, opcode: one(0xF09B), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fboge", size: 2, opcode: one(0xF083), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbogl", size: 2, opcode: one(0xF086), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbogt", size: 2, opcode: one(0xF082), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbole", size: 2, opcode: one(0xF085), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbolt", size: 2, opcode: one(0xF084), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbor", size: 2, opcode: one(0xF087), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbseq", size: 2, opcode: one(0xF091), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbsf", size: 2, opcode: one(0xF090), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbsne", size: 2, opcode: one(0xF09E), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbst", size: 2, opcode: one(0xF09F), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbt", size: 2, opcode: one(0xF08F), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbueq", size: 2, opcode: one(0xF089), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbuge", size: 2, opcode: one(0xF08B), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbugt", size: 2, opcode: one(0xF08A), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbule", size: 2, opcode: one(0xF08D), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbult", size: 2, opcode: one(0xF08C), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbun", size: 2, opcode: one(0xF088), match: one(0xF1FF), args: "IdBW", arch: mfloat, type: dis.nonbranch },
    { name: "fbeq.l", size: 2, opcode: one(0xF0C1), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbf.l", size: 2, opcode: one(0xF0C0), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbge.l", size: 2, opcode: one(0xF0D3), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbgl.l", size: 2, opcode: one(0xF0D6), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbgle.l", size: 2, opcode: one(0xF0D7), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbgt.l", size: 2, opcode: one(0xF0D2), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fble.l", size: 2, opcode: one(0xF0D5), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fblt.l", size: 2, opcode: one(0xF0D4), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbne.l", size: 2, opcode: one(0xF0CE), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbnge.l", size: 2, opcode: one(0xF0DC), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbngl.l", size: 2, opcode: one(0xF0D9), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbngle.l", size: 2, opcode: one(0xF0D8), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbngt.l", size: 2, opcode: one(0xF0DD), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbnle.l", size: 2, opcode: one(0xF0DA), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbnlt.l", size: 2, opcode: one(0xF0DB), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fboge.l", size: 2, opcode: one(0xF0C3), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbogl.l", size: 2, opcode: one(0xF0C6), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbogt.l", size: 2, opcode: one(0xF0C2), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbole.l", size: 2, opcode: one(0xF0C5), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbolt.l", size: 2, opcode: one(0xF0C4), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbor.l", size: 2, opcode: one(0xF0C7), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbseq.l", size: 2, opcode: one(0xF0D1), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbsf.l", size: 2, opcode: one(0xF0D0), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbsne.l", size: 2, opcode: one(0xF0DE), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbst.l", size: 2, opcode: one(0xF0DF), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbt.l", size: 2, opcode: one(0xF0CF), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbueq.l", size: 2, opcode: one(0xF0C9), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbuge.l", size: 2, opcode: one(0xF0CB), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbugt.l", size: 2, opcode: one(0xF0CA), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbule.l", size: 2, opcode: one(0xF0CD), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbult.l", size: 2, opcode: one(0xF0CC), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fbun.l", size: 2, opcode: one(0xF0C8), match: one(0xF1FF), args: "IdBC", arch: mfloat, type: dis.nonbranch },
    { name: "fjeq", size: 2, opcode: one(0xF081), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjf", size: 2, opcode: one(0xF080), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjge", size: 2, opcode: one(0xF093), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjgl", size: 2, opcode: one(0xF096), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjgle", size: 2, opcode: one(0xF097), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjgt", size: 2, opcode: one(0xF092), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjle", size: 2, opcode: one(0xF095), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjlt", size: 2, opcode: one(0xF094), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjne", size: 2, opcode: one(0xF08E), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjnge", size: 2, opcode: one(0xF09C), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjngl", size: 2, opcode: one(0xF099), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjngle", size: 2, opcode: one(0xF098), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjngt", size: 2, opcode: one(0xF09D), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjnle", size: 2, opcode: one(0xF09A), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjnlt", size: 2, opcode: one(0xF09B), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjoge", size: 2, opcode: one(0xF083), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjogl", size: 2, opcode: one(0xF086), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjogt", size: 2, opcode: one(0xF082), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjole", size: 2, opcode: one(0xF085), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjolt", size: 2, opcode: one(0xF084), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjor", size: 2, opcode: one(0xF087), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjseq", size: 2, opcode: one(0xF091), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjsf", size: 2, opcode: one(0xF090), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjsne", size: 2, opcode: one(0xF09E), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjst", size: 2, opcode: one(0xF09F), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjt", size: 2, opcode: one(0xF08F), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjueq", size: 2, opcode: one(0xF089), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjuge", size: 2, opcode: one(0xF08B), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjugt", size: 2, opcode: one(0xF08A), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjule", size: 2, opcode: one(0xF08D), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjult", size: 2, opcode: one(0xF08C), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fjun", size: 2, opcode: one(0xF088), match: one(0xF1BF), args: "IdBc", arch: mfloat, type: dis.condbranch },
    { name: "fcmp.b", size: 4, opcode: two(0xF000, 0x5838), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcmp.d", size: 4, opcode: two(0xF000, 0x5438), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcmp.l", size: 4, opcode: two(0xF000, 0x4038), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcmp.p", size: 4, opcode: two(0xF000, 0x4C38), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcmp.s", size: 4, opcode: two(0xF000, 0x4438), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcmp.w", size: 4, opcode: two(0xF000, 0x5038), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcmp.x", size: 4, opcode: two(0xF000, 0x0038), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fcmp.x", size: 4, opcode: two(0xF000, 0x4838), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcos.b", size: 4, opcode: two(0xF000, 0x581D), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcos.d", size: 4, opcode: two(0xF000, 0x541D), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcos.l", size: 4, opcode: two(0xF000, 0x401D), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcos.p", size: 4, opcode: two(0xF000, 0x4C1D), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcos.s", size: 4, opcode: two(0xF000, 0x441D), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcos.w", size: 4, opcode: two(0xF000, 0x501D), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcos.x", size: 4, opcode: two(0xF000, 0x001D), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fcos.x", size: 4, opcode: two(0xF000, 0x481D), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcos.x", size: 4, opcode: two(0xF000, 0x001D), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fcosh.b", size: 4, opcode: two(0xF000, 0x5819), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcosh.d", size: 4, opcode: two(0xF000, 0x5419), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcosh.l", size: 4, opcode: two(0xF000, 0x4019), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcosh.p", size: 4, opcode: two(0xF000, 0x4C19), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcosh.s", size: 4, opcode: two(0xF000, 0x4419), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcosh.w", size: 4, opcode: two(0xF000, 0x5019), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcosh.x", size: 4, opcode: two(0xF000, 0x0019), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fcosh.x", size: 4, opcode: two(0xF000, 0x4819), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fcosh.x", size: 4, opcode: two(0xF000, 0x0019), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fdbeq", size: 4, opcode: two(0xF048, 0x0001), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbf", size: 4, opcode: two(0xF048, 0x0000), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbge", size: 4, opcode: two(0xF048, 0x0013), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbgl", size: 4, opcode: two(0xF048, 0x0016), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbgle", size: 4, opcode: two(0xF048, 0x0017), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbgt", size: 4, opcode: two(0xF048, 0x0012), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdble", size: 4, opcode: two(0xF048, 0x0015), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdblt", size: 4, opcode: two(0xF048, 0x0014), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbne", size: 4, opcode: two(0xF048, 0x000E), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbnge", size: 4, opcode: two(0xF048, 0x001C), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbngl", size: 4, opcode: two(0xF048, 0x0019), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbngle", size: 4, opcode: two(0xF048, 0x0018), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbngt", size: 4, opcode: two(0xF048, 0x001D), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbnle", size: 4, opcode: two(0xF048, 0x001A), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbnlt", size: 4, opcode: two(0xF048, 0x001B), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdboge", size: 4, opcode: two(0xF048, 0x0003), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbogl", size: 4, opcode: two(0xF048, 0x0006), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbogt", size: 4, opcode: two(0xF048, 0x0002), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbole", size: 4, opcode: two(0xF048, 0x0005), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbolt", size: 4, opcode: two(0xF048, 0x0004), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbor", size: 4, opcode: two(0xF048, 0x0007), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbseq", size: 4, opcode: two(0xF048, 0x0011), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbsf", size: 4, opcode: two(0xF048, 0x0010), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbsne", size: 4, opcode: two(0xF048, 0x001E), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbst", size: 4, opcode: two(0xF048, 0x001F), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbt", size: 4, opcode: two(0xF048, 0x000F), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbueq", size: 4, opcode: two(0xF048, 0x0009), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbuge", size: 4, opcode: two(0xF048, 0x000B), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbugt", size: 4, opcode: two(0xF048, 0x000A), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbule", size: 4, opcode: two(0xF048, 0x000D), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbult", size: 4, opcode: two(0xF048, 0x000C), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdbun", size: 4, opcode: two(0xF048, 0x0008), match: two(0xF1F8, 0xFFFF), args: "IiDsBw", arch: mfloat, type: dis.condbranch },
    { name: "fdiv.b", size: 4, opcode: two(0xF000, 0x5820), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fdiv.d", size: 4, opcode: two(0xF000, 0x5420), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fdiv.l", size: 4, opcode: two(0xF000, 0x4020), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fdiv.p", size: 4, opcode: two(0xF000, 0x4C20), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fdiv.s", size: 4, opcode: two(0xF000, 0x4420), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fdiv.w", size: 4, opcode: two(0xF000, 0x5020), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fdiv.x", size: 4, opcode: two(0xF000, 0x0020), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fdiv.x", size: 4, opcode: two(0xF000, 0x4820), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsdiv.b", size: 4, opcode: two(0xF000, 0x5860), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsdiv.d", size: 4, opcode: two(0xF000, 0x5460), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsdiv.l", size: 4, opcode: two(0xF000, 0x4060), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsdiv.p", size: 4, opcode: two(0xF000, 0x4C60), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsdiv.s", size: 4, opcode: two(0xF000, 0x4460), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsdiv.w", size: 4, opcode: two(0xF000, 0x5060), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsdiv.x", size: 4, opcode: two(0xF000, 0x0060), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fsdiv.x", size: 4, opcode: two(0xF000, 0x4860), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fddiv.b", size: 4, opcode: two(0xF000, 0x5864), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fddiv.d", size: 4, opcode: two(0xF000, 0x5464), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fddiv.l", size: 4, opcode: two(0xF000, 0x4064), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fddiv.p", size: 4, opcode: two(0xF000, 0x4C64), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fddiv.s", size: 4, opcode: two(0xF000, 0x4464), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fddiv.w", size: 4, opcode: two(0xF000, 0x5064), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fddiv.x", size: 4, opcode: two(0xF000, 0x0064), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fddiv.x", size: 4, opcode: two(0xF000, 0x4864), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fetox.b", size: 4, opcode: two(0xF000, 0x5810), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetox.d", size: 4, opcode: two(0xF000, 0x5410), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetox.l", size: 4, opcode: two(0xF000, 0x4010), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetox.p", size: 4, opcode: two(0xF000, 0x4C10), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetox.s", size: 4, opcode: two(0xF000, 0x4410), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetox.w", size: 4, opcode: two(0xF000, 0x5010), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetox.x", size: 4, opcode: two(0xF000, 0x0010), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fetox.x", size: 4, opcode: two(0xF000, 0x4810), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetox.x", size: 4, opcode: two(0xF000, 0x0010), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fetoxm1.b", size: 4, opcode: two(0xF000, 0x5808), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetoxm1.d", size: 4, opcode: two(0xF000, 0x5408), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetoxm1.l", size: 4, opcode: two(0xF000, 0x4008), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetoxm1.p", size: 4, opcode: two(0xF000, 0x4C08), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetoxm1.s", size: 4, opcode: two(0xF000, 0x4408), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetoxm1.w", size: 4, opcode: two(0xF000, 0x5008), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetoxm1.x", size: 4, opcode: two(0xF000, 0x0008), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fetoxm1.x", size: 4, opcode: two(0xF000, 0x4808), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fetoxm1.x", size: 4, opcode: two(0xF000, 0x0008), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fgetexp.b", size: 4, opcode: two(0xF000, 0x581E), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetexp.d", size: 4, opcode: two(0xF000, 0x541E), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetexp.l", size: 4, opcode: two(0xF000, 0x401E), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetexp.p", size: 4, opcode: two(0xF000, 0x4C1E), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetexp.s", size: 4, opcode: two(0xF000, 0x441E), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetexp.w", size: 4, opcode: two(0xF000, 0x501E), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetexp.x", size: 4, opcode: two(0xF000, 0x001E), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetexp.x", size: 4, opcode: two(0xF000, 0x481E), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetexp.x", size: 4, opcode: two(0xF000, 0x001E), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fgetman.b", size: 4, opcode: two(0xF000, 0x581F), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetman.d", size: 4, opcode: two(0xF000, 0x541F), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetman.l", size: 4, opcode: two(0xF000, 0x401F), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetman.p", size: 4, opcode: two(0xF000, 0x4C1F), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetman.s", size: 4, opcode: two(0xF000, 0x441F), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetman.w", size: 4, opcode: two(0xF000, 0x501F), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetman.x", size: 4, opcode: two(0xF000, 0x001F), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetman.x", size: 4, opcode: two(0xF000, 0x481F), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fgetman.x", size: 4, opcode: two(0xF000, 0x001F), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fint.b", size: 4, opcode: two(0xF000, 0x5801), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fint.d", size: 4, opcode: two(0xF000, 0x5401), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fint.l", size: 4, opcode: two(0xF000, 0x4001), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fint.p", size: 4, opcode: two(0xF000, 0x4C01), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fint.s", size: 4, opcode: two(0xF000, 0x4401), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fint.w", size: 4, opcode: two(0xF000, 0x5001), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fint.x", size: 4, opcode: two(0xF000, 0x0001), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fint.x", size: 4, opcode: two(0xF000, 0x4801), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fint.x", size: 4, opcode: two(0xF000, 0x0001), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fintrz.b", size: 4, opcode: two(0xF000, 0x5803), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fintrz.d", size: 4, opcode: two(0xF000, 0x5403), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fintrz.l", size: 4, opcode: two(0xF000, 0x4003), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fintrz.p", size: 4, opcode: two(0xF000, 0x4C03), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fintrz.s", size: 4, opcode: two(0xF000, 0x4403), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fintrz.w", size: 4, opcode: two(0xF000, 0x5003), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fintrz.x", size: 4, opcode: two(0xF000, 0x0003), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fintrz.x", size: 4, opcode: two(0xF000, 0x4803), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fintrz.x", size: 4, opcode: two(0xF000, 0x0003), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "flog10.b", size: 4, opcode: two(0xF000, 0x5815), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog10.d", size: 4, opcode: two(0xF000, 0x5415), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog10.l", size: 4, opcode: two(0xF000, 0x4015), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog10.p", size: 4, opcode: two(0xF000, 0x4C15), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog10.s", size: 4, opcode: two(0xF000, 0x4415), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog10.w", size: 4, opcode: two(0xF000, 0x5015), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog10.x", size: 4, opcode: two(0xF000, 0x0015), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "flog10.x", size: 4, opcode: two(0xF000, 0x4815), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog10.x", size: 4, opcode: two(0xF000, 0x0015), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "flog2.b", size: 4, opcode: two(0xF000, 0x5816), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog2.d", size: 4, opcode: two(0xF000, 0x5416), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog2.l", size: 4, opcode: two(0xF000, 0x4016), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog2.p", size: 4, opcode: two(0xF000, 0x4C16), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog2.s", size: 4, opcode: two(0xF000, 0x4416), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog2.w", size: 4, opcode: two(0xF000, 0x5016), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog2.x", size: 4, opcode: two(0xF000, 0x0016), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "flog2.x", size: 4, opcode: two(0xF000, 0x4816), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "flog2.x", size: 4, opcode: two(0xF000, 0x0016), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "flogn.b", size: 4, opcode: two(0xF000, 0x5814), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "flogn.d", size: 4, opcode: two(0xF000, 0x5414), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "flogn.l", size: 4, opcode: two(0xF000, 0x4014), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "flogn.p", size: 4, opcode: two(0xF000, 0x4C14), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "flogn.s", size: 4, opcode: two(0xF000, 0x4414), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "flogn.w", size: 4, opcode: two(0xF000, 0x5014), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "flogn.x", size: 4, opcode: two(0xF000, 0x0014), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "flogn.x", size: 4, opcode: two(0xF000, 0x4814), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "flogn.x", size: 4, opcode: two(0xF000, 0x0014), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "flognp1.b", size: 4, opcode: two(0xF000, 0x5806), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "flognp1.d", size: 4, opcode: two(0xF000, 0x5406), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "flognp1.l", size: 4, opcode: two(0xF000, 0x4006), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "flognp1.p", size: 4, opcode: two(0xF000, 0x4C06), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "flognp1.s", size: 4, opcode: two(0xF000, 0x4406), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "flognp1.w", size: 4, opcode: two(0xF000, 0x5006), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "flognp1.x", size: 4, opcode: two(0xF000, 0x0006), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "flognp1.x", size: 4, opcode: two(0xF000, 0x4806), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "flognp1.x", size: 4, opcode: two(0xF000, 0x0006), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fmod.b", size: 4, opcode: two(0xF000, 0x5821), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmod.d", size: 4, opcode: two(0xF000, 0x5421), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmod.l", size: 4, opcode: two(0xF000, 0x4021), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmod.p", size: 4, opcode: two(0xF000, 0x4C21), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmod.s", size: 4, opcode: two(0xF000, 0x4421), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmod.w", size: 4, opcode: two(0xF000, 0x5021), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmod.x", size: 4, opcode: two(0xF000, 0x0021), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fmod.x", size: 4, opcode: two(0xF000, 0x4821), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.b", size: 4, opcode: two(0xF000, 0x5800), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.b", size: 4, opcode: two(0xF000, 0x7800), match: two(0xF1C0, 0xFC7F), args: "IiF7$b", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.d", size: 4, opcode: two(0xF000, 0x5400), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.d", size: 4, opcode: two(0xF000, 0x7400), match: two(0xF1C0, 0xFC7F), args: "IiF7~F", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.l", size: 4, opcode: two(0xF000, 0x4000), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.l", size: 4, opcode: two(0xF000, 0x6000), match: two(0xF1C0, 0xFC7F), args: "IiF7$l", arch: mfloat, type: dis.nonbranch },
    // FIXME: the next two variants should not permit moving an address register to anything but the floating point instruction register.
    { name: "fmove.l", size: 4, opcode: two(0xF000, 0xA000), match: two(0xF1C0, 0xE3FF), args: "Iis8%s", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.l", size: 4, opcode: two(0xF000, 0x8000), match: two(0xF1C0, 0xE3FF), args: "Ii*ls8", arch: mfloat, type: dis.nonbranch },
    // Move the FP control registers.
    { name: "fmove.p", size: 4, opcode: two(0xF000, 0x4C00), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.p", size: 4, opcode: two(0xF000, 0x6C00), match: two(0xF1C0, 0xFC00), args: "IiF7~pkC", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.p", size: 4, opcode: two(0xF000, 0x7C00), match: two(0xF1C0, 0xFC0F), args: "IiF7~pDk", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.s", size: 4, opcode: two(0xF000, 0x4400), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.s", size: 4, opcode: two(0xF000, 0x6400), match: two(0xF1C0, 0xFC7F), args: "IiF7$f", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.w", size: 4, opcode: two(0xF000, 0x5000), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.w", size: 4, opcode: two(0xF000, 0x7000), match: two(0xF1C0, 0xFC7F), args: "IiF7$w", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.x", size: 4, opcode: two(0xF000, 0x0000), match: two(0xF1FF, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.x", size: 4, opcode: two(0xF000, 0x4800), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmove.x", size: 4, opcode: two(0xF000, 0x6800), match: two(0xF1C0, 0xFC7F), args: "IiF7~x", arch: mfloat, type: dis.nonbranch },
    { name: "fsmove.b", size: 4, opcode: two(0xF000, 0x5840), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmove.d", size: 4, opcode: two(0xF000, 0x5440), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmove.l", size: 4, opcode: two(0xF000, 0x4040), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmove.s", size: 4, opcode: two(0xF000, 0x4440), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmove.w", size: 4, opcode: two(0xF000, 0x5040), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmove.x", size: 4, opcode: two(0xF000, 0x0040), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmove.x", size: 4, opcode: two(0xF000, 0x4840), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmove.p", size: 4, opcode: two(0xF000, 0x4C40), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmove.b", size: 4, opcode: two(0xF000, 0x5844), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmove.d", size: 4, opcode: two(0xF000, 0x5444), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmove.l", size: 4, opcode: two(0xF000, 0x4044), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmove.s", size: 4, opcode: two(0xF000, 0x4444), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmove.w", size: 4, opcode: two(0xF000, 0x5044), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmove.x", size: 4, opcode: two(0xF000, 0x0044), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmove.x", size: 4, opcode: two(0xF000, 0x4844), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmove.p", size: 4, opcode: two(0xF000, 0x4C44), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fmovecrx", size: 4, opcode: two(0xF000, 0x5C00), match: two(0xF1FF, 0xFC00), args: "Ii#CF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF000, 0xF800), match: two(0xF1C0, 0xFF8F), args: "IiDk&s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF020, 0xE800), match: two(0xF1F8, 0xFF8F), args: "IiDk-s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF000, 0xD800), match: two(0xF1C0, 0xFF8F), args: "Ii&sDk", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF018, 0xD800), match: two(0xF1F8, 0xFF8F), args: "Ii+sDk", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF000, 0xF000), match: two(0xF1C0, 0xFF00), args: "Idl3&s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF000, 0xF000), match: two(0xF1C0, 0xFF00), args: "Id#3&s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF000, 0xD000), match: two(0xF1C0, 0xFF00), args: "Id&sl3", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF000, 0xD000), match: two(0xF1C0, 0xFF00), args: "Id&s#3", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF020, 0xE000), match: two(0xF1F8, 0xFF00), args: "IdL3-s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF020, 0xE000), match: two(0xF1F8, 0xFF00), args: "Id#3-s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF018, 0xD000), match: two(0xF1F8, 0xFF00), args: "Id+sl3", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.x", size: 4, opcode: two(0xF018, 0xD000), match: two(0xF1F8, 0xFF00), args: "Id+s#3", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.l", size: 4, opcode: two(0xF000, 0xA000), match: two(0xF1C0, 0xE3FF), args: "Iis8%s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem.l", size: 4, opcode: two(0xF000, 0xA000), match: two(0xF1C0, 0xE3FF), args: "IiL8~s", arch: mfloat, type: dis.nonbranch },
    // FIXME: In the next instruction, we should only permit %dn if the target is a single register.  We should only permit %an if the target is a single %fpiar.
    { name: "fmovem.l", size: 4, opcode: two(0xF000, 0x8000), match: two(0xF1C0, 0xE3FF), args: "Ii*lL8", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF020, 0xE000), match: two(0xF1F8, 0xFF00), args: "IdL3-s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0xF000), match: two(0xF1C0, 0xFF00), args: "Idl3&s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF018, 0xD000), match: two(0xF1F8, 0xFF00), args: "Id+sl3", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0xD000), match: two(0xF1C0, 0xFF00), args: "Id&sl3", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF020, 0xE000), match: two(0xF1F8, 0xFF00), args: "Id#3-s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF020, 0xE800), match: two(0xF1F8, 0xFF8F), args: "IiDk-s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0xF000), match: two(0xF1C0, 0xFF00), args: "Id#3&s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0xF800), match: two(0xF1C0, 0xFF8F), args: "IiDk&s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF018, 0xD000), match: two(0xF1F8, 0xFF00), args: "Id+s#3", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF018, 0xD800), match: two(0xF1F8, 0xFF8F), args: "Ii+sDk", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0xD000), match: two(0xF1C0, 0xFF00), args: "Id&s#3", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0xD800), match: two(0xF1C0, 0xFF8F), args: "Ii&sDk", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0xA000), match: two(0xF1C0, 0xE3FF), args: "Iis8%s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0x8000), match: two(0xF1C0, 0xE3FF), args: "Ii*ss8", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0xA000), match: two(0xF1C0, 0xE3FF), args: "IiL8~s", arch: mfloat, type: dis.nonbranch },
    { name: "fmovem", size: 4, opcode: two(0xF000, 0x8000), match: two(0xF2C0, 0xE3FF), args: "Ii*sL8", arch: mfloat, type: dis.nonbranch },
    { name: "fmul.b", size: 4, opcode: two(0xF000, 0x5823), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmul.d", size: 4, opcode: two(0xF000, 0x5423), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmul.l", size: 4, opcode: two(0xF000, 0x4023), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmul.p", size: 4, opcode: two(0xF000, 0x4C23), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmul.s", size: 4, opcode: two(0xF000, 0x4423), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmul.w", size: 4, opcode: two(0xF000, 0x5023), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fmul.x", size: 4, opcode: two(0xF000, 0x0023), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fmul.x", size: 4, opcode: two(0xF000, 0x4823), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsmul.b", size: 4, opcode: two(0xF000, 0x5863), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmul.d", size: 4, opcode: two(0xF000, 0x5463), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmul.l", size: 4, opcode: two(0xF000, 0x4063), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmul.p", size: 4, opcode: two(0xF000, 0x4C63), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmul.s", size: 4, opcode: two(0xF000, 0x4463), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmul.w", size: 4, opcode: two(0xF000, 0x5063), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmul.x", size: 4, opcode: two(0xF000, 0x0063), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fsmul.x", size: 4, opcode: two(0xF000, 0x4863), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmul.b", size: 4, opcode: two(0xF000, 0x5867), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmul.d", size: 4, opcode: two(0xF000, 0x5467), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmul.l", size: 4, opcode: two(0xF000, 0x4067), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmul.p", size: 4, opcode: two(0xF000, 0x4C67), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmul.s", size: 4, opcode: two(0xF000, 0x4467), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmul.w", size: 4, opcode: two(0xF000, 0x5067), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmul.x", size: 4, opcode: two(0xF000, 0x0067), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fdmul.x", size: 4, opcode: two(0xF000, 0x4867), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fneg.b", size: 4, opcode: two(0xF000, 0x581A), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fneg.d", size: 4, opcode: two(0xF000, 0x541A), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fneg.l", size: 4, opcode: two(0xF000, 0x401A), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fneg.p", size: 4, opcode: two(0xF000, 0x4C1A), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fneg.s", size: 4, opcode: two(0xF000, 0x441A), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fneg.w", size: 4, opcode: two(0xF000, 0x501A), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fneg.x", size: 4, opcode: two(0xF000, 0x001A), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fneg.x", size: 4, opcode: two(0xF000, 0x481A), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fneg.x", size: 4, opcode: two(0xF000, 0x001A), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fsneg.b", size: 4, opcode: two(0xF000, 0x585A), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsneg.d", size: 4, opcode: two(0xF000, 0x545A), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsneg.l", size: 4, opcode: two(0xF000, 0x405A), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsneg.p", size: 4, opcode: two(0xF000, 0x4C5A), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsneg.s", size: 4, opcode: two(0xF000, 0x445A), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsneg.w", size: 4, opcode: two(0xF000, 0x505A), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsneg.x", size: 4, opcode: two(0xF000, 0x005A), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fsneg.x", size: 4, opcode: two(0xF000, 0x485A), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fsneg.x", size: 4, opcode: two(0xF000, 0x005A), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: m68040up, type: dis.nonbranch },
    { name: "fdneg.b", size: 4, opcode: two(0xF000, 0x585E), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdneg.d", size: 4, opcode: two(0xF000, 0x545E), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdneg.l", size: 4, opcode: two(0xF000, 0x405E), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdneg.p", size: 4, opcode: two(0xF000, 0x4C5E), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdneg.s", size: 4, opcode: two(0xF000, 0x445E), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdneg.w", size: 4, opcode: two(0xF000, 0x505E), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdneg.x", size: 4, opcode: two(0xF000, 0x005E), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fdneg.x", size: 4, opcode: two(0xF000, 0x485E), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdneg.x", size: 4, opcode: two(0xF000, 0x005E), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: m68040up, type: dis.nonbranch },
    { name: "frem.b", size: 4, opcode: two(0xF000, 0x5825), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "frem.d", size: 4, opcode: two(0xF000, 0x5425), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "frem.l", size: 4, opcode: two(0xF000, 0x4025), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "frem.p", size: 4, opcode: two(0xF000, 0x4C25), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "frem.s", size: 4, opcode: two(0xF000, 0x4425), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "frem.w", size: 4, opcode: two(0xF000, 0x5025), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "frem.x", size: 4, opcode: two(0xF000, 0x0025), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "frem.x", size: 4, opcode: two(0xF000, 0x4825), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "frestore", size: 2, opcode: one(0xF140), match: one(0xF1C0), args: "Id<s", arch: mfloat, type: dis.nonbranch },
    { name: "fsave", size: 2, opcode: one(0xF100), match: one(0xF1C0), args: "Id>s", arch: mfloat, type: dis.nonbranch },
    { name: "fscale.b", size: 4, opcode: two(0xF000, 0x5826), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fscale.d", size: 4, opcode: two(0xF000, 0x5426), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fscale.l", size: 4, opcode: two(0xF000, 0x4026), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fscale.p", size: 4, opcode: two(0xF000, 0x4C26), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fscale.s", size: 4, opcode: two(0xF000, 0x4426), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fscale.w", size: 4, opcode: two(0xF000, 0x5026), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fscale.x", size: 4, opcode: two(0xF000, 0x0026), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fscale.x", size: 4, opcode: two(0xF000, 0x4826), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    // $ is necessary to prevent the assembler from using PC-relative.If used, "label: fseq label" could produce "ftrapeq", size: 2, because "label" became "pc@label".
    { name: "fseq", size: 4, opcode: two(0xF040, 0x0001), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsf", size: 4, opcode: two(0xF040, 0x0000), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsge", size: 4, opcode: two(0xF040, 0x0013), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsgl", size: 4, opcode: two(0xF040, 0x0016), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsgle", size: 4, opcode: two(0xF040, 0x0017), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsgt", size: 4, opcode: two(0xF040, 0x0012), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsle", size: 4, opcode: two(0xF040, 0x0015), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fslt", size: 4, opcode: two(0xF040, 0x0014), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsne", size: 4, opcode: two(0xF040, 0x000E), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsnge", size: 4, opcode: two(0xF040, 0x001C), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsngl", size: 4, opcode: two(0xF040, 0x0019), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsngle", size: 4, opcode: two(0xF040, 0x0018), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsngt", size: 4, opcode: two(0xF040, 0x001D), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsnle", size: 4, opcode: two(0xF040, 0x001A), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsnlt", size: 4, opcode: two(0xF040, 0x001B), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsoge", size: 4, opcode: two(0xF040, 0x0003), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsogl", size: 4, opcode: two(0xF040, 0x0006), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsogt", size: 4, opcode: two(0xF040, 0x0002), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsole", size: 4, opcode: two(0xF040, 0x0005), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsolt", size: 4, opcode: two(0xF040, 0x0004), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsor", size: 4, opcode: two(0xF040, 0x0007), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsseq", size: 4, opcode: two(0xF040, 0x0011), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fssf", size: 4, opcode: two(0xF040, 0x0010), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fssne", size: 4, opcode: two(0xF040, 0x001E), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsst", size: 4, opcode: two(0xF040, 0x001F), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fst", size: 4, opcode: two(0xF040, 0x000F), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsueq", size: 4, opcode: two(0xF040, 0x0009), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsuge", size: 4, opcode: two(0xF040, 0x000B), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsugt", size: 4, opcode: two(0xF040, 0x000A), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsule", size: 4, opcode: two(0xF040, 0x000D), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsult", size: 4, opcode: two(0xF040, 0x000C), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsun", size: 4, opcode: two(0xF040, 0x0008), match: two(0xF1C0, 0xFFFF), args: "Ii$s", arch: mfloat, type: dis.nonbranch },
    { name: "fsgldiv.b", size: 4, opcode: two(0xF000, 0x5824), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsgldiv.d", size: 4, opcode: two(0xF000, 0x5424), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsgldiv.l", size: 4, opcode: two(0xF000, 0x4024), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsgldiv.p", size: 4, opcode: two(0xF000, 0x4C24), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsgldiv.s", size: 4, opcode: two(0xF000, 0x4424), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsgldiv.w", size: 4, opcode: two(0xF000, 0x5024), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsgldiv.x", size: 4, opcode: two(0xF000, 0x0024), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsgldiv.x", size: 4, opcode: two(0xF000, 0x4824), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsgldiv.x", size: 4, opcode: two(0xF000, 0x0024), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fsglmul.b", size: 4, opcode: two(0xF000, 0x5827), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsglmul.d", size: 4, opcode: two(0xF000, 0x5427), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsglmul.l", size: 4, opcode: two(0xF000, 0x4027), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsglmul.p", size: 4, opcode: two(0xF000, 0x4C27), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsglmul.s", size: 4, opcode: two(0xF000, 0x4427), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsglmul.w", size: 4, opcode: two(0xF000, 0x5027), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsglmul.x", size: 4, opcode: two(0xF000, 0x0027), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsglmul.x", size: 4, opcode: two(0xF000, 0x4827), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsglmul.x", size: 4, opcode: two(0xF000, 0x0027), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fsin.b", size: 4, opcode: two(0xF000, 0x580E), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsin.d", size: 4, opcode: two(0xF000, 0x540E), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsin.l", size: 4, opcode: two(0xF000, 0x400E), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsin.p", size: 4, opcode: two(0xF000, 0x4C0E), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsin.s", size: 4, opcode: two(0xF000, 0x440E), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsin.w", size: 4, opcode: two(0xF000, 0x500E), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsin.x", size: 4, opcode: two(0xF000, 0x000E), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsin.x", size: 4, opcode: two(0xF000, 0x480E), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsin.x", size: 4, opcode: two(0xF000, 0x000E), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fsincos.b", size: 4, opcode: two(0xF000, 0x5830), match: two(0xF1C0, 0xFC78), args: "Ii;bF3F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsincos.d", size: 4, opcode: two(0xF000, 0x5430), match: two(0xF1C0, 0xFC78), args: "Ii;FF3F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsincos.l", size: 4, opcode: two(0xF000, 0x4030), match: two(0xF1C0, 0xFC78), args: "Ii;lF3F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsincos.p", size: 4, opcode: two(0xF000, 0x4C30), match: two(0xF1C0, 0xFC78), args: "Ii;pF3F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsincos.s", size: 4, opcode: two(0xF000, 0x4430), match: two(0xF1C0, 0xFC78), args: "Ii;fF3F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsincos.w", size: 4, opcode: two(0xF000, 0x5030), match: two(0xF1C0, 0xFC78), args: "Ii;wF3F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsincos.x", size: 4, opcode: two(0xF000, 0x0030), match: two(0xF1C0, 0xE078), args: "IiF8F3F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsincos.x", size: 4, opcode: two(0xF000, 0x4830), match: two(0xF1C0, 0xFC78), args: "Ii;xF3F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsinh.b", size: 4, opcode: two(0xF000, 0x5802), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsinh.d", size: 4, opcode: two(0xF000, 0x5402), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsinh.l", size: 4, opcode: two(0xF000, 0x4002), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsinh.p", size: 4, opcode: two(0xF000, 0x4C02), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsinh.s", size: 4, opcode: two(0xF000, 0x4402), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsinh.w", size: 4, opcode: two(0xF000, 0x5002), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsinh.x", size: 4, opcode: two(0xF000, 0x0002), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsinh.x", size: 4, opcode: two(0xF000, 0x4802), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsinh.x", size: 4, opcode: two(0xF000, 0x0002), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fsqrt.b", size: 4, opcode: two(0xF000, 0x5804), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsqrt.d", size: 4, opcode: two(0xF000, 0x5404), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsqrt.l", size: 4, opcode: two(0xF000, 0x4004), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsqrt.p", size: 4, opcode: two(0xF000, 0x4C04), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsqrt.s", size: 4, opcode: two(0xF000, 0x4404), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsqrt.w", size: 4, opcode: two(0xF000, 0x5004), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsqrt.x", size: 4, opcode: two(0xF000, 0x0004), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsqrt.x", size: 4, opcode: two(0xF000, 0x4804), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsqrt.x", size: 4, opcode: two(0xF000, 0x0004), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fssqrt.b", size: 4, opcode: two(0xF000, 0x5841), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssqrt.d", size: 4, opcode: two(0xF000, 0x5441), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssqrt.l", size: 4, opcode: two(0xF000, 0x4041), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssqrt.p", size: 4, opcode: two(0xF000, 0x4C41), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssqrt.s", size: 4, opcode: two(0xF000, 0x4441), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssqrt.w", size: 4, opcode: two(0xF000, 0x5041), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssqrt.x", size: 4, opcode: two(0xF000, 0x0041), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fssqrt.x", size: 4, opcode: two(0xF000, 0x4841), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssqrt.x", size: 4, opcode: two(0xF000, 0x0041), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: m68040up, type: dis.nonbranch },
    { name: "fdsqrt.b", size: 4, opcode: two(0xF000, 0x5845), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsqrt.d", size: 4, opcode: two(0xF000, 0x5445), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsqrt.l", size: 4, opcode: two(0xF000, 0x4045), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsqrt.p", size: 4, opcode: two(0xF000, 0x4C45), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsqrt.s", size: 4, opcode: two(0xF000, 0x4445), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsqrt.w", size: 4, opcode: two(0xF000, 0x5045), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsqrt.x", size: 4, opcode: two(0xF000, 0x0045), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsqrt.x", size: 4, opcode: two(0xF000, 0x4845), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsqrt.x", size: 4, opcode: two(0xF000, 0x0045), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: m68040up, type: dis.nonbranch },
    { name: "fsub.b", size: 4, opcode: two(0xF000, 0x5828), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsub.d", size: 4, opcode: two(0xF000, 0x5428), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsub.l", size: 4, opcode: two(0xF000, 0x4028), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsub.p", size: 4, opcode: two(0xF000, 0x4C28), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsub.s", size: 4, opcode: two(0xF000, 0x4428), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsub.w", size: 4, opcode: two(0xF000, 0x5028), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsub.x", size: 4, opcode: two(0xF000, 0x0028), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "fsub.x", size: 4, opcode: two(0xF000, 0x4828), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "fsub.x", size: 4, opcode: two(0xF000, 0x0028), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "fssub.b", size: 4, opcode: two(0xF000, 0x5868), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssub.d", size: 4, opcode: two(0xF000, 0x5468), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssub.l", size: 4, opcode: two(0xF000, 0x4068), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssub.p", size: 4, opcode: two(0xF000, 0x4C68), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssub.s", size: 4, opcode: two(0xF000, 0x4468), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssub.w", size: 4, opcode: two(0xF000, 0x5068), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssub.x", size: 4, opcode: two(0xF000, 0x0068), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fssub.x", size: 4, opcode: two(0xF000, 0x4868), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fssub.x", size: 4, opcode: two(0xF000, 0x0068), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: m68040up, type: dis.nonbranch },
    { name: "fdsub.b", size: 4, opcode: two(0xF000, 0x586c), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsub.d", size: 4, opcode: two(0xF000, 0x546c), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsub.l", size: 4, opcode: two(0xF000, 0x406c), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsub.p", size: 4, opcode: two(0xF000, 0x4C6c), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsub.s", size: 4, opcode: two(0xF000, 0x446c), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsub.w", size: 4, opcode: two(0xF000, 0x506c), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsub.x", size: 4, opcode: two(0xF000, 0x006c), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsub.x", size: 4, opcode: two(0xF000, 0x486c), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: m68040up, type: dis.nonbranch },
    { name: "fdsub.x", size: 4, opcode: two(0xF000, 0x006c), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: m68040up, type: dis.nonbranch },
    { name: "ftan.b", size: 4, opcode: two(0xF000, 0x580F), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftan.d", size: 4, opcode: two(0xF000, 0x540F), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftan.l", size: 4, opcode: two(0xF000, 0x400F), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftan.p", size: 4, opcode: two(0xF000, 0x4C0F), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftan.s", size: 4, opcode: two(0xF000, 0x440F), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftan.w", size: 4, opcode: two(0xF000, 0x500F), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftan.x", size: 4, opcode: two(0xF000, 0x000F), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "ftan.x", size: 4, opcode: two(0xF000, 0x480F), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftan.x", size: 4, opcode: two(0xF000, 0x000F), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "ftanh.b", size: 4, opcode: two(0xF000, 0x5809), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftanh.d", size: 4, opcode: two(0xF000, 0x5409), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftanh.l", size: 4, opcode: two(0xF000, 0x4009), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftanh.p", size: 4, opcode: two(0xF000, 0x4C09), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftanh.s", size: 4, opcode: two(0xF000, 0x4409), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftanh.w", size: 4, opcode: two(0xF000, 0x5009), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftanh.x", size: 4, opcode: two(0xF000, 0x0009), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "ftanh.x", size: 4, opcode: two(0xF000, 0x4809), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftanh.x", size: 4, opcode: two(0xF000, 0x0009), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "ftentox.b", size: 4, opcode: two(0xF000, 0x5812), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftentox.d", size: 4, opcode: two(0xF000, 0x5412), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftentox.l", size: 4, opcode: two(0xF000, 0x4012), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftentox.p", size: 4, opcode: two(0xF000, 0x4C12), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftentox.s", size: 4, opcode: two(0xF000, 0x4412), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftentox.w", size: 4, opcode: two(0xF000, 0x5012), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftentox.x", size: 4, opcode: two(0xF000, 0x0012), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "ftentox.x", size: 4, opcode: two(0xF000, 0x4812), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftentox.x", size: 4, opcode: two(0xF000, 0x0012), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapeq", size: 4, opcode: two(0xF07C, 0x0001), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapf", size: 4, opcode: two(0xF07C, 0x0000), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapge", size: 4, opcode: two(0xF07C, 0x0013), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapgl", size: 4, opcode: two(0xF07C, 0x0016), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapgle", size: 4, opcode: two(0xF07C, 0x0017), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapgt", size: 4, opcode: two(0xF07C, 0x0012), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftraple", size: 4, opcode: two(0xF07C, 0x0015), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftraplt", size: 4, opcode: two(0xF07C, 0x0014), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapne", size: 4, opcode: two(0xF07C, 0x000E), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapnge", size: 4, opcode: two(0xF07C, 0x001C), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapngl", size: 4, opcode: two(0xF07C, 0x0019), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapngle", size: 4, opcode: two(0xF07C, 0x0018), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapngt", size: 4, opcode: two(0xF07C, 0x001D), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapnle", size: 4, opcode: two(0xF07C, 0x001A), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapnlt", size: 4, opcode: two(0xF07C, 0x001B), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapoge", size: 4, opcode: two(0xF07C, 0x0003), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapogl", size: 4, opcode: two(0xF07C, 0x0006), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapogt", size: 4, opcode: two(0xF07C, 0x0002), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapole", size: 4, opcode: two(0xF07C, 0x0005), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapolt", size: 4, opcode: two(0xF07C, 0x0004), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapor", size: 4, opcode: two(0xF07C, 0x0007), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapseq", size: 4, opcode: two(0xF07C, 0x0011), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapsf", size: 4, opcode: two(0xF07C, 0x0010), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapsne", size: 4, opcode: two(0xF07C, 0x001E), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapst", size: 4, opcode: two(0xF07C, 0x001F), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapt", size: 4, opcode: two(0xF07C, 0x000F), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapueq", size: 4, opcode: two(0xF07C, 0x0009), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapuge", size: 4, opcode: two(0xF07C, 0x000B), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapugt", size: 4, opcode: two(0xF07C, 0x000A), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapule", size: 4, opcode: two(0xF07C, 0x000D), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapult", size: 4, opcode: two(0xF07C, 0x000C), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapun", size: 4, opcode: two(0xF07C, 0x0008), match: two(0xF1FF, 0xFFFF), args: "Ii", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapeq.w", size: 4, opcode: two(0xF07A, 0x0001), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapf.w", size: 4, opcode: two(0xF07A, 0x0000), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapge.w", size: 4, opcode: two(0xF07A, 0x0013), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapgl.w", size: 4, opcode: two(0xF07A, 0x0016), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapgle.w", size: 4, opcode: two(0xF07A, 0x0017), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapgt.w", size: 4, opcode: two(0xF07A, 0x0012), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftraple.w", size: 4, opcode: two(0xF07A, 0x0015), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftraplt.w", size: 4, opcode: two(0xF07A, 0x0014), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapne.w", size: 4, opcode: two(0xF07A, 0x000E), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapnge.w", size: 4, opcode: two(0xF07A, 0x001C), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapngl.w", size: 4, opcode: two(0xF07A, 0x0019), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapngle.w", size: 4, opcode: two(0xF07A, 0x0018), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapngt.w", size: 4, opcode: two(0xF07A, 0x001D), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapnle.w", size: 4, opcode: two(0xF07A, 0x001A), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapnlt.w", size: 4, opcode: two(0xF07A, 0x001B), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapoge.w", size: 4, opcode: two(0xF07A, 0x0003), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapogl.w", size: 4, opcode: two(0xF07A, 0x0006), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapogt.w", size: 4, opcode: two(0xF07A, 0x0002), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapolew.", size: 4, opcode: two(0xF07A, 0x0005), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapolt.w", size: 4, opcode: two(0xF07A, 0x0004), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapor.w", size: 4, opcode: two(0xF07A, 0x0007), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapseq.w", size: 4, opcode: two(0xF07A, 0x0011), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapsf.w", size: 4, opcode: two(0xF07A, 0x0010), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapsne.w", size: 4, opcode: two(0xF07A, 0x001E), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapst.w", size: 4, opcode: two(0xF07A, 0x001F), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapt.w", size: 4, opcode: two(0xF07A, 0x000F), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapueq.w", size: 4, opcode: two(0xF07A, 0x0009), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapuge.w", size: 4, opcode: two(0xF07A, 0x000B), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapugt.w", size: 4, opcode: two(0xF07A, 0x000A), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapule.w", size: 4, opcode: two(0xF07A, 0x000D), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapult.w", size: 4, opcode: two(0xF07A, 0x000C), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapun.w", size: 4, opcode: two(0xF07A, 0x0008), match: two(0xF1FF, 0xFFFF), args: "Ii^w", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapeq.l", size: 4, opcode: two(0xF07B, 0x0001), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapf.l", size: 4, opcode: two(0xF07B, 0x0000), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapge.l", size: 4, opcode: two(0xF07B, 0x0013), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapgl.l", size: 4, opcode: two(0xF07B, 0x0016), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapgle.l", size: 4, opcode: two(0xF07B, 0x0017), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapgt.l", size: 4, opcode: two(0xF07B, 0x0012), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftraple.l", size: 4, opcode: two(0xF07B, 0x0015), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftraplt.l", size: 4, opcode: two(0xF07B, 0x0014), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapne.l", size: 4, opcode: two(0xF07B, 0x000E), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapnge.l", size: 4, opcode: two(0xF07B, 0x001C), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapngl.l", size: 4, opcode: two(0xF07B, 0x0019), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapngle.l", size: 4, opcode: two(0xF07B, 0x0018), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapngt.l", size: 4, opcode: two(0xF07B, 0x001D), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapnle.l", size: 4, opcode: two(0xF07B, 0x001A), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapnlt.l", size: 4, opcode: two(0xF07B, 0x001B), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapoge.l", size: 4, opcode: two(0xF07B, 0x0003), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapogl.l", size: 4, opcode: two(0xF07B, 0x0006), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapogt.l", size: 4, opcode: two(0xF07B, 0x0002), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapole.l", size: 4, opcode: two(0xF07B, 0x0005), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapolt.l", size: 4, opcode: two(0xF07B, 0x0004), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapor.l", size: 4, opcode: two(0xF07B, 0x0007), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapseq.l", size: 4, opcode: two(0xF07B, 0x0011), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapsf.l", size: 4, opcode: two(0xF07B, 0x0010), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapsne.l", size: 4, opcode: two(0xF07B, 0x001E), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapst.l", size: 4, opcode: two(0xF07B, 0x001F), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapt.l", size: 4, opcode: two(0xF07B, 0x000F), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapueq.l", size: 4, opcode: two(0xF07B, 0x0009), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapuge.l", size: 4, opcode: two(0xF07B, 0x000B), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapugt.l", size: 4, opcode: two(0xF07B, 0x000A), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapule.l", size: 4, opcode: two(0xF07B, 0x000D), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapult.l", size: 4, opcode: two(0xF07B, 0x000C), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftrapun.l", size: 4, opcode: two(0xF07B, 0x0008), match: two(0xF1FF, 0xFFFF), args: "Ii^l", arch: mfloat, type: dis.nonbranch },
    { name: "ftst.b", size: 4, opcode: two(0xF000, 0x583A), match: two(0xF1C0, 0xFC7F), args: "Ii;b", arch: mfloat, type: dis.nonbranch },
    { name: "ftst.d", size: 4, opcode: two(0xF000, 0x543A), match: two(0xF1C0, 0xFC7F), args: "Ii;F", arch: mfloat, type: dis.nonbranch },
    { name: "ftst.l", size: 4, opcode: two(0xF000, 0x403A), match: two(0xF1C0, 0xFC7F), args: "Ii;l", arch: mfloat, type: dis.nonbranch },
    { name: "ftst.p", size: 4, opcode: two(0xF000, 0x4C3A), match: two(0xF1C0, 0xFC7F), args: "Ii;p", arch: mfloat, type: dis.nonbranch },
    { name: "ftst.s", size: 4, opcode: two(0xF000, 0x443A), match: two(0xF1C0, 0xFC7F), args: "Ii;f", arch: mfloat, type: dis.nonbranch },
    { name: "ftst.w", size: 4, opcode: two(0xF000, 0x503A), match: two(0xF1C0, 0xFC7F), args: "Ii;w", arch: mfloat, type: dis.nonbranch },
    { name: "ftst.x", size: 4, opcode: two(0xF000, 0x003A), match: two(0xF1C0, 0xE07F), args: "IiF8", arch: mfloat, type: dis.nonbranch },
    { name: "ftst.x", size: 4, opcode: two(0xF000, 0x483A), match: two(0xF1C0, 0xFC7F), args: "Ii;x", arch: mfloat, type: dis.nonbranch },
    { name: "ftwotox.b", size: 4, opcode: two(0xF000, 0x5811), match: two(0xF1C0, 0xFC7F), args: "Ii;bF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftwotox.d", size: 4, opcode: two(0xF000, 0x5411), match: two(0xF1C0, 0xFC7F), args: "Ii;FF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftwotox.l", size: 4, opcode: two(0xF000, 0x4011), match: two(0xF1C0, 0xFC7F), args: "Ii;lF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftwotox.p", size: 4, opcode: two(0xF000, 0x4C11), match: two(0xF1C0, 0xFC7F), args: "Ii;pF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftwotox.s", size: 4, opcode: two(0xF000, 0x4411), match: two(0xF1C0, 0xFC7F), args: "Ii;fF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftwotox.w", size: 4, opcode: two(0xF000, 0x5011), match: two(0xF1C0, 0xFC7F), args: "Ii;wF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftwotox.x", size: 4, opcode: two(0xF000, 0x0011), match: two(0xF1C0, 0xE07F), args: "IiF8F7", arch: mfloat, type: dis.nonbranch },
    { name: "ftwotox.x", size: 4, opcode: two(0xF000, 0x4811), match: two(0xF1C0, 0xFC7F), args: "Ii;xF7", arch: mfloat, type: dis.nonbranch },
    { name: "ftwotox.x", size: 4, opcode: two(0xF000, 0x0011), match: two(0xF1C0, 0xE07F), args: "IiFt", arch: mfloat, type: dis.nonbranch },
    // FLOAT ends here
    { name: "halt", size: 2, opcode: one(19144), match: one(65535), args: "", arch: m68060, type: dis.nonbranch },
    { name: "illegal", size: 2, opcode: one(19196), match: one(65535), args: "", arch: m68000up, type: dis.nonbranch },
    { name: "jmp", size: 2, opcode: one(20160), match: one(65472), args: "!s", arch: m68000up, type: dis.branch },
    { name: "jra", size: 2, opcode: one(24576), match: one(65280), args: "Bb", arch: m68000up, type: dis.branch },
    { name: "jra", size: 2, opcode: one(20160), match: one(65472), args: "!s", arch: m68000up, type: dis.branch },
    { name: "jsr", size: 2, opcode: one(20096), match: one(65472), args: "!s", arch: m68000up, type: dis.jsr },
    { name: "jbsr", size: 2, opcode: one(24832), match: one(65280), args: "Bs", arch: m68000up, type: dis.jsr },
    { name: "jbsr", size: 2, opcode: one(20096), match: one(65472), args: "!s", arch: m68000up, type: dis.jsr },
    { name: "lea", size: 2, opcode: one(16832), match: one(61888), args: "!sAd", arch: m68000up, type: dis.nonbranch },
    { name: "lpstop", size: 6, opcode: two(63488, 448), match: two(65535, 65535), args: "#w", arch: m68060, type: dis.nonbranch },
    { name: "link.w", size: 4, opcode: one(20048), match: one(65528), args: "As#w", arch: m68000up, type: dis.nonbranch },
    { name: "link.l", size: 6, opcode: one(18440), match: one(65528), args: "As#l", arch: m68020up, type: dis.nonbranch },
    { name: "link", size: 4, opcode: one(20048), match: one(65528), args: "As#W", arch: m68000up, type: dis.nonbranch },
    { name: "link", size: 6, opcode: one(18440), match: one(65528), args: "As#l", arch: m68020up, type: dis.nonbranch },
    { name: "lsl.b", size: 2, opcode: one(57608), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsl.b", size: 2, opcode: one(57640), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsl.w", size: 2, opcode: one(57672), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsl.w", size: 2, opcode: one(57704), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsl.w", size: 2, opcode: one(58304), match: one(65472), args: "~s", arch: m68000up, type: dis.nonbranch },
    { name: "lsl.l", size: 2, opcode: one(57736), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsl.l", size: 2, opcode: one(57768), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsr.b", size: 2, opcode: one(57352), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsr.b", size: 2, opcode: one(57384), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsr.w", size: 2, opcode: one(57416), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsr.w", size: 2, opcode: one(57448), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsr.w", size: 2, opcode: one(58048), match: one(65472), args: "~s", arch: m68000up, type: dis.nonbranch },
    { name: "lsr.l", size: 2, opcode: one(57480), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "lsr.l", size: 2, opcode: one(57512), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "movea.l", size: 2, opcode: one(8256), match: one(61888), args: "*lAd", arch: m68000up, type: dis.nonbranch },
    { name: "movea.w", size: 2, opcode: one(12352), match: one(61888), args: "*wAd", arch: m68000up, type: dis.nonbranch },
    { name: "movec", size: 4, opcode: one(20091), match: one(65535), args: "R1Jj", arch: m68010up, type: dis.nonbranch },
    { name: "movec", size: 4, opcode: one(20091), match: one(65535), args: "R1#j", arch: m68010up, type: dis.nonbranch },
    { name: "movec", size: 4, opcode: one(20090), match: one(65535), args: "JjR1", arch: m68010up, type: dis.nonbranch },
    { name: "movec", size: 4, opcode: one(20090), match: one(65535), args: "#jR1", arch: m68010up, type: dis.nonbranch },
    { name: "movem.w", size: 4, opcode: one(18560), match: one(65472), args: "Lw&s", arch: m68000up, type: dis.nonbranch },
    { name: "movem.w", size: 4, opcode: one(18592), match: one(65528), args: "lw-s", arch: m68000up, type: dis.nonbranch },
    { name: "movem.w", size: 4, opcode: one(18560), match: one(65472), args: "#w>s", arch: m68000up, type: dis.nonbranch },
    { name: "movem.w", size: 4, opcode: one(19584), match: one(65472), args: "<sLw", arch: m68000up, type: dis.nonbranch },
    { name: "movem.w", size: 4, opcode: one(19584), match: one(65472), args: "<s#w", arch: m68000up, type: dis.nonbranch },
    { name: "movem.l", size: 4, opcode: one(18624), match: one(65472), args: "Lw&s", arch: m68000up, type: dis.nonbranch },
    { name: "movem.l", size: 4, opcode: one(18656), match: one(65528), args: "lw-s", arch: m68000up, type: dis.nonbranch },
    { name: "movem.l", size: 4, opcode: one(18624), match: one(65472), args: "#w>s", arch: m68000up, type: dis.nonbranch },
    { name: "movem.l", size: 4, opcode: one(19648), match: one(65472), args: "<sLw", arch: m68000up, type: dis.nonbranch },
    { name: "movem.l", size: 4, opcode: one(19648), match: one(65472), args: "<s#w", arch: m68000up, type: dis.nonbranch },
    { name: "movep.w", size: 2, opcode: one(264), match: one(61944), args: "dsDd", arch: m68000up, type: dis.nonbranch },
    { name: "movep.w", size: 2, opcode: one(392), match: one(61944), args: "Ddds", arch: m68000up, type: dis.nonbranch },
    { name: "movep.l", size: 2, opcode: one(328), match: one(61944), args: "dsDd", arch: m68000up, type: dis.nonbranch },
    { name: "movep.l", size: 2, opcode: one(456), match: one(61944), args: "Ddds", arch: m68000up, type: dis.nonbranch },
    { name: "moveq", size: 2, opcode: one(28672), match: one(61696), args: "MsDd", arch: m68000up, type: dis.nonbranch },
    { name: "moveq", size: 2, opcode: one(28672), match: one(61696), args: "#BDd", arch: m68000up, type: dis.nonbranch },
    /* The move opcode can generate the movea and moveq instructions.  */
    { name: "move.b", size: 2, opcode: one(4096), match: one(61440), args: ";b$d", arch: m68000up, type: dis.nonbranch },
    { name: "move.w", size: 2, opcode: one(12288), match: one(61440), args: "*w%d", arch: m68000up, type: dis.nonbranch },
    { name: "move.w", size: 2, opcode: one(16576), match: one(65472), args: "Ss$s", arch: m68000up, type: dis.nonbranch },
    { name: "move.w", size: 2, opcode: one(17088), match: one(65472), args: "Cs$s", arch: m68010up, type: dis.nonbranch },
    { name: "move.w", size: 2, opcode: one(17600), match: one(65472), args: ";wCd", arch: m68000up, type: dis.nonbranch },
    { name: "move.w", size: 2, opcode: one(18112), match: one(65472), args: ";wSd", arch: m68000up, type: dis.nonbranch },
    { name: "move.l", size: 2, opcode: one(28672), match: one(61696), args: "MsDd", arch: m68000up, type: dis.nonbranch },
    { name: "move.l", size: 2, opcode: one(8192), match: one(61440), args: "*l%d", arch: m68000up, type: dis.nonbranch },
    { name: "move.l", size: 2, opcode: one(20064), match: one(65528), args: "AsUd", arch: m68000up, type: dis.nonbranch },
    { name: "move.l", size: 2, opcode: one(20072), match: one(65528), args: "UdAs", arch: m68000up, type: dis.nonbranch },
    { name: "move", size: 2, opcode: one(12288), match: one(61440), args: "*w%d", arch: m68000up, type: dis.nonbranch },
    { name: "move", size: 2, opcode: one(16576), match: one(65472), args: "Ss$s", arch: m68000up, type: dis.nonbranch },
    { name: "move", size: 2, opcode: one(17088), match: one(65472), args: "Cs$s", arch: m68010up, type: dis.nonbranch },
    { name: "move", size: 2, opcode: one(17600), match: one(65472), args: ";wCd", arch: m68000up, type: dis.nonbranch },
    { name: "move", size: 2, opcode: one(18112), match: one(65472), args: ";wSd", arch: m68000up, type: dis.nonbranch },
    { name: "move", size: 2, opcode: one(20064), match: one(65528), args: "AsUd", arch: m68000up, type: dis.nonbranch },
    { name: "move", size: 2, opcode: one(20072), match: one(65528), args: "UdAs", arch: m68000up, type: dis.nonbranch },
    { name: "moves.b", size: 4, opcode: two(3584, 0), match: two(65472, 4095), args: "~sR1", arch: m68010up, type: dis.nonbranch },
    { name: "moves.b", size: 4, opcode: two(3584, 2048), match: two(65472, 4095), args: "R1~s", arch: m68010up, type: dis.nonbranch },
    { name: "moves.w", size: 4, opcode: two(3648, 0), match: two(65472, 4095), args: "~sR1", arch: m68010up, type: dis.nonbranch },
    { name: "moves.w", size: 4, opcode: two(3648, 2048), match: two(65472, 4095), args: "R1~s", arch: m68010up, type: dis.nonbranch },
    { name: "moves.l", size: 4, opcode: two(3712, 0), match: two(65472, 4095), args: "~sR1", arch: m68010up, type: dis.nonbranch },
    { name: "moves.l", size: 4, opcode: two(3712, 2048), match: two(65472, 4095), args: "R1~s", arch: m68010up, type: dis.nonbranch },
    { name: "move16", size: 4, opcode: two(0xf620, 0x8000), match: two(0xfff8, 0x8fff), args: "+s+1", arch: m68040up, type: dis.nonbranch },
    { name: "move16", size: 2, opcode: one(0xf600), match: one(0xfff8), args: "+s_L", arch: m68040up, type: dis.nonbranch },
    { name: "move16", size: 2, opcode: one(0xf608), match: one(0xfff8), args: "_L+s", arch: m68040up, type: dis.nonbranch },
    { name: "move16", size: 2, opcode: one(0xf610), match: one(0xfff8), args: "as_L", arch: m68040up, type: dis.nonbranch },
    { name: "move16", size: 2, opcode: one(0xf618), match: one(0xfff8), args: "_Las", arch: m68040up, type: dis.nonbranch },
    { name: "muls.w", size: 2, opcode: one(49600), match: one(61888), args: ";wDd", arch: m68000up, type: dis.nonbranch },
    { name: "muls.l", size: 4, opcode: two(19456, 2048), match: two(65472, 36856), args: ";lD1", arch: m68020up, type: dis.nonbranch },
    { name: "muls.l", size: 4, opcode: two(19456, 3072), match: two(65472, 36856), args: ";lD3D1", arch: m68020up, type: dis.nonbranch },
    { name: "mulu.w", size: 2, opcode: one(49344), match: one(61888), args: ";wDd", arch: m68000up, type: dis.nonbranch },
    { name: "mulu.l", size: 4, opcode: two(19456, 0), match: two(65472, 36856), args: ";lD1", arch: m68020up, type: dis.nonbranch },
    { name: "mulu.l", size: 4, opcode: two(19456, 1024), match: two(65472, 36856), args: ";lD3D1", arch: m68020up, type: dis.nonbranch },
    { name: "nbcd", size: 2, opcode: one(18432), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "neg.b", size: 2, opcode: one(17408), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "neg.w", size: 2, opcode: one(17472), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "neg.l", size: 2, opcode: one(17536), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "negx.b", size: 2, opcode: one(16384), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "negx.w", size: 2, opcode: one(16448), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "negx.l", size: 2, opcode: one(16512), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "nop", size: 2, opcode: one(20081), match: one(65535), args: "", arch: m68000up, type: dis.nonbranch },
    { name: "not.b", size: 2, opcode: one(17920), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "not.w", size: 2, opcode: one(17984), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "not.l", size: 2, opcode: one(18048), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "ori.b", size: 4, opcode: one(0), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "ori.b", size: 4, opcode: one(60), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "ori.w", size: 4, opcode: one(64), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "ori.w", size: 4, opcode: one(124), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "ori.l", size: 6, opcode: one(128), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "ori", size: 4, opcode: one(60), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "ori", size: 4, opcode: one(64), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "ori", size: 4, opcode: one(124), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    /* The or opcode can generate the ori instruction.  */
    { name: "or.b", size: 4, opcode: one(0), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "or.b", size: 4, opcode: one(60), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "or.b", size: 2, opcode: one(32768), match: one(61888), args: ";bDd", arch: m68000up, type: dis.nonbranch },
    { name: "or.b", size: 2, opcode: one(33024), match: one(61888), args: "Dd~s", arch: m68000up, type: dis.nonbranch },
    { name: "or.w", size: 4, opcode: one(64), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "or.w", size: 4, opcode: one(124), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "or.w", size: 2, opcode: one(32832), match: one(61888), args: ";wDd", arch: m68000up, type: dis.nonbranch },
    { name: "or.w", size: 2, opcode: one(33088), match: one(61888), args: "Dd~s", arch: m68000up, type: dis.nonbranch },
    { name: "or.l", size: 6, opcode: one(128), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "or.l", size: 2, opcode: one(32896), match: one(61888), args: ";lDd", arch: m68000up, type: dis.nonbranch },
    { name: "or.l", size: 2, opcode: one(33152), match: one(61888), args: "Dd~s", arch: m68000up, type: dis.nonbranch },
    { name: "or", size: 4, opcode: one(60), match: one(65535), args: "#bCs", arch: m68000up, type: dis.nonbranch },
    { name: "or", size: 4, opcode: one(64), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "or", size: 4, opcode: one(124), match: one(65535), args: "#wSs", arch: m68000up, type: dis.nonbranch },
    { name: "or", size: 2, opcode: one(32832), match: one(61888), args: ";wDd", arch: m68000up, type: dis.nonbranch },
    { name: "or", size: 2, opcode: one(33088), match: one(61888), args: "Dd~s", arch: m68000up, type: dis.nonbranch },
    { name: "pack", size: 4, opcode: one(33088), match: one(61944), args: "DsDd#w", arch: m68020up, type: dis.nonbranch },
    { name: "pack", size: 4, opcode: one(33096), match: one(61944), args: "-s-d#w", arch: m68020up, type: dis.nonbranch },
    // MMU opcodes missing here
    { name: "pea", size: 2, opcode: one(18496), match: one(65472), args: "!s", arch: m68000up, type: dis.nonbranch },
    { name: "pflusha", size: 2, opcode: one(0xf518), match: one(0xfff8), args: "", arch: m68040up, type: dis.nonbranch },
    { name: "pflusha", size: 4, opcode: two(0xf000, 0x2400), match: two(0xffff, 0xffff), args: "", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pflush", size: 4, opcode: two(0xf000, 0x3010), match: two(0xffc0, 0xfe10), args: "T3T9", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pflush", size: 4, opcode: two(0xf000, 0x3810), match: two(0xffc0, 0xfe10), args: "T3T9&s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pflush", size: 4, opcode: two(0xf000, 0x3008), match: two(0xffc0, 0xfe18), args: "D3T9", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pflush", size: 4, opcode: two(0xf000, 0x3808), match: two(0xffc0, 0xfe18), args: "D3T9&s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pflush", size: 4, opcode: two(0xf000, 0x3000), match: two(0xffc0, 0xfe1e), args: "f3T9", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pflush", size: 4, opcode: two(0xf000, 0x3800), match: two(0xffc0, 0xfe1e), args: "f3T9&s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pflush", size: 2, opcode: one(0xf508), match: one(0xfff8), args: "as", arch: m68040up, type: dis.nonbranch },
    { name: "pflush", size: 2, opcode: one(0xf508), match: one(0xfff8), args: "As", arch: m68040up, type: dis.nonbranch },
    { name: "pflushan", size: 2, opcode: one(0xf510), match: one(0xfff8), args: "", arch: m68040up, type: dis.nonbranch },
    { name: "pflushn", size: 2, opcode: one(0xf500), match: one(0xfff8), args: "as", arch: m68040up, type: dis.nonbranch },
    { name: "pflushn", size: 2, opcode: one(0xf500), match: one(0xfff8), args: "As", arch: m68040up, type: dis.nonbranch },
    { name: "ploadr", size: 4, opcode: two(0xf000, 0x2210), match: two(0xffc0, 0xfff0), args: "T3&s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ploadr", size: 4, opcode: two(0xf000, 0x2208), match: two(0xffc0, 0xfff8), args: "D3&s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ploadr", size: 4, opcode: two(0xf000, 0x2200), match: two(0xffc0, 0xfffe), args: "f3&s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ploadw", size: 4, opcode: two(0xf000, 0x2010), match: two(0xffc0, 0xfff0), args: "T3&s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ploadw", size: 4, opcode: two(0xf000, 0x2008), match: two(0xffc0, 0xfff8), args: "D3&s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ploadw", size: 4, opcode: two(0xf000, 0x2000), match: two(0xffc0, 0xfffe), args: "f3&s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "plpar", size: 2, opcode: one(0xf5c8), match: one(0xfff8), args: "as", arch: m68060, type: dis.nonbranch },
    { name: "plpaw", size: 2, opcode: one(0xf588), match: one(0xfff8), args: "as", arch: m68060, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x4000), match: two(0xffc0, 0xffff), args: "*l08", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x4200), match: two(0xffc0, 0xffff), args: "08%s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x5e00), match: two(0xffc0, 0xffff), args: "18%s", arch: m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x4200), match: two(0xffc0, 0xe3ff), args: "28%s", arch: m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x4000), match: two(0xffc0, 0xe3ff), args: "|sW8", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x4200), match: two(0xffc0, 0xe3ff), args: "W8~s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x6000), match: two(0xffc0, 0xffff), args: "*wY8", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x6200), match: two(0xffc0, 0xffff), args: "Y8%s", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x6600), match: two(0xffc0, 0xffff), args: "Z8%s", arch: m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x6000), match: two(0xffc0, 0xe3e3), args: "*wX3", arch: m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x6200), match: two(0xffc0, 0xe3e3), args: "X3%s", arch: m68851, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x0800), match: two(0xffc0, 0xfbff), args: "*l38", arch: m68030, type: dis.nonbranch },
    { name: "pmove", size: 4, opcode: two(0xf000, 0x0a00), match: two(0xffc0, 0xfbff), args: "38%s", arch: m68030, type: dis.nonbranch },
    { name: "pmovefd", size: 4, opcode: two(0xf000, 0x4100), match: two(0xffc0, 0xe3ff), args: "*l08", arch: m68030, type: dis.nonbranch },
    { name: "pmovefd", size: 4, opcode: two(0xf000, 0x4100), match: two(0xffc0, 0xe3ff), args: "|sW8", arch: m68030, type: dis.nonbranch },
    { name: "pmovefd", size: 4, opcode: two(0xf000, 0x0900), match: two(0xffc0, 0xfbff), args: "*l38", arch: m68030, type: dis.nonbranch },
    { name: "ptestr", size: 4, opcode: two(0xf000, 0x8210), match: two(0xffc0, 0xe3f0), args: "T3&st8", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestr", size: 4, opcode: two(0xf000, 0x8310), match: two(0xffc0, 0xe310), args: "T3&st8A9", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestr", size: 4, opcode: two(0xf000, 0x8208), match: two(0xffc0, 0xe3f8), args: "D3&st8", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestr", size: 4, opcode: two(0xf000, 0x8308), match: two(0xffc0, 0xe318), args: "D3&st8A9", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestr", size: 4, opcode: two(0xf000, 0x8200), match: two(0xffc0, 0xe3fe), args: "f3&st8", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestr", size: 4, opcode: two(0xf000, 0x8300), match: two(0xffc0, 0xe31e), args: "f3&st8A9", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestr", size: 2, opcode: one(0xf568), match: one(0xfff8), args: "as", arch: m68040, type: dis.nonbranch },
    { name: "ptestw", size: 4, opcode: two(0xf000, 0x8010), match: two(0xffc0, 0xe3f0), args: "T3&st8", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestw", size: 4, opcode: two(0xf000, 0x8110), match: two(0xffc0, 0xe310), args: "T3&st8A9", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestw", size: 4, opcode: two(0xf000, 0x8008), match: two(0xffc0, 0xe3f8), args: "D3&st8", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestw", size: 4, opcode: two(0xf000, 0x8108), match: two(0xffc0, 0xe318), args: "D3&st8A9", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestw", size: 4, opcode: two(0xf000, 0x8000), match: two(0xffc0, 0xe3fe), args: "f3&st8", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestw", size: 4, opcode: two(0xf000, 0x8100), match: two(0xffc0, 0xe31e), args: "f3&st8A9", arch: m68030 | m68851, type: dis.nonbranch },
    { name: "ptestw", size: 2, opcode: one(0xf548), match: one(0xfff8), args: "as", arch: m68040, type: dis.nonbranch },
    { name: "ptrapac.w", size: 6, opcode: two(0xf07a, 0x0007), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapac.l", size: 6, opcode: two(0xf07b, 0x0007), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapac", size: 4, opcode: two(0xf07c, 0x0007), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapas.w", size: 6, opcode: two(0xf07a, 0x0006), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapas.l", size: 6, opcode: two(0xf07b, 0x0006), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapas", size: 4, opcode: two(0xf07c, 0x0006), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapbc.w", size: 6, opcode: two(0xf07a, 0x0001), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapbc.l", size: 6, opcode: two(0xf07b, 0x0001), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapbc", size: 4, opcode: two(0xf07c, 0x0001), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapbs.w", size: 6, opcode: two(0xf07a, 0x0000), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapbs.l", size: 6, opcode: two(0xf07b, 0x0000), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapbs", size: 4, opcode: two(0xf07c, 0x0000), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapcc.w", size: 6, opcode: two(0xf07a, 0x000f), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapcc.l", size: 6, opcode: two(0xf07b, 0x000f), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapcc", size: 4, opcode: two(0xf07c, 0x000f), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapcs.w", size: 6, opcode: two(0xf07a, 0x000e), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapcs.l", size: 6, opcode: two(0xf07b, 0x000e), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapcs", size: 4, opcode: two(0xf07c, 0x000e), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapgc.w", size: 6, opcode: two(0xf07a, 0x000d), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapgc.l", size: 6, opcode: two(0xf07b, 0x000d), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapgc", size: 4, opcode: two(0xf07c, 0x000d), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapgs.w", size: 6, opcode: two(0xf07a, 0x000c), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapgs.l", size: 6, opcode: two(0xf07b, 0x000c), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapgs", size: 4, opcode: two(0xf07c, 0x000c), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapic.w", size: 6, opcode: two(0xf07a, 0x000b), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapic.l", size: 6, opcode: two(0xf07b, 0x000b), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapic", size: 4, opcode: two(0xf07c, 0x000b), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapis.w", size: 6, opcode: two(0xf07a, 0x000a), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapis.l", size: 6, opcode: two(0xf07b, 0x000a), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapis", size: 4, opcode: two(0xf07c, 0x000a), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptraplc.w", size: 6, opcode: two(0xf07a, 0x0003), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptraplc.l", size: 6, opcode: two(0xf07b, 0x0003), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptraplc", size: 4, opcode: two(0xf07c, 0x0003), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapls.w", size: 6, opcode: two(0xf07a, 0x0002), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapls.l", size: 6, opcode: two(0xf07b, 0x0002), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapls", size: 4, opcode: two(0xf07c, 0x0002), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapsc.w", size: 6, opcode: two(0xf07a, 0x0005), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapsc.l", size: 6, opcode: two(0xf07b, 0x0005), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapsc", size: 4, opcode: two(0xf07c, 0x0005), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapss.w", size: 6, opcode: two(0xf07a, 0x0004), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapss.l", size: 6, opcode: two(0xf07b, 0x0004), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapss", size: 4, opcode: two(0xf07c, 0x0004), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapwc.w", size: 6, opcode: two(0xf07a, 0x0009), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapwc.l", size: 6, opcode: two(0xf07b, 0x0009), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapwc", size: 4, opcode: two(0xf07c, 0x0009), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "ptrapws.w", size: 6, opcode: two(0xf07a, 0x0008), match: two(0xffff, 0xffff), args: "#w", arch: m68851, type: dis.nonbranch },
    { name: "ptrapws.l", size: 6, opcode: two(0xf07b, 0x0008), match: two(0xffff, 0xffff), args: "#l", arch: m68851, type: dis.nonbranch },
    { name: "ptrapws", size: 4, opcode: two(0xf07c, 0x0008), match: two(0xffff, 0xffff), args: "", arch: m68851, type: dis.nonbranch },
    { name: "pulse", size: 2, opcode: one(19148), match: one(65535), args: "", arch: m68060, type: dis.nonbranch },
    { name: "reset", size: 2, opcode: one(20080), match: one(65535), args: "", arch: m68000up, type: dis.nonbranch },
    { name: "rol.b", size: 2, opcode: one(57624), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "rol.b", size: 2, opcode: one(57656), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "rol.w", size: 2, opcode: one(57688), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "rol.w", size: 2, opcode: one(57720), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "rol.w", size: 2, opcode: one(59328), match: one(65472), args: "~s", arch: m68000up, type: dis.nonbranch },
    { name: "rol.l", size: 2, opcode: one(57752), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "rol.l", size: 2, opcode: one(57784), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "ror.b", size: 2, opcode: one(57368), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "ror.b", size: 2, opcode: one(57400), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "ror.w", size: 2, opcode: one(57432), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "ror.w", size: 2, opcode: one(57464), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "ror.w", size: 2, opcode: one(59072), match: one(65472), args: "~s", arch: m68000up, type: dis.nonbranch },
    { name: "ror.l", size: 2, opcode: one(57496), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "ror.l", size: 2, opcode: one(57528), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxl.b", size: 2, opcode: one(57616), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxl.b", size: 2, opcode: one(57648), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxl.w", size: 2, opcode: one(57680), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxl.w", size: 2, opcode: one(57712), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxl.w", size: 2, opcode: one(58816), match: one(65472), args: "~s", arch: m68000up, type: dis.nonbranch },
    { name: "roxl.l", size: 2, opcode: one(57744), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxl.l", size: 2, opcode: one(57776), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxr.b", size: 2, opcode: one(57360), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxr.b", size: 2, opcode: one(57392), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxr.w", size: 2, opcode: one(57424), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxr.w", size: 2, opcode: one(57456), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxr.w", size: 2, opcode: one(58560), match: one(65472), args: "~s", arch: m68000up, type: dis.nonbranch },
    { name: "roxr.l", size: 2, opcode: one(57488), match: one(61944), args: "QdDs", arch: m68000up, type: dis.nonbranch },
    { name: "roxr.l", size: 2, opcode: one(57520), match: one(61944), args: "DdDs", arch: m68000up, type: dis.nonbranch },
    { name: "rtd", size: 4, opcode: one(20084), match: one(65535), args: "#w", arch: m68010up, type: dis.nonbranch },
    { name: "rte", size: 2, opcode: one(20083), match: one(65535), args: "", arch: m68000up, type: dis.nonbranch },
    { name: "rtm", size: 2, opcode: one(1728), match: one(65520), args: "Rs", arch: m68020, type: dis.nonbranch },
    { name: "rtr", size: 2, opcode: one(20087), match: one(65535), args: "", arch: m68000up, type: dis.nonbranch },
    { name: "rts", size: 2, opcode: one(20085), match: one(65535), args: "", arch: m68000up, type: dis.nonbranch },
    { name: "sbcd", size: 2, opcode: one(33024), match: one(61944), args: "DsDd", arch: m68000up, type: dis.nonbranch },
    { name: "sbcd", size: 2, opcode: one(33032), match: one(61944), args: "-s-d", arch: m68000up, type: dis.nonbranch },
    // Traps have to come before conditional sets, as they have a more specific opcode.
    { name: "trapcc", size: 2, opcode: one(21756), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapcs", size: 2, opcode: one(22012), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapeq", size: 2, opcode: one(22524), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapf", size: 2, opcode: one(20988), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapge", size: 2, opcode: one(23804), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapgt", size: 2, opcode: one(24316), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "traphi", size: 2, opcode: one(21244), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "traple", size: 2, opcode: one(24572), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapls", size: 2, opcode: one(21500), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "traplt", size: 2, opcode: one(24060), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapmi", size: 2, opcode: one(23548), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapne", size: 2, opcode: one(22268), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trappl", size: 2, opcode: one(23292), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapt", size: 2, opcode: one(20732), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapvc", size: 2, opcode: one(22780), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapvs", size: 2, opcode: one(23036), match: one(65535), args: "", arch: m68020up, type: dis.nonbranch },
    { name: "trapcc.w", size: 4, opcode: one(21754), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapcs.w", size: 4, opcode: one(22010), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapeq.w", size: 4, opcode: one(22522), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapf.w", size: 4, opcode: one(20986), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapge.w", size: 4, opcode: one(23802), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapgt.w", size: 4, opcode: one(24314), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "traphi.w", size: 4, opcode: one(21242), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "traple.w", size: 4, opcode: one(24570), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapls.w", size: 4, opcode: one(21498), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "traplt.w", size: 4, opcode: one(24058), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapmi.w", size: 4, opcode: one(23546), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapne.w", size: 4, opcode: one(22266), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trappl.w", size: 4, opcode: one(23290), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapt.w", size: 4, opcode: one(20730), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapvc.w", size: 4, opcode: one(22778), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapvs.w", size: 4, opcode: one(23034), match: one(65535), args: "#w", arch: m68020up, type: dis.nonbranch },
    { name: "trapcc.l", size: 6, opcode: one(21755), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapcs.l", size: 6, opcode: one(22011), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapeq.l", size: 6, opcode: one(22523), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapf.l", size: 6, opcode: one(20987), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapge.l", size: 6, opcode: one(23803), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapgt.l", size: 6, opcode: one(24315), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "traphi.l", size: 6, opcode: one(21243), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "traple.l", size: 6, opcode: one(24571), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapls.l", size: 6, opcode: one(21499), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "traplt.l", size: 6, opcode: one(24059), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapmi.l", size: 6, opcode: one(23547), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapne.l", size: 6, opcode: one(22267), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trappl.l", size: 6, opcode: one(23291), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapt.l", size: 6, opcode: one(20731), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapvc.l", size: 6, opcode: one(22779), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapvs.l", size: 6, opcode: one(23035), match: one(65535), args: "#l", arch: m68020up, type: dis.nonbranch },
    { name: "trapv", size: 2, opcode: one(20086), match: one(65535), args: "", arch: m68000up, type: dis.nonbranch },
    { name: "scc", size: 2, opcode: one(21696), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "scs", size: 2, opcode: one(21952), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "seq", size: 2, opcode: one(22464), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "sf", size: 2, opcode: one(20928), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "sge", size: 2, opcode: one(23744), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "sgt", size: 2, opcode: one(24256), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "shi", size: 2, opcode: one(21184), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "sle", size: 2, opcode: one(24512), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "sls", size: 2, opcode: one(21440), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "slt", size: 2, opcode: one(24000), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "smi", size: 2, opcode: one(23488), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "sne", size: 2, opcode: one(22208), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "spl", size: 2, opcode: one(23232), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "st", size: 2, opcode: one(20672), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "svc", size: 2, opcode: one(22720), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "svs", size: 2, opcode: one(22976), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "stop", size: 4, opcode: one(20082), match: one(65535), args: "#w", arch: m68000up, type: dis.nonbranch },
    { name: "suba.l", size: 2, opcode: one(37312), match: one(61888), args: "*lAd", arch: m68000up, type: dis.nonbranch },
    { name: "suba.w", size: 2, opcode: one(37056), match: one(61888), args: "*wAd", arch: m68000up, type: dis.nonbranch },
    { name: "subi.b", size: 4, opcode: one(1024), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "subi.w", size: 4, opcode: one(1088), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "subi.l", size: 6, opcode: one(1152), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "subq.b", size: 2, opcode: one(20736), match: one(61888), args: "Qd%s", arch: m68000up, type: dis.nonbranch },
    { name: "subq.w", size: 2, opcode: one(20800), match: one(61888), args: "Qd%s", arch: m68000up, type: dis.nonbranch },
    { name: "subq.l", size: 2, opcode: one(20864), match: one(61888), args: "Qd%s", arch: m68000up, type: dis.nonbranch },
    /* The sub opcode can generate the suba, subi, and subq instructions.  */
    { name: "sub.b", size: 2, opcode: one(20736), match: one(61888), args: "Qd%s", arch: m68000up, type: dis.nonbranch },
    { name: "sub.b", size: 4, opcode: one(1024), match: one(65472), args: "#b$s", arch: m68000up, type: dis.nonbranch },
    { name: "sub.b", size: 2, opcode: one(36864), match: one(61888), args: ";bDd", arch: m68000up, type: dis.nonbranch },
    { name: "sub.b", size: 2, opcode: one(37120), match: one(61888), args: "Dd~s", arch: m68000up, type: dis.nonbranch },
    { name: "sub.w", size: 2, opcode: one(20800), match: one(61888), args: "Qd%s", arch: m68000up, type: dis.nonbranch },
    { name: "sub.w", size: 4, opcode: one(1088), match: one(65472), args: "#w$s", arch: m68000up, type: dis.nonbranch },
    { name: "sub.w", size: 2, opcode: one(37056), match: one(61888), args: "*wAd", arch: m68000up, type: dis.nonbranch },
    { name: "sub.w", size: 2, opcode: one(36928), match: one(61888), args: "*wDd", arch: m68000up, type: dis.nonbranch },
    { name: "sub.w", size: 2, opcode: one(37184), match: one(61888), args: "Dd~s", arch: m68000up, type: dis.nonbranch },
    { name: "sub.l", size: 2, opcode: one(20864), match: one(61888), args: "Qd%s", arch: m68000up, type: dis.nonbranch },
    { name: "sub.l", size: 6, opcode: one(1152), match: one(65472), args: "#l$s", arch: m68000up, type: dis.nonbranch },
    { name: "sub.l", size: 2, opcode: one(37312), match: one(61888), args: "*lAd", arch: m68000up, type: dis.nonbranch },
    { name: "sub.l", size: 2, opcode: one(36992), match: one(61888), args: "*lDd", arch: m68000up, type: dis.nonbranch },
    { name: "sub.l", size: 2, opcode: one(37248), match: one(61888), args: "Dd~s", arch: m68000up, type: dis.nonbranch },
    { name: "subx.b", size: 2, opcode: one(37120), match: one(61944), args: "DsDd", arch: m68000up, type: dis.nonbranch },
    { name: "subx.b", size: 2, opcode: one(37128), match: one(61944), args: "-s-d", arch: m68000up, type: dis.nonbranch },
    { name: "subx.w", size: 2, opcode: one(37184), match: one(61944), args: "DsDd", arch: m68000up, type: dis.nonbranch },
    { name: "subx.w", size: 2, opcode: one(37192), match: one(61944), args: "-s-d", arch: m68000up, type: dis.nonbranch },
    { name: "subx.l", size: 2, opcode: one(37248), match: one(61944), args: "DsDd", arch: m68000up, type: dis.nonbranch },
    { name: "subx.l", size: 2, opcode: one(37256), match: one(61944), args: "-s-d", arch: m68000up, type: dis.nonbranch },
    { name: "swap", size: 2, opcode: one(18496), match: one(65528), args: "Ds", arch: m68000up, type: dis.nonbranch },
    { name: "tas", size: 2, opcode: one(19136), match: one(65472), args: "$s", arch: m68000up, type: dis.nonbranch },
    { name: "trap", size: 2, opcode: one(20032), match: one(65520), args: "Ts", arch: m68000up, type: dis.nonbranch },
    { name: "tst.b", size: 2, opcode: one(18944), match: one(65472), args: ";b", arch: m68020up, type: dis.nonbranch },
    { name: "tst.b", size: 2, opcode: one(18944), match: one(65472), args: "$b", arch: m68000up, type: dis.nonbranch },
    { name: "tst.w", size: 2, opcode: one(19008), match: one(65472), args: "*w", arch: m68020up, type: dis.nonbranch },
    { name: "tst.w", size: 2, opcode: one(19008), match: one(65472), args: "$w", arch: m68000up, type: dis.nonbranch },
    { name: "tst.l", size: 2, opcode: one(19072), match: one(65472), args: "*l", arch: m68020up, type: dis.nonbranch },
    { name: "tst.l", size: 2, opcode: one(19072), match: one(65472), args: "$l", arch: m68000up, type: dis.nonbranch },
    { name: "unlk", size: 2, opcode: one(20056), match: one(65528), args: "As", arch: m68000up, type: dis.nonbranch },
    { name: "unpk", size: 4, opcode: one(33152), match: one(61944), args: "DsDd#w", arch: m68020up, type: dis.nonbranch },
    { name: "unpk", size: 4, opcode: one(33160), match: one(61944), args: "-s-d#w", arch: m68020up, type: dis.nonbranch },
];
// no size suffix
var m68k_opcode_aliases_nosize = [
    { alias: "add", primary: "add.w" },
    { alias: "adda", primary: "adda.w" },
    { alias: "addi", primary: "addi.w" },
    { alias: "addq", primary: "addq.w" },
    { alias: "addx", primary: "addx.w" },
    { alias: "asl", primary: "asl.w" },
    { alias: "asr", primary: "asr.w" },
    { alias: "bhi", primary: "bhi.w" },
    { alias: "bls", primary: "bls.w" },
    { alias: "bcc", primary: "bcc.w" },
    { alias: "bcs", primary: "bcs.w" },
    { alias: "bne", primary: "bne.w" },
    { alias: "beq", primary: "beq.w" },
    { alias: "bvc", primary: "bvc.w" },
    { alias: "bvs", primary: "bvs.w" },
    { alias: "bpl", primary: "bpl.w" },
    { alias: "bmi", primary: "bmi.w" },
    { alias: "bge", primary: "bge.w" },
    { alias: "blt", primary: "blt.w" },
    { alias: "bgt", primary: "bgt.w" },
    { alias: "ble", primary: "ble.w" },
    { alias: "bra", primary: "bra.w" },
    { alias: "bsr", primary: "bsr.w" },
    { alias: "cas2", primary: "cas2.w" },
    { alias: "cas", primary: "cas.w" },
    { alias: "chk2", primary: "chk2.w" },
    { alias: "chk", primary: "chk.w" },
    { alias: "clr", primary: "clr.w" },
    { alias: "cmp2", primary: "cmp2.w" },
    { alias: "cmpa", primary: "cmpa.w" },
    { alias: "cmpi", primary: "cmpi.w" },
    { alias: "cmpm", primary: "cmpm.w" },
    { alias: "cmp", primary: "cmp.w" },
    { alias: "divs", primary: "divs.w" },
    { alias: "divu", primary: "divu.w" },
    { alias: "ext", primary: "ext.w" },
    { alias: "lsl", primary: "lsl.w" },
    { alias: "lsr", primary: "lsr.w" },
    { alias: "movea", primary: "movea.w" },
    { alias: "movem", primary: "movem.w" },
    { alias: "movep", primary: "movep.w" },
    { alias: "muls", primary: "muls.w" },
    { alias: "mulu", primary: "mulu.w" },
    { alias: "neg", primary: "neg.w" },
    { alias: "negx", primary: "negx.w" },
    { alias: "not", primary: "not.w" },
    { alias: "rol", primary: "rol.w" },
    { alias: "ror", primary: "ror.w" },
    { alias: "roxl", primary: "roxl.w" },
    { alias: "roxr", primary: "roxr.w" },
    { alias: "tst", primary: "tst.w" },
    { alias: "suba", primary: "suba.w" },
    { alias: "subi", primary: "subi.w" },
    { alias: "subq", primary: "subq.w" },
    { alias: "sub", primary: "sub.w" },
    { alias: "subx", primary: "subx.w" },
];
var m68k_opcode_aliases_branch = [
    { alias: "dbra", primary: "dbf" },
    { alias: "jbra", primary: "jra" },
    { alias: "jbhi", primary: "jhi" },
    { alias: "jbls", primary: "jls" },
    { alias: "jbcc", primary: "jcc" },
    { alias: "jbcs", primary: "jcs" },
    { alias: "jbne", primary: "jne" },
    { alias: "jbeq", primary: "jeq" },
    { alias: "jbvc", primary: "jvc" },
    { alias: "jbvs", primary: "jvs" },
    { alias: "jbpl", primary: "jpl" },
    { alias: "jbmi", primary: "jmi" },
    { alias: "jbge", primary: "jge" },
    { alias: "jblt", primary: "jlt" },
    { alias: "jbgt", primary: "jgt" },
    { alias: "jble", primary: "jle" },
];
// obscure stuff
var m68k_opcode_aliases2 = [
    { alias: "bhib", primary: "bhi.s" },
    { alias: "blsb", primary: "bls.s" },
    { alias: "bccb", primary: "bcc.s" },
    { alias: "bcsb", primary: "bcs.s" },
    { alias: "bneb", primary: "bne.s" },
    { alias: "beqb", primary: "beq.s" },
    { alias: "bvcb", primary: "bvc.s" },
    { alias: "bvsb", primary: "bvs.s" },
    { alias: "bplb", primary: "bpl.s" },
    { alias: "bmib", primary: "bmi.s" },
    { alias: "bgeb", primary: "bge.s" },
    { alias: "bltb", primary: "blt.s" },
    { alias: "bgtb", primary: "bgt.s" },
    { alias: "bleb", primary: "ble.s" },
    { alias: "brab", primary: "bra.s" },
    { alias: "bsrb", primary: "bsr.s" },
    { alias: "bhs", primary: "bcc.w" },
    { alias: "bhss", primary: "bcc.s" },
    { alias: "bhsb", primary: "bcc.s" },
    { alias: "bhsw", primary: "bcc.w" },
    { alias: "bhsl", primary: "bcc.l" },
    { alias: "blo", primary: "bcs.w" },
    { alias: "blos", primary: "bcs.s" },
    { alias: "blob", primary: "bcs.s" },
    { alias: "blow", primary: "bcs.w" },
    { alias: "blol", primary: "bcs.l" },
    { alias: "br", primary: "bra.w" },
    { alias: "brs", primary: "bra.s" },
    { alias: "brb", primary: "bra.s" },
    { alias: "brw", primary: "bra.w" },
    { alias: "brl", primary: "bra.l" },
    { alias: "jfnlt", primary: "bcc" },
    { alias: "jfngt", primary: "ble" },
    { alias: "jfeq", primary: "beq.s" },
    { alias: "bchgb", primary: "bchg" },
    { alias: "bchgl", primary: "bchg" },
    { alias: "bclrb", primary: "bclr" },
    { alias: "bclrl", primary: "bclr" },
    { alias: "bsetb", primary: "bset" },
    { alias: "bsetl", primary: "bset" },
    { alias: "btstb", primary: "btst" },
    { alias: "btstl", primary: "btst" },
    { alias: "dbccw", primary: "dbcc" },
    { alias: "dbcsw", primary: "dbcs" },
    { alias: "dbeqw", primary: "dbeq" },
    { alias: "dbfw", primary: "dbf" },
    { alias: "dbgew", primary: "dbge" },
    { alias: "dbgtw", primary: "dbgt" },
    { alias: "dbhiw", primary: "dbhi" },
    { alias: "dblew", primary: "dble" },
    { alias: "dblsw", primary: "dbls" },
    { alias: "dbltw", primary: "dblt" },
    { alias: "dbmiw", primary: "dbmi" },
    { alias: "dbnew", primary: "dbne" },
    { alias: "dbplw", primary: "dbpl" },
    { alias: "dbtw", primary: "dbt" },
    { alias: "dbvcw", primary: "dbvc" },
    { alias: "dbvsw", primary: "dbvs" },
    { alias: "dbhs", primary: "dbcc" },
    { alias: "dbhsw", primary: "dbcc" },
    { alias: "dbraw", primary: "dbf" },
    { alias: "tdivsl", primary: "divsl.l" },
    { alias: "extbw", primary: "ext.w" },
    { alias: "extwl", primary: "ext.l" },
    { alias: "fbneq", primary: "fbne" },
    { alias: "fbsneq", primary: "fbsne" },
    { alias: "fdbneq", primary: "fdbne" },
    { alias: "fdbsneq", primary: "fdbsne" },
    { alias: "fmovecr", primary: "fmovecrx" },
    { alias: "fmovm", primary: "fmovem" },
    { alias: "fsneq", primary: "fsne" },
    { alias: "fssneq", primary: "fssne" },
    { alias: "ftrapneq", primary: "ftrapne" },
    { alias: "ftrapsneq", primary: "ftrapsne" },
    { alias: "fjneq", primary: "fjne" },
    { alias: "fjsneq", primary: "fjsne" },
    { alias: "jmpl", primary: "jmp" },
    { alias: "jmps", primary: "jmp" },
    { alias: "jsrl", primary: "jsr" },
    { alias: "jsrs", primary: "jsr" },
    { alias: "leal", primary: "lea" },
    { alias: "movml", primary: "movem.l" },
    { alias: "movmw", primary: "movem.w" },
    { alias: "movm", primary: "movem.w" },
    { alias: "movpw", primary: "movep.w" },
    { alias: "moves", primary: "moves.w" },
    { alias: "nbcdb", primary: "nbcd" },
    { alias: "peal", primary: "pea" },
    { alias: "sats", primary: "satsl" },
    { alias: "sbcdb", primary: "sbcd" },
    { alias: "sccb", primary: "scc" },
    { alias: "scsb", primary: "scs" },
    { alias: "seqb", primary: "seq" },
    { alias: "sfb", primary: "sf" },
    { alias: "sgeb", primary: "sge" },
    { alias: "sgtb", primary: "sgt" },
    { alias: "shib", primary: "shi" },
    { alias: "sleb", primary: "sle" },
    { alias: "slsb", primary: "sls" },
    { alias: "sltb", primary: "slt" },
    { alias: "smib", primary: "smi" },
    { alias: "sneb", primary: "sne" },
    { alias: "splb", primary: "spl" },
    { alias: "stb", primary: "st" },
    { alias: "svcb", primary: "svc" },
    { alias: "svsb", primary: "svs" },
    { alias: "sfge", primary: "sge" },
    { alias: "sfgt", primary: "sgt" },
    { alias: "sfle", primary: "sle" },
    { alias: "sflt", primary: "slt" },
    { alias: "sfneq", primary: "sne" },
    { alias: "swapw", primary: "swap" },
    { alias: "tasb", primary: "tas" },
    { alias: "tpcc", primary: "trapcc" },
    { alias: "tcc", primary: "trapcc" },
    { alias: "movql", primary: "moveq" },
    { alias: "moveql", primary: "moveq" },
    { alias: "movl", primary: "movel" },
    { alias: "movq", primary: "moveq" },
    { alias: "moval", primary: "movea.l" },
    { alias: "movaw", primary: "movea.w" },
    { alias: "movb", primary: "moveb" },
    { alias: "movc", primary: "movec" },
    { alias: "movecl", primary: "movec" },
    { alias: "movpl", primary: "movep.l" },
    { alias: "movw", primary: "movew" },
    { alias: "movsb", primary: "moves.b" },
    { alias: "movsl", primary: "moves.l" },
    { alias: "movsw", primary: "moves.w" },
    { alias: "mov3q", primary: "mov3q.l" },
    { alias: "tdivul", primary: "divul.l" },
    { alias: "fmovb", primary: "fmoveb" },
    { alias: "fsmovb", primary: "fsmoveb" },
    { alias: "fdmovb", primary: "fdmoveb" },
    { alias: "fmovd", primary: "fmoved" },
    { alias: "fsmovd", primary: "fsmoved" },
    { alias: "fmovl", primary: "fmovel" },
    { alias: "fsmovl", primary: "fsmovel" },
    { alias: "fdmovl", primary: "fdmovel" },
    { alias: "fmovp", primary: "fmovep" },
    { alias: "fsmovp", primary: "fsmovep" },
    { alias: "fdmovp", primary: "fdmovep" },
    { alias: "fmovs", primary: "fmoves" },
    { alias: "fsmovs", primary: "fsmoves" },
    { alias: "fdmovs", primary: "fdmoves" },
    { alias: "fmovw", primary: "fmovew" },
    { alias: "fsmovw", primary: "fsmovew" },
    { alias: "fdmovw", primary: "fdmovew" },
    { alias: "fmovx", primary: "fmovex" },
    { alias: "fsmovx", primary: "fsmovex" },
    { alias: "fdmovx", primary: "fdmovex" },
    { alias: "fmovcr", primary: "fmovecr" },
    { alias: "fmovcrx", primary: "fmovecrx" },
    { alias: "ftestb", primary: "ftstb" },
    { alias: "ftestd", primary: "ftstd" },
    { alias: "ftestl", primary: "ftstl" },
    { alias: "ftestp", primary: "ftstp" },
    { alias: "ftests", primary: "ftsts" },
    { alias: "ftestw", primary: "ftstw" },
    { alias: "ftestx", primary: "ftstx" },
    { alias: "bitrevl", primary: "bitrev" },
    { alias: "byterevl", primary: "byterev" },
    { alias: "ff1l", primary: "ff1" },
];
// ported from binutils-gdb/opcodes/m68k-dis.c, Copyright (C) 1989-2021 Free Software Foundation, Inc. GPLv3
var fpcr_names = [
    "", "fpiar", "fpsr", "fpiar/fpsr", "fpcr",
    "fpiar/fpcr", "fpsr/fpcr", "fpiar/fpsr/fpcr"
];
var reg_names = [
    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "sp",
    "ps", "pc"
];
var reg_half_names = [
    "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "ps", "pc"
];
var reg_names_J = {
    0x000: "sfc",
    0x001: "dfc",
    0x002: "cacr",
    0x003: "tc",
    0x004: "itt0",
    0x005: "itt1",
    0x006: "dtt0",
    0x007: "dtt1",
    0x008: "buscr",
    0x009: "rgpiobar",
    0x00c: "acr4",
    0x00d: "acr5",
    0x00e: "acr6",
    0x00f: "acr7",
    0x800: "usp",
    0x801: "vbr",
    0x802: "caar",
    0x803: "msp",
    0x804: "isp",
    0x80f: "pc",
    0xc04: "rambar0",
    0xc05: "rambar1",
    0xc0e: "mbar0",
    0xc0f: "mbar1",
    0x805: "mmusr",
    0x806: "urp",
    0x807: "srp",
    0x808: "pcr",
    // Fido added these.
    0xffe: "cac",
    0xfff: "mbo"
};
function fetch_arg(buffer, code, bits) {
    var val = 0;
    switch (code) {
        case '/': /* MAC/EMAC mask bit.  */
            val = buffer[3] >> 5;
            break;
        case 'G': /* EMAC ACC load.  */
            val = ((buffer[3] >> 3) & 0x2) | ((~buffer[1] >> 7) & 0x1);
            break;
        case 'H': /* EMAC ACC !load.  */
            val = ((buffer[3] >> 3) & 0x2) | ((buffer[1] >> 7) & 0x1);
            break;
        case ']': /* EMAC ACCEXT bit.  */
            val = buffer[0] >> 2;
            break;
        case 'I': /* MAC/EMAC scale factor.  */
            val = buffer[2] >> 1;
            break;
        case 'F': /* EMAC ACCx.  */
            val = buffer[0] >> 1;
            break;
        case 'f':
            val = buffer[1];
            break;
        case 's':
            val = buffer[1];
            break;
        case 'd': /* Destination, for register or quick.  */
            val = (buffer[0] << 8) + buffer[1];
            val >>= 9;
            break;
        case 'x': /* Destination, for general arg.  */
            val = (buffer[0] << 8) + buffer[1];
            val >>= 6;
            break;
        case 'k':
            val = (buffer[3] >> 4);
            break;
        case 'C':
            val = buffer[3];
            break;
        case '1':
            val = (buffer[2] << 8) + buffer[3];
            val >>= 12;
            break;
        case '2':
            val = (buffer[2] << 8) + buffer[3];
            val >>= 6;
            break;
        case '3':
        case 'j':
            val = (buffer[2] << 8) + buffer[3];
            break;
        case '4':
            val = (buffer[4] << 8) + buffer[5];
            val >>= 12;
            break;
        case '5':
            val = (buffer[4] << 8) + buffer[5];
            val >>= 6;
            break;
        case '6':
            val = (buffer[4] << 8) + buffer[5];
            break;
        case '7':
            val = (buffer[2] << 8) + buffer[3];
            val >>= 7;
            break;
        case '8':
            val = (buffer[2] << 8) + buffer[3];
            val >>= 10;
            break;
        case '9':
            val = (buffer[2] << 8) + buffer[3];
            val >>= 5;
            break;
        case 'e':
            val = (buffer[1] >> 6);
            break;
        case 'E':
            val = (buffer[2] >> 1);
            break;
        case 'm':
            val = (buffer[1] & 0x40 ? 0x8 : 0)
                | ((buffer[0] >> 1) & 0x7)
                | (buffer[3] & 0x80 ? 0x10 : 0);
            break;
        case 'n':
            val = (buffer[1] & 0x40 ? 0x8 : 0) | ((buffer[0] >> 1) & 0x7);
            break;
        case 'o':
            val = (buffer[2] >> 4) | (buffer[3] & 0x80 ? 0x10 : 0);
            break;
        case 'M':
            val = (buffer[1] & 0xf) | (buffer[3] & 0x40 ? 0x10 : 0);
            break;
        case 'N':
            val = (buffer[3] & 0xf) | (buffer[3] & 0x40 ? 0x10 : 0);
            break;
        case 'h':
            val = buffer[2] >> 2;
            break;
        default:
        //throw new Error("<internal error>"); // TODO: bei divu.l dürfen wir hier nicht hinkommen, wieso geht das in der C version?
    }
    /* bits is never too big.  */
    return val & ((1 << bits) - 1);
}
function m68k_valid_ea(code, val) {
    var mask = 0;
    var M = function (n0, n1, n2, n3, n4, n5, n6, n70, n71, n72, n73, n74) {
        return (n0 | n1 << 1 | n2 << 2 | n3 << 3 | n4 << 4 | n5 << 5 | n6 << 6 | n70 << 7 | n71 << 8 | n72 << 9 | n73 << 10 | n74 << 11);
    };
    switch (code) {
        case '*':
            mask = M(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
            break;
        case '~':
            mask = M(0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0);
            break;
        case '%':
            mask = M(1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0);
            break;
        case ';':
            mask = M(1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1);
            break;
        case '@':
            mask = M(1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0);
            break;
        case '!':
            mask = M(0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0);
            break;
        case '&':
            mask = M(0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0);
            break;
        case '$':
            mask = M(1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0);
            break;
        case '?':
            mask = M(1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0);
            break;
        case '/':
            mask = M(1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0);
            break;
        case '|':
            mask = M(0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0);
            break;
        case '>':
            mask = M(0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0);
            break;
        case '<':
            mask = M(0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0);
            break;
        case 'm':
            mask = M(1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0);
            break;
        case 'n':
            mask = M(0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0);
            break;
        case 'o':
            mask = M(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1);
            break;
        case 'p':
            mask = M(1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 'q':
            mask = M(1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 'v':
            mask = M(1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0);
            break;
        case 'b':
            mask = M(1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0);
            break;
        case 'w':
            mask = M(0, 0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0);
            break;
        case 'y':
            mask = M(0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0);
            break;
        case 'z':
            mask = M(0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0);
            break;
        case '4':
            mask = M(0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0);
            break;
        default:
            throw new Error("abort");
    }
    var mode = (val >> 3) & 7;
    if (mode === 7)
        mode += val & 7;
    return (mask & (1 << mode)) !== 0;
}
var print_address = function (adr) { return "$" + adr.toString(16); };
var print_vma = function (b) { return b.toString(); }; // needs to be signed;
function print_base(regno, disp) {
    if (regno === -1)
        return print_address(disp) + ",pc";
    if (regno === -2)
        return disp.toString();
    if (regno === -3)
        return print_vma(disp) + ",zpc";
    else
        return print_vma(disp) + "," + reg_names[regno];
}
// COERCE: make signed
var COERCE8 = function (x) { return ((x ^ 0x80) & 0xff) - 128; };
var COERCE16 = function (x) { return (x ^ 0x8000) - 0x8000; };
var COERCE32 = function (x) { return (x >>> 0); };
var NEXTBYTE = function (buffer, p) { p += 2; return [p, COERCE8(buffer[p - 1])]; };
var NEXTWORD = function (buffer, p) { p += 2; return [p, COERCE16((buffer[p - 2] << 8) + buffer[p - 1])]; };
var NEXTLONG = function (buffer, p) { p += 4; return [p, COERCE32((((buffer[p - 4] << 8) + buffer[p - 3] << 8) + buffer[p - 2] << 8) + buffer[p - 1])]; };
var NEXTULONG = function (buffer, p) { p += 4; return [p, (((buffer[p - 4] << 8) + buffer[p - 3] << 8) + buffer[p - 2] << 8) + buffer[p - 1]]; };
function print_indexed(basereg, buffer, p, addr) {
    var _a, _b, _c, _d, _e;
    var scales = ["", "*2", "*4", "*8"];
    var text = '';
    var word;
    var base_disp, outer_disp;
    _a = NEXTWORD(buffer, p), p = _a[0], word = _a[1];
    var buf = "".concat(reg_names[(word >> 12) & 0xf], ".").concat((word & 0x800) ? 'l' : 'w').concat(scales[(word >> 9) & 3]);
    /* Handle the 68000 style of indexing.  */
    if ((word & 0x100) === 0) {
        base_disp = word & 0xff;
        if ((base_disp & 0x80) !== 0)
            base_disp -= 0x100;
        if (basereg === -1)
            base_disp += addr;
        return ['(' + print_base(basereg, base_disp) + ',' + buf + ')', p];
    }
    /* Handle the generalized kind.  */
    /* First, compute the displacement to add to the base register.  */
    if (word & 128) {
        if (basereg === -1)
            basereg = -3;
        else
            basereg = -2;
    }
    if (word & 64)
        buf = '';
    base_disp = 0;
    switch ((word >> 4) & 3) {
        case 2:
            _b = NEXTWORD(buffer, p), p = _b[0], base_disp = _b[1];
            break;
        case 3:
            _c = NEXTLONG(buffer, p), p = _c[0], base_disp = _c[1];
            break;
    }
    if (basereg === -1)
        base_disp += addr;
    /* Handle single-level case (not indirect).  */
    if ((word & 7) === 0)
        return ['(' + print_base(basereg, base_disp) + ((buf !== '') ? (',' + buf) : '') + ')', p];
    /* Two level.  Compute displacement to add after indirection.  */
    outer_disp = 0;
    switch (word & 3) {
        case 2:
            _d = NEXTWORD(buffer, p), p = _d[0], outer_disp = _d[1];
            break;
        case 3:
            _e = NEXTLONG(buffer, p), p = _e[0], outer_disp = _e[1];
            break;
    }
    text = "([" + print_base(basereg, base_disp);
    if ((word & 4) === 0 && buf !== '') {
        text += "," + buf;
        buf = '';
    }
    text += "]";
    if (buf !== '')
        text += "," + buf;
    text += "," + print_vma(outer_disp) + ")";
    return [text, p];
}
function print_insn_arg(d, buffer, p0, addr) {
    var _a, _b, _c, _d, _e, _f, _g, _h, _j, _k, _l, _m, _o, _p, _q, _r, _s, _t, _u, _v, _w, _x, _y;
    var val = 0;
    var disp = 0;
    var regno = 0;
    var text = '';
    var place = d[1];
    var o = 0;
    var p = p0;
    var FETCH_ARG = function (bits) { return fetch_arg(buffer, place, bits); };
    switch (d[0]) {
        case 'c': // Cache identifier.
            var cacheFieldName = ["nc", "dc", "ic", "bc"];
            val = FETCH_ARG(2);
            text = cacheFieldName[val];
            break;
        case 'a': // Address register indirect only. Cf. case '+'.
            val = FETCH_ARG(3);
            text = "(".concat(reg_names[val + 8], ")");
            break;
        case '_': // 32-bit absolute address for move16.
            _a = NEXTULONG(buffer, p), p = _a[0], val = _a[1];
            text = print_address(val);
            break;
        case 'C':
            text = "ccr";
            break;
        case 'S':
            text = "sr";
            break;
        case 'U':
            text = "usp";
            break;
        case 'E':
            text = "acc";
            break;
        case 'G':
            text = "macsr";
            break;
        case 'H':
            text = "mask";
            break;
        case 'J':
            val = FETCH_ARG(12);
            if (reg_names_J[val])
                text = reg_names_J[val];
            else
                text = "0x".concat(val.toString(16));
            break;
        case 'Q':
            val = FETCH_ARG(3);
            /* 0 means 8, except for the bkpt instruction... */
            if (val === 0 && d[1] !== 's')
                val = 8;
            text = "#".concat(val);
            break;
        case 'x':
            val = FETCH_ARG(3);
            /* 0 means -1 */
            if (val === 0)
                val = -1;
            text = "#".concat(val);
            break;
        case 'j':
            val = FETCH_ARG(3);
            text = "#".concat(val + 1);
            break;
        case 'K':
            val = FETCH_ARG(9);
            text = "#".concat(val);
            break;
        case 'M':
            if (place === 'h') {
                text = "<not supported: Mh>";
            }
            else {
                val = FETCH_ARG(8);
                if (val & 0x80)
                    val -= 0x100;
                text = "#".concat(val);
            }
            break;
        case 'T':
            val = FETCH_ARG(4);
            text = "#".concat(val);
            break;
        case 'D':
            val = FETCH_ARG(3);
            text += reg_names[val];
            break;
        case 'A':
            val = FETCH_ARG(3);
            text += reg_names[val + 8];
            break;
        case 'R':
            val = FETCH_ARG(4);
            text += reg_names[val];
            break;
        case 'r':
            regno = FETCH_ARG(4);
            text = "(".concat(reg_names[regno], ")");
            break;
        case 'F':
            val = FETCH_ARG(3);
            text += "fp".concat(val);
            break;
        case 'O':
            val = FETCH_ARG(6);
            if (val & 0x20)
                text = reg_names[val & 7];
            else
                text = val.toString();
            break;
        case '+':
            val = FETCH_ARG(3);
            text = "(".concat(reg_names[val + 8], ")+");
            break;
        case '-':
            val = FETCH_ARG(3);
            text = "-(".concat(reg_names[val + 8], ")");
            break;
        case 'k':
            if (place === 'k') {
                val = FETCH_ARG(3);
                text = "{".concat(reg_names[val], "}");
            }
            else if (place === 'C') {
                val = FETCH_ARG(7);
                if (val > 63) /* This is a signed constant.  */
                    val -= 128;
                text = "{#".concat(val, "}");
            }
            else
                return { len: -1 };
            break;
        case '#':
        case '^':
            var p1 = d[0] === '#' ? 2 : 4;
            if (place === 's')
                val = FETCH_ARG(4);
            else if (place === 'C')
                val = FETCH_ARG(7);
            else if (place === '8')
                val = FETCH_ARG(3);
            else if (place === '3')
                val = FETCH_ARG(8);
            else if (place === 'b')
                _b = NEXTBYTE(buffer, p1), p1 = _b[0], val = _b[1];
            else if (place === 'w' || place === 'W')
                _c = NEXTWORD(buffer, p1), p1 = _c[0], val = _c[1];
            else if (place === 'l')
                _d = NEXTLONG(buffer, p1), p1 = _d[0], val = _d[1];
            else
                throw new Error("<invalid op_table>");
            text = "#".concat(val);
            break;
        case 'B':
            if (place === 'b')
                _e = NEXTBYTE(buffer, p), p = _e[0], disp = _e[1];
            else if (place === 'B')
                disp = COERCE8(buffer[1]);
            else if (place === 'w' || place === 'W')
                _f = NEXTWORD(buffer, p), p = _f[0], disp = _f[1];
            else if (place === 'l' || place === 'L' || place === 'C')
                _g = NEXTLONG(buffer, p), p = _g[0], disp = _g[1];
            else if (place === 'g') {
                _h = NEXTBYTE(buffer, o), o = _h[0], disp = _h[1];
                if (disp === 0)
                    _j = NEXTWORD(buffer, p), p = _j[0], disp = _j[1];
                else if (disp === -1)
                    _k = NEXTLONG(buffer, p), p = _k[0], disp = _k[1];
            }
            else if (place === 'c') {
                if (buffer[1] & 0x40) // If bit six is one, long offset.
                    _l = NEXTLONG(buffer, p), p = _l[0], disp = _l[1];
                else
                    _m = NEXTWORD(buffer, p), p = _m[0], disp = _m[1];
            }
            else
                throw new Error("<invalid op_table>");
            text = print_address(addr + disp);
            break;
        case 'd':
            _o = NEXTWORD(buffer, p), p = _o[0], val = _o[1];
            var val1 = FETCH_ARG(3);
            text = "".concat(val, "(").concat(reg_names[val1 + 8], ")");
            break;
        case 's':
            val = FETCH_ARG(3);
            text = fpcr_names[val];
            break;
        case 'I':
            // Get coprocessor ID...
            val = fetch_arg(buffer, 'd', 3);
            if (val < 0)
                text = "<PRINT_INSN_ARG_MEMORY_ERROR>";
            if (val !== 1) // Unusual coprocessor ID?
                text = "(cpid=".concat(val, ") ");
            break;
        case '4':
        case '*':
        case '~':
        case '%':
        case ';':
        case '@':
        case '!':
        case '$':
        case '?':
        case '/':
        case '&':
        case '|':
        case '<':
        case '>':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'v':
        case 'b':
        case 'w':
        case 'y':
        case 'z':
            if (place === 'd') {
                val = fetch_arg(buffer, 'x', 6);
                val = ((val & 7) << 3) + ((val >> 3) & 7);
            }
            else {
                val = fetch_arg(buffer, 's', 6);
            }
            /* If the <ea> is invalid for *d, then reject this match.  */
            if (!m68k_valid_ea(d[0], val))
                return { len: -1 };
            /* Get register number assuming address register.  */
            regno = (val & 7) + 8;
            var regname = reg_names[regno];
            switch (val >> 3) {
                case 0:
                    text = reg_names[val];
                    break;
                case 1:
                    text = regname;
                    break;
                case 2:
                    text = "(".concat(reg_names[regno], ")");
                    break;
                case 3:
                    text = "(".concat(reg_names[regno], ")+");
                    break;
                case 4:
                    text = "-(".concat(reg_names[regno], ")");
                    break;
                case 5:
                    _p = NEXTWORD(buffer, p), p = _p[0], val = _p[1];
                    text = "".concat(val, "(").concat(regname, ")");
                    break;
                case 6:
                    _q = print_indexed(regno, buffer, p, addr), text = _q[0], p = _q[1];
                    break;
                case 7:
                    switch (val & 7) {
                        case 0:
                            _r = NEXTWORD(buffer, p), p = _r[0], val = _r[1];
                            text = print_address(val);
                            break;
                        case 1:
                            _s = NEXTULONG(buffer, p), p = _s[0], val = _s[1];
                            text = print_address(val);
                            break;
                        case 2:
                            _t = NEXTWORD(buffer, p), p = _t[0], val = _t[1];
                            text = print_address(addr + val) + "(pc)";
                            break;
                        case 3:
                            _u = print_indexed(-1, buffer, p, addr), text = _u[0], p = _u[1];
                            break;
                        case 4:
                            switch (place) {
                                case 'b':
                                    _v = NEXTBYTE(buffer, p), p = _v[0], val = _v[1];
                                    break;
                                case 'w':
                                    _w = NEXTWORD(buffer, p), p = _w[0], val = _w[1];
                                    break;
                                case 'l':
                                    _x = NEXTLONG(buffer, p), p = _x[0], val = _x[1];
                                    break;
                                default:
                                    console.log("float not supported");
                                    return { len: 0 };
                            }
                            text = "#".concat(val);
                            break;
                    }
            }
            break;
        case 'L':
        case 'l':
            if (place === 'w') {
                var doneany = false;
                var p1_1 = 2;
                _y = NEXTWORD(buffer, p1_1), p1_1 = _y[0], val = _y[1];
                if (p1_1 > p)
                    p = p1_1;
                if (val === 0) {
                    text = '#0';
                    break;
                }
                if (d[0] === 'l') {
                    var newval = 0;
                    for (regno = 0; regno < 16; ++regno)
                        if (val & (0x8000 >> regno))
                            newval |= 1 << regno;
                    val = newval;
                }
                val &= 0xffff;
                doneany = false;
                for (regno = 0; regno < 16; ++regno)
                    if (val & (1 << regno)) {
                        if (doneany)
                            text += "/";
                        doneany = true;
                        text += reg_names[regno];
                        var first_regno = regno;
                        while (val & (1 << (regno + 1)))
                            ++regno;
                        if (regno > first_regno)
                            text += "-".concat(reg_names[regno]);
                    }
            }
            else if (place === '3') {
                console.log("float not supported");
            }
            else if (place === '8') {
                val = FETCH_ARG(3);
                text = fpcr_names[val];
            }
            else
                throw new Error("<invalid op_table>");
            break;
        case 'X':
            place = '8';
        // fall through
        case 'Y':
        case 'Z':
        case 'W':
        case '0':
        case '1':
        case '2':
        case '3':
            val = FETCH_ARG(5);
            switch (val) {
                case 2:
                    text = "tt0";
                    break;
                case 3:
                    text = "tt1";
                    break;
                case 0x10:
                    text = "tc";
                    break;
                case 0x11:
                    text = "drp";
                    break;
                case 0x12:
                    text = "srp";
                    break;
                case 0x13:
                    text = "crp";
                    break;
                case 0x14:
                    text = "cal";
                    break;
                case 0x15:
                    text = "val";
                    break;
                case 0x16:
                    text = "scc";
                    break;
                case 0x17:
                    text = "ac";
                    break;
                case 0x18:
                    text = "psr";
                    break;
                case 0x19:
                    text = "pcsr";
                    break;
                case 0x1c:
                case 0x1d:
                    var break_reg = ((buffer[3] >> 2) & 7);
                    text = (val === 0x1c ? "bad" : "bac") + break_reg.toString();
                    break;
                default:
                    text = "<mmu register ".concat(val, ">");
                    break;
            }
        case 'f':
            var fc = FETCH_ARG(5);
            if (fc === 1)
                text = "dfc";
            else if (fc === 0)
                text = "sfc";
            else
                text = "<function code ".concat(fc, ">");
            break;
        case 'V':
            text = "val";
            break;
        case 't':
            var level = FETCH_ARG(3);
            text = level.toString();
            break;
        default:
            throw new Error("<invalid op_table ".concat(d[0], ">"));
    }
    return { text: text, len: p - p0 };
}
function match_insn_m68k(buffer, memaddr, best) {
    var text = '';
    var d = 0;
    if (best.args[d] === '.')
        d++;
    var p = 2;
    for (; d < best.args.length; d += 2) {
        if (best.args[d] === '#') {
            if (best.args[d + 1] === 'l' && p < 6)
                p = 6;
            else if (p < 4 && best.args[d + 1] !== 'C' && best.args[d + 1] !== '8')
                p = 4;
        }
        if ((best.args[d] === 'L' || best.args[d] === 'l') && best.args[d + 1] === 'w' && p < 4)
            p = 4;
        switch (best.args[d + 1]) {
            case '1':
            case '2':
            case '3':
            case '7':
            case '8':
            case '9':
            case 'i':
                if (p < 4)
                    p = 4;
                break;
            case '4':
            case '5':
            case '6':
                if (p < 6)
                    p = 6;
                break;
            default:
                break;
        }
    }
    /* pflusha is an exceptions.  It takes no arguments but is two words
       long.  Recognize it by looking at the lower 16 bits of the mask.  */
    if (p < 4 && (best.match & 0xffff) !== 0)
        p = 4;
    /* lpstop is another exception.  It takes a one word argument but is
       three words long.  */
    if (p < 6
        && (best.match & 0xffff) === 0xffff
        && best.args[0] === '#'
        && best.args[1] === 'w') {
        /* Copy the one word argument into the usual location for a one
       word argument, to simplify printing it.  We can get away with
       this because we know exactly what the second word is, and we
       aren't going to print anything based on it.  */
        p = 6;
        buffer[2] = buffer[4];
        buffer[3] = buffer[5];
    }
    d = 0;
    text += best.name;
    if (best.args.length > 0)
        text += ' ';
    while (d < best.args.length) {
        var arg_val = print_insn_arg(best.args.slice(d, d + 2), buffer, p, memaddr + p);
        if (arg_val.len === -1) // invalid argument, reject match
            return { text: '', len: 0 };
        p += arg_val.len;
        text += arg_val.text;
        d += 2;
        if (d < best.args.length && best.args[d - 2] !== 'I' && best.args[d] !== 'k')
            text += ",";
    }
    return { text: text, len: p };
}
function m68k_scan_mask(buffer, memaddr, arch_mask) {
    for (var _i = 0, m68k_opcodes_1 = m68k_opcodes; _i < m68k_opcodes_1.length; _i++) {
        var opc = m68k_opcodes_1[_i];
        if (((0xff & buffer[0] & (opc.match >> 24)) === (0xff & (opc.opcode >> 24)))
            && ((0xff & buffer[1] & (opc.match >> 16)) === (0xff & (opc.opcode >> 16)))
            && (((0xffff & opc.match) === 0)
                || (((0xff & buffer[2] & (opc.match >> 8)) === (0xff & (opc.opcode >> 8)))
                    && ((0xff & buffer[3] & opc.match) === (0xff & opc.opcode))))) {
                        // console.log('match:', opc);
                        if ((opc.arch & arch_mask) !== 0) {
                // TODO: args for divul, divsl
                console.log('match:', opc);
                var val = match_insn_m68k(buffer, memaddr, opc);
                if (val.len)
                    return val;
            }
        }
    }
    return { text: '', len: 0 };
}
function print_insn_m68k(buffer, memaddr) {
    var ret = m68k_scan_mask(buffer, memaddr, m68k_mask);
    if (ret.len === 0) {
        return { text: ".short 0x".concat(buffer[0].toString(16).padStart(2, '0')).concat(buffer[1].toString(16).padStart(2, '0')), len: 2 };
    }
    return ret;
}
exports.print_insn_m68k = print_insn_m68k;
function get_all_insn_m68k() {
    var set = new Set();
    for (var _i = 0, m68k_opcodes_2 = m68k_opcodes; _i < m68k_opcodes_2.length; _i++) {
        var opc = m68k_opcodes_2[_i];
        set.add(opc.name);
    }
    for (var _a = 0, m68k_opcode_aliases_nosize_1 = m68k_opcode_aliases_nosize; _a < m68k_opcode_aliases_nosize_1.length; _a++) {
        var alias = m68k_opcode_aliases_nosize_1[_a];
        set.add(alias.alias);
    }
    for (var _b = 0, m68k_opcode_aliases_branch_1 = m68k_opcode_aliases_branch; _b < m68k_opcode_aliases_branch_1.length; _b++) {
        var alias = m68k_opcode_aliases_branch_1[_b];
        set.add(alias.alias);
    }
    var ret = [];
    for (var _c = 0, _d = set.values(); _c < _d.length; _c++) {
        var v = _d[_c];
        ret.push(v);
    }
    return ret;
}
exports.get_all_insn_m68k = get_all_insn_m68k;

var args = process.argv.slice(2);
var mem = new Uint8Array(32);
var i,j;

for (i = 0, j = 0; i < 16; i++) {

    if (i < args.length) {
        
        var word = parseInt(args[i], 16);
        mem[j++] = (word >> 8) & 0xFF;
        mem[j++] = word & 0xFF;
    
    } else {

        mem[j++] = 0;
        mem[j++] = 0;
    }
}

var dasm = print_insn_m68k(mem, 0);
var hex = ""

for (i = 0; i < 16; i++) {
    hex += ("00"+mem[i].toString(16)).slice(-2) + " "; 
}
console.log(hex);
hex = "";

for (i = 0; i < dasm.len; i++) {

    if (i % 2 == 0) hex += "$";
    hex += ("00"+mem[i].toString(16)).slice(-2);
    if (i % 2 == 1) hex += " ";
}
console.log(hex + ": " + dasm.text);
