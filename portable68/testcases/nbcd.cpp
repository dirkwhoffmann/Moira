
#include "tester.h"
#include "results.h"

void Tester_68k::sampleNbcd() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("NBCD D1");
    oObj->setX()->setC()->setV();
    oObj->setRegD(1, 0x20);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("NBCD D1 with x");
    oObj->setX()->setC()->setV();
    oObj->setRegD(1, 0x1234561f);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("NBCD D1 zero");
    oObj->setZ();
    oObj->setRegD(1, 0x12345600);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("NBCD D1 neg");
    oObj->setX()->setN()->setC();
    oObj->setRegD(1, 0x1234569a);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("NBCD ($3000).W");
    oObj->setX()->setN()->setC();
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x99);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);
}

void Tester_68k::testNbcd() {
    u16 opcode;

    //MOVE.B    #$7a, D1
    //MOVE #$0, CCR
    //NBCD D1
    setUp();
    opcode = 0x4800  | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(1, 0x7a);
    process();
    check("NBCD D1");

    //MOVE.L    #$1234567a, D1
    //MOVE #$14, CCR
    //NBCD D1
    setUp();
    opcode = 0x4800  | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0x1234567a);
    process();
    check("NBCD D1 with x");

    //MOVE.L    #$12345600, D1
    //MOVE #$4, CCR
    //NBCD D1
    setUp();
    opcode = 0x4800  | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x12345600);
    process();
    check("NBCD D1 zero");

    //MOVE.L    #$123456ff, D1
    //MOVE #$14, CCR
    //NBCD D1
    setUp();
    opcode = 0x4800  | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x14);
    setRegD(1, 0x123456ff);
    process();
    check("NBCD D1 neg");

    //MOVE.B    #$01, $3000
    //MOVE #$0, CCR
    //NBCD ($3000).W
    //MOVE $3000, D2
    setUp();
    opcode = 0x4800  | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0x0);
    memoryblock.write(0x3000, 0x01);
    process();
    check("NBCD ($3000).W");
}
