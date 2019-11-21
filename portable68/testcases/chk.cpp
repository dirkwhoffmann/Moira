
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleChk() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("CHK D1, D2");
    oObj->setX();
    oObj->setRegD(1, 0x1111);
    oObj->setRegD(2, 0x1111);
    oObj->setCycleCount(10);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("CHK D1, D2 zero");
    oObj->setX()->setZ();
    oObj->setRegD(1, 0x1111);
    oObj->setRegD(2, 0);
    oObj->setCycleCount(10);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("CHK D1, D2 trap");
    oObj->expectGroup2Exception(true);
    oObj->setX();
    oObj->setRegD(1, 0x8000);
    oObj->setRegD(2, 0x8001);
    oObj->setRegA(7, 0xfffffffa);
    oObj->setRegIRD(0);
    oObj->setCycleCount(42);

    oObj = new Results("CHK D1, D2 trap alt");
    oObj->expectGroup2Exception(true);
    oObj->setX()->setN();
    oObj->setRegD(1, 0x8000);
    oObj->setRegD(2, 0x8000);
    oObj->setRegA(7, 0xfffffffa);
    oObj->setRegIRD(0);
    oObj->setCycleCount(44);
}

void Tester_68k::testChk() {
    u16 opcode;

    //MOVE      #$1f, CCR
    //MOVE.L    #$1111, D1
    //MOVE.L    #$1111, D2
    //CHK       D1, D2
    setUp();
    opcode = 0x4000 | (2 << 9) | (3 << 7) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x1111);
    setRegD(2, 0x1111);
    process();
    check("CHK D1, D2");

    //MOVE      #$1f, CCR
    //MOVE.L    #$1111, D1
    //MOVE.L    #$0000, D2
    //CHK       D1, D2
    setUp();
    opcode = 0x4000 | (2 << 9) | (3 << 7) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x1111);
    setRegD(2, 0);
    process();
    check("CHK D1, D2 zero");

    //MOVE      #$1f, CCR
    //MOVE.L    #$8000, D1
    //MOVE.L    #$8001, D2
    //CHK       D1, D2
    setUp();
    opcode = 0x4000 | (2 << 9) | (3 << 7) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x8000);
    setRegD(2, 0x8001);
    process();
    check("CHK D1, D2 trap");

    //MOVE      #$1f, CCR
    //MOVE.L    #$8000, D1
    //MOVE.L    #$8000, D2
    //CHK       D1, D2
    setUp();
    opcode = 0x4000 | (2 << 9) | (3 << 7) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x8000);
    setRegD(2, 0x8000);
    process();
    check("CHK D1, D2 trap alt");
}
