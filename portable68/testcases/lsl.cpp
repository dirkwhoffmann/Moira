
#include "tester.h"
#include "results.h"

void Tester_68k::sampleLsl() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("LSL.B D2, D1");
    oObj->setRegD(1, 0xce3dd59c);
    oObj->setRegD(2, 2);
    oObj->setX()->setN()->setC();
    oObj->setCycleCount(10);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSL.B D2, D1 alt");
    oObj->setRegD(1, 0xce3dd500);
    oObj->setRegD(2, 0x69);
    oObj->setZ();
    oObj->setCycleCount(88);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSL.W D2, D1");
    oObj->setRegD(1, 0xce3dd567);
    oObj->setRegD(2, 0);
    oObj->setN();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSL.W D2, D1 alt");
    oObj->setRegD(1, 0xce3d3800);
    oObj->setRegD(2, 0xb);
    oObj->setX()->setC();
    oObj->setCycleCount(28);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSL.L D2, D1");
    oObj->setRegD(1, 0);
    oObj->setRegD(2, 0x20);
    oObj->setX()->setZ()->setC();
    oObj->setCycleCount(72);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSL.L #n, D1");
    oObj->setRegD(1, 0x3dd56700);
    oObj->setCycleCount(24);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("LSL (x).w");
    oSampleMem = new MemoryBlock;
    oSampleMem->writeWord(0x3000, 0x1998);
    oObj->setCodeBlock(oSampleMem);
    oObj->setX()->setC();
    oObj->setCycleCount(16);

}


void Tester_68k::testLsl() {
    u16 opcode;

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #2, D2
    //MOVE      #0, CCR
    //LSL.B     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (1 << 8) | (0 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 2);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSL.B D2, D1");

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #$69, D2
    //MOVE      #0, CCR
    //LSL.B     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (1 << 8) | (0 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x69);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSL.B D2, D1 alt");

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #0, D2
    //MOVE      #0, CCR
    //LSL.W     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (1 << 8) | (1 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSL.W D2, D1");

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #$b, D2
    //MOVE      #0, CCR
    //LSL.W     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (1 << 8) | (1 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0xb);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSL.W D2, D1 alt");

    //MOVE.L    #$CE3DD567, D1
    //MOVE.L    #$20, D2
    //MOVE      #0, CCR
    //LSL.L     D2, D1
    setUp();
    opcode = 0xe008 | (2 << 9) | (1 << 8) | (2 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x20);
    setRegD(1, 0xCE3DD567);
    process();
    check("LSL.L D2, D1");

    //MOVE.L    #$CE3DD567, D1
    //MOVE      #0, CCR
    //LSL.L     #8, D1
    setUp();
    opcode = 0xe008 | (0 << 9) | (1 << 8) | (2 << 6) | (0 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0xCE3DD567);
    process();
    check("LSL.L #n, D1");

    //MOVE.W    #$8ccc, $3000
    //MOVE      #0, CCR
    //LSL       ($3000).W
    //MOVE.W    $3000, D3
    setUp();
    opcode = 0xe2c0 | (1 << 8) | (7 << 3);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x3000, 0x8ccc);
    process();
    check("LSL (x).w");

}
