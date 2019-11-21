
#include "tester.h"
#include "results.h"

void Tester_68k::sampleExt() {
    Results* oObj;

    oObj = new Results("ext.w D3");
    oObj->setRegD(3, 0x12340078);
    oObj->setX();
    oObj->setCycleCount(4);

    oObj = new Results("ext.w D3 zero");
    oObj->setRegD(3, 0x12340000);
    oObj->setX()->setZ();
    oObj->setCycleCount(4);

    oObj = new Results("ext.w D3 negative");
    oObj->setRegD(3, 0x1234fff0);
    oObj->setX()->setN();
    oObj->setCycleCount(4);

    oObj = new Results("ext.l D3");
    oObj->setRegD(3, 0xfffff6f0);
    oObj->setX()->setN();
    oObj->setCycleCount(4);
}

void Tester_68k::testExt() {
    u16 opcode;

    //MOVE.L #$12345678, D3
    //MOVE #$13, CCR
    //EXT.W D3
    setUp();
    opcode = 0x4800 | 2 << 6 | 3;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x12345678);
    setCCR(0x13);
    process();
    check("ext.w D3");

    //MOVE.L #$12345600, D3
    //MOVE #$13, CCR
    //EXT.W D3
    setUp();
    opcode = 0x4800 | 2 << 6 | 3;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x12345600);
    setCCR(0x13);
    process();
    check("ext.w D3 zero");

    //MOVE.L #$123456f0, D3
    //MOVE #$13, CCR
    //EXT.W D3
    setUp();
    opcode = 0x4800 | 2 << 6 | 3;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x123456f0);
    setCCR(0x13);
    process();
    check("ext.w D3 negative");

    //MOVE.L #$1234f6f0, D3
    //MOVE #$13, CCR
    //EXT.L D3
    setUp();
    opcode = 0x4800 | 3 << 6 | 3;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x1234f6f0);
    setCCR(0x13);
    process();
    check("ext.l D3");
}
