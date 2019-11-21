
#include "tester.h"
#include "results.h"

void Tester_68k::sampleMulu() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("MULU.W D1, D2");
    oObj->setRegD(1, 0x12345678);
    oObj->setRegD(2, 0x1d34d840);
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(54);

    oObj = new Results("MULU.W D1, D2 zero");
    oObj->setRegD(1, 0x1);
    oObj->setRegD(2, 0x0);
    oObj->setX()->setZ();
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(40);

    oObj = new Results("MULU.W #$ffff, D2 neg");
    oObj->setRegD(2, 0xfffe0001);
    oObj->setX()->setN();
    oObj->setIrqSampleCyclePos(6);
    oObj->setCycleCount(74);
}

void Tester_68k::testMulu() {
    u16 opcode;

    //MOVE.L    #$12345678, D1
    //MOVE.L    #$12345678, D2
    //MOVE      #$0, CCR
    //MULU.W    D1, D2
    setUp();
    opcode = 0xc0c0 | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0x12345678);
    setRegD(2, 0x12345678);
    setCCR(0);
    process();
    check("MULU.W D1, D2");

    //MOVE.L    #$1, D1
    //MOVE.L    #$0, D2
    //MOVE      #$13, CCR
    //MULU.W    D1, D2
    setUp();
    opcode = 0xc0c0 | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0x1);
    setRegD(2, 0x0);
    setCCR(0x13);
    process();
    check("MULU.W D1, D2 zero");

    //MOVE.L    #$ffff, D2
    //MOVE      #$13, CCR
    //MULU.W    #$ffff, D2
    setUp();
    opcode = 0xc0c0 | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xffff);
    addWord(0xffff);
    power();
    setRegD(2, 0xffff);
    setCCR(0x13);
    process();
    check("MULU.W #$ffff, D2 neg");
}

