
#include "tester.h"
#include "results.h"

void Tester_68k::sampleLea() {
    Results* oObj;

    oObj = new Results("lea x.w, A");
    oObj->setRegA(0, 0xc);
    oObj->setCycleCount(8);

    oObj = new Results("lea x.l, A");
    oObj->setRegA(0, 0xc000d);
    oObj->setCycleCount(12);

    oObj = new Results("lea (An), A");
    oObj->setRegA(1, 0xc000d);
    oObj->setRegA(2, 0xc000d);
    oObj->setCycleCount(4);

    oObj = new Results("lea (d,An), A");
    oObj->setRegA(1, 0x000c000c);
    oObj->setRegA(2, 0xc000d);
    oObj->setCycleCount(8);

    oObj = new Results("lea (d,An,Xn.W), A");
    oObj->setRegA(1, 0x000c0031);
    oObj->setRegA(2, 0xc000d);
    oObj->setRegD(7, 0x10000022);
    oObj->setCycleCount(12);

    oObj = new Results("lea (d,An,Xn.L), A");
    oObj->setRegA(1, 0x100c0031);
    oObj->setRegA(2, 0xc000d);
    oObj->setRegD(7, 0x10000022);
    oObj->setCycleCount(12);

    oObj = new Results("lea (d,PC), A");
    oObj->setRegA(1, 0x08);
    oObj->setCycleCount(8);

    oObj = new Results("lea (d,PC,Xn.L), A");
    oObj->setRegA(1, 0xa);
    oObj->setRegD(3, 0x2);
    oObj->setCycleCount(12);
}

void Tester_68k::testLea() {
    u16 opcode;
    //LEA   12.w, A0
    setUp();
    opcode = 0x41c0 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0xc);
    addWord(0xffff);
    power();
    process();
    check("lea x.w, A");

    //LEA   786445.l, A0
    setUp();
    opcode = 0x41c0 | getEA(ABS_LONG);
    addWord(opcode);
    addWord(0xc);
    addWord(0xd);
    addWord(0xffff);
    power();
    process();
    check("lea x.l, A");

    //MOVE.L	#$c000d, A2
    //LEA       (A2), A1
    setUp();
    opcode = 0x41c0 | (1 << 9) | getEA(AR_INDIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0xc000d);
    process();
    check("lea (An), A");

    //MOVE.L	#$c000d, A2
    //LEA       (-1,A2), A1
    setUp();
    opcode = 0x41c0 | (1 << 9) | getEA(AR_INDIRECT_D16, 2);
    addWord(opcode);
    addWord(0xffff);
    addWord(0xffff);
    power();
    setRegA(2, 0xc000d);
    process();
    check("lea (d,An), A");

    //MOVE.L	#$c000d, A2
    //MOVE.L	#$10000022, D7
    //LEA	(2,A2,D7.W), A1
    setUp();
    opcode = 0x41c0 | (1 << 9) | getEA(AR_INDIRECT_D8, 2);
    addWord(opcode);
    addWord(0x7002);
    addWord(0xffff);
    power();
    setRegA(2, 0xc000d);
    setRegD(7, 0x10000022);
    process();
    check("lea (d,An,Xn.W), A");

    //MOVE.L	#$c000d, A2
    //MOVE.L	#$10000022, D7
    //LEA	(2,A2,D7.L), A1
    setUp();
    opcode = 0x41c0 | (1 << 9) | getEA(AR_INDIRECT_D8, 2);
    addWord(opcode);
    addWord(0x7802);
    addWord(0xffff);
    power();
    setRegA(2, 0xc000d);
    setRegD(7, 0x10000022);
    process();
    check("lea (d,An,Xn.L), A");

    //LEA	(-6,PC), A1
    setUp();
    addWord(0x44fc);
    addWord(0x0);
    opcode = 0x41c0 | (1 << 9) | getEA(PC_INDIRECT_D16);
    addWord(opcode);
    addWord(0xfffa);
    addWord(0xffff);
    memoryblock.write(7, 12);
    power();
    process();
    check("lea (d,PC), A");

    //MOVE.L  #$2, D3
    //LEA	(-6,PC,D3), A1
    setUp();
    opcode = 0x41c0 | (1 << 9) | getEA(PC_INDIRECT_D8);
    addWord(0x44fc);
    addWord(0x0);
    addWord(opcode);
    addWord(0x30FA);
    addWord(0xffff);
    memoryblock.write(7, 12);
    power();
    setRegD(3, 0x2);
    process();
    check("lea (d,PC,Xn.L), A");
}
