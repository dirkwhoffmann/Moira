
#include "tester.h"
#include "results.h"

void Tester_68k::sampleLsr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("LSR.B D2, D1");
    oObj->setRegD(1, 0xce3dd519);
    oObj->setRegD(2, 2);
    oObj->setX()->setC();
    oObj->setCycleCount(10);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSR.B D2, D1 alt");
    oObj->setRegD(1, 0xce3dd500);
    oObj->setRegD(2, 0x69);
    oObj->setZ();
    oObj->setCycleCount(88);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSR.W D2, D1");
    oObj->setRegD(1, 0xce3dd567);
    oObj->setRegD(2, 0);
    oObj->setN();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSR.W D2, D1 alt");
    oObj->setRegD(1, 0xce3d001a);
    oObj->setRegD(2, 0xb);
    oObj->setX()->setC();
    oObj->setCycleCount(28);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSR.L D2, D1");
    oObj->setRegD(1, 0);
    oObj->setRegD(2, 0x20);
    oObj->setX()->setZ()->setC();
    oObj->setCycleCount(72);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSR.L #n, D1");
    oObj->setRegD(1, 0xce3dd5);
    oObj->setCycleCount(24);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSR (x).w");
    oSampleMem = new MemoryBlock;
    oSampleMem->writeWord(0x3000, 0x4666);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);

}


void Tester_68k::testLsr() {
    u16 opcode;

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #2, D2
    //MOVE      #0, CCR
    //LSR.B     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (0 << 8) | (0 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 2);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSR.B D2, D1");

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #$69, D2
    //MOVE      #0, CCR
    //LSR.B     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (0 << 8) | (0 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x69);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSR.B D2, D1 alt");

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #0, D2
    //MOVE      #0, CCR
    //LSR.W     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (0 << 8) | (1 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSR.W D2, D1");

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #$b, D2
    //MOVE      #0, CCR
    //LSR.W     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (0 << 8) | (1 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0xb);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSR.W D2, D1 alt");

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #$20, D2
    //MOVE      #0, CCR
    //LSR.L     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (0 << 8) | (2 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x20);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSR.L D2, D1");

    //MOVE.L    #$CE3DD567, D1
    //MOVE      #0, CCR
    //LSR.L     #8, D1
    setUp();
    opcode = 0xe008 | (0 << 9) | (0 << 8) | (2 << 6) | (0 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0xCE3DD567);
    process();
    check("LSR.L #n, D1");

    //MOVE.W    #$8ccc, $3000
    //MOVE      #0, CCR
    //LSR       ($3000).W
    //MOVE.W    $3000, D3
    setUp();
    opcode = 0xe2c0 | (0 << 8) | (7 << 3);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x3000, 0x8ccc);
    process();
    check("LSR (x).w");

}
