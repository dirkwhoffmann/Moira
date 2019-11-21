
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleNeg() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("NEG.B D0");
    oObj->setRegD(0, 0x12345688);
    oObj->setC()->setX()->setN();
    oObj->setCycleCount(4);

    oObj = new Results("NEG.B D0 alt");
    oObj->setRegD(0, 0x12345600);
    oObj->setZ();
    oObj->setCycleCount(4);

    oObj = new Results("NEG.B D0 overflow");
    oObj->setRegD(0, 0x12345680);
    oObj->setC()->setX()->setN()->setV();
    oObj->setCycleCount(4);

    oObj = new Results("NEG.W D0 overflow");
    oObj->setRegD(0, 0x12348000);
    oObj->setC()->setX()->setN()->setV();
    oObj->setCycleCount(4);

    oObj = new Results("NEG.L D0");
    oObj->setRegD(0, 0xedcba988);
    oObj->setC()->setX()->setN();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("NEG.L D0 alt");
    oObj->setRegD(0, 0x1);
    oObj->setC()->setX();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("NEG.L ($3000).L");
    oObj->setC()->setX();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0ffedcef);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(28);
}

void Tester_68k::testNeg() {
    u16 opcode;

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //NEG.B     D0
    setUp();
    opcode = 0x4400 | (0 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("NEG.B D0");

    //MOVE.L    #$12345600, D0
    //MOVE      #0, CCR
    //NEG.B     D0
    setUp();
    opcode = 0x4400 | (0 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345600);
    process();
    check("NEG.B D0 alt");

    //MOVE.L    #$12345680, D0
    //MOVE      #0, CCR
    //NEG.B     D0
    setUp();
    opcode = 0x4400 | (0 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345680);
    process();
    check("NEG.B D0 overflow");

    //MOVE.L    #$12348000, D0
    //MOVE      #0, CCR
    //NEG.W     D0
    setUp();
    opcode = 0x4400 | (1 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12348000);
    process();
    check("NEG.W D0 overflow");

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //NEG.L     D0
    setUp();
    opcode = 0x4400 | (2 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("NEG.L D0");

    //MOVE.L    #$ffffffff, D0
    //MOVE      #0, CCR
    //NEG.L     D0
    setUp();
    opcode = 0x4400 | (2 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0xffffffff);
    process();
    check("NEG.L D0 alt");

    //MOVE.L    #$f0012311, $3000
    //MOVE      #0, CCR
    //NEG.L     ($3000).L
    //MOVE.L    $3000, D0
    setUp();
    opcode = 0x4400 | (2 << 6) | getEA(ABS_LONG);
    addWord(opcode);
    addWord(0);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.writeLong(0x3000, 0xf0012311);
    process();
    check("NEG.L ($3000).L");
}
