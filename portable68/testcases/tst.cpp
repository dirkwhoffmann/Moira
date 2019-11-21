
#include "tester.h"
#include "results.h"

void Tester_68k::sampleTst() {
    Results* oObj;

    oObj = new Results("tst.w D4");
    oObj->setRegD(4, 0xfff1);
    oObj->setN()->setX();
    oObj->setCycleCount(4);

    oObj = new Results("tst.l D4");
    oObj->setRegD(4, 0);
    oObj->setZ()->setX();
    oObj->setCycleCount(4);

    oObj = new Results("tst.w A0");
    oObj->expectIllegal(true);
    oObj->setRegIRD(0);
    oObj->setRegA(7, 0xfffffffa);
    oObj->setCycleCount(34);
}

void Tester_68k::testTst() {
    u16 opcode;

    //MOVE.L #$fff1, D4
    //MOVE 0x13, CCR
    //TST.W D4
    setUp();
    opcode = 0x4a00 | (1 << 6) | getEA(DR_DIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(4, 0xfff1);
    setCCR(0x13);
    process();
    check("tst.w D4");

    //MOVE.L #$0, D4
    //MOVE 0x13, CCR
    //TST.L D4
    setUp();
    opcode = 0x4a00 | (2 << 6) | getEA(DR_DIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(4, 0);
    setCCR(0x13);
    process();
    check("tst.l D4");

    //TST.W A0
    setUp();
    opcode = 0x4a00 | (1 << 6) | getEA(AR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    process();
    group1exceptions();
    check("tst.w A0");
}
