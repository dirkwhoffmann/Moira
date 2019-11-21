
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleNegx() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("NEGX.B D0");
    oObj->setRegD(0, 0x12345687);
    oObj->setC()->setX()->setN();
    oObj->setCycleCount(4);

    oObj = new Results("NEGX.B D0 alt");
    oObj->setRegD(0, 0x123456ff);
    oObj->setC()->setX()->setN();
    oObj->setCycleCount(4);

    oObj = new Results("NEGX.B D0 no overflow");
    oObj->setRegD(0, 0x1234567f);
    oObj->setC()->setX();
    oObj->setCycleCount(4);

    oObj = new Results("NEGX.W D0 no overflow");
    oObj->setRegD(0, 0x12347fff);
    oObj->setC()->setX();
    oObj->setCycleCount(4);

    oObj = new Results("NEGX.L D0");
    oObj->setRegD(0, 0xedcba987);
    oObj->setC()->setX()->setN();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("NEGX.L D0 alt");
    oObj->setRegD(0, 0x0);
    oObj->setC()->setX();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("NEGX.L ($3000).L");
    oObj->setC()->setX();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0ffedcee);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(28);
}

void Tester_68k::testNegx() {
    u16 opcode;

    //MOVE.L    #$12345678, D0
    //MOVE      #$10, CCR
    //NEGX.B    D0
    setUp();
    opcode = 0x4000 | (0 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0x12345678);
    process();
    check("NEGX.B D0");

    //MOVE.L    #$12345600, D0
    //MOVE      #$10, CCR
    //NEGX.B    D0
    setUp();
    opcode = 0x4000 | (0 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0x12345600);
    process();
    check("NEGX.B D0 alt");

    //MOVE.L    #$12345680, D0
    //MOVE      #$10, CCR
    //NEGX.B    D0
    setUp();
    opcode = 0x4000 | (0 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0x12345680);
    process();
    check("NEGX.B D0 no overflow");

    //MOVE.L    #$12348000, D0
    //MOVE      #$10, CCR
    //NEGX.W    D0
    setUp();
    opcode = 0x4000 | (1 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0x12348000);
    process();
    check("NEGX.W D0 no overflow");

    //MOVE.L    #$12345678, D0
    //MOVE      #$10, CCR
    //NEGX.L    D0
    setUp();
    opcode = 0x4000 | (2 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0x12345678);
    process();
    check("NEGX.L D0");

    //MOVE.L    #$ffffffff, D0
    //MOVE      #$10, CCR
    //NEGX.L    D0
    setUp();
    opcode = 0x4000 | (2 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0xffffffff);
    process();
    check("NEGX.L D0 alt");

    //MOVE.L    #$f0012311, $3000
    //MOVE      #$10, CCR
    //NEGX.L    ($3000).L
    //MOVE.L    $3000, D0
    setUp();
    opcode = 0x4000 | (2 << 6) | getEA(ABS_LONG);
    addWord(opcode);
    addWord(0);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0x10);
    memoryblock.writeLong(0x3000, 0xf0012311);
    process();
    check("NEGX.L ($3000).L");
}
