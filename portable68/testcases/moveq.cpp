
#include "tester.h"
#include "results.h"

void Tester_68k::sampleMoveq() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("MOVEQ #01, D1");
    oObj->setRegD(1, 0x01);
    oObj->setCycleCount(4);

    oObj = new Results("MOVEQ #-1, D1");
    oObj->setRegD(1, 0xffffffff);
    oObj->setX()->setN();
    oObj->setCycleCount(4);

    oObj = new Results("MOVEQ #0, D1");
    oObj->setRegD(1, 0);
    oObj->setX()->setZ();
    oObj->setCycleCount(4);

    oObj = new Results("MOVEQ #80, D1");
    oObj->setRegD(1, 0xffffff80);
    oObj->setX()->setN();
    oObj->setCycleCount(4);
}

void Tester_68k::testMoveq() {
    u16 opcode;

    //MOVE.L    #$ffffffff, D1
    //MOVE      #$0, CCR
    //MOVEQ     #$01, D1
    setUp();
    opcode = 0x7000 | (1 << 9) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(1, 0xffffffff);
    process();
    check("MOVEQ #01, D1");

    //MOVE.L    #$12345678, D1
    //MOVE      #$13, CCR
    //MOVEQ     #-1, D1
    setUp();
    opcode = 0x7000 | (1 << 9) | 0xff;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x13);
    setRegD(1, 0x12345678);
    process();
    check("MOVEQ #-1, D1");

    //MOVE.L    #$12345678, D1
    //MOVE      #$13, CCR
    //MOVEQ     #0, D1
    setUp();
    opcode = 0x7000 | (1 << 9) | 0x0;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x13);
    setRegD(1, 0x12345678);
    process();
    check("MOVEQ #0, D1");

    //MOVE.L    #$12345678, D1
    //MOVE      #$13, CCR
    //MOVEQ     #-128, D1
    setUp();
    opcode = 0x7000 | (1 << 9) | 0x80;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x13);
    setRegD(1, 0x12345678);
    process();
    check("MOVEQ #80, D1");

}
