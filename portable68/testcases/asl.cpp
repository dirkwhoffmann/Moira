
#include "tester.h"
#include "results.h"

void Tester_68k::sampleAsl() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ASL.B D2, D1");
    oObj->setRegD(1, 0xce3dd59c);
    oObj->setRegD(2, 2);
    oObj->setX()->setN()->setV()->setC();
    oObj->setCycleCount(10);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ASL.B D2, D1 alt");
    oObj->setRegD(1, 0xce3dd500);
    oObj->setRegD(2, 105);
    oObj->setV()->setZ();
    oObj->setCycleCount(88);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ASL.W D2, D1");
    oObj->setRegD(1, 0xce3dd567);
    oObj->setRegD(2, 0);
    oObj->setN();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ASL.W D2, D1 alt");
    oObj->setRegD(1, 0xce3d3800);
    oObj->setRegD(2, 0xb);
    oObj->setX()->setV()->setC();
    oObj->setCycleCount(28);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ASL.L D2, D1");
    oObj->setRegD(1, 0);
    oObj->setRegD(2, 32);
    oObj->setX()->setZ()->setV()->setC();
    oObj->setCycleCount(72);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ASL.L #n, D1");
    oObj->setRegD(1, 0x3dd56700);
    oObj->setV();
    oObj->setCycleCount(24);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ASL (x).w");
    oSampleMem = new MemoryBlock;
    oSampleMem->writeWord(0x3000, 0x1998);
    oObj->setCodeBlock(oSampleMem);
    oObj->setX()->setV()->setC();
    oObj->setCycleCount(16);

    oObj = new Results("ASL (x).w alt");
    oSampleMem = new MemoryBlock;
    oSampleMem->writeLong(0x3000, 0x8af00782);
    oObj->setCodeBlock(oSampleMem);
    oObj->setN()->setV();
    oObj->setCycleCount(16);
}


void Tester_68k::testAsl() {
    u16 opcode;

    //MOVE.L    #3460158823, D1
    //MOVE.L    #2, D2
    //MOVE      #0, CCR
    //ASL.B     D2, D1
    setUp();
    opcode = 0xe000 | (2 << 9) | (1 << 8) | (0 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 2);
    setRegD(1, 3460158823);
    process();
    check("ASL.B D2, D1");

    //MOVE.L    #3460158823, D1
    //MOVE.L    #105, D2
    //MOVE      #0, CCR
    //ASL.B     D2, D1
    setUp();
    opcode = 0xe000 | (2 << 9) | (1 << 8) | (0 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 105);
    setRegD(1, 3460158823);
    process();
    check("ASL.B D2, D1 alt");

    //MOVE.L    #3460158823, D1
    //MOVE.L    #0, D2
    //MOVE      #0, CCR
    //ASL.W     D2, D1
    setUp();
    opcode = 0xe000 | (2 << 9) | (1 << 8) | (1 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0);
    setRegD(1, 3460158823);
    process();
    check("ASL.W D2, D1");

    //MOVE.L    #3460158823, D1
    //MOVE.L    #11, D2
    //MOVE      #0, CCR
    //ASL.W     D2, D1
    setUp();
    opcode = 0xe000 | (2 << 9) | (1 << 8) | (1 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 11);
    setRegD(1, 3460158823);
    process();
    check("ASL.W D2, D1 alt");

    //MOVE.L    #3460158823, D1
    //MOVE.L    #32, D2
    //MOVE      #0, CCR
    //ASL.L     D2, D1
    setUp();
    opcode = 0xe000 | (2 << 9) | (1 << 8) | (2 << 6) | (1 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 32);
    setRegD(1, 3460158823);
    process();
    check("ASL.L D2, D1");

    //MOVE.L    #3460158823, D1
    //MOVE      #0, CCR
    //ASL.L     #8, D1
    setUp();
    opcode = 0xe000 | (0 << 9) | (1 << 8) | (2 << 6) | (0 << 5) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 3460158823);
    process();
    check("ASL.L #n, D1");

    //MOVE.W    #36044, $3000
    //MOVE      #0, CCR
    //ASL       ($3000).W
    //MOVE.W    $3000, D3
    setUp();
    opcode = 0xe0c0 | (1 << 8) | (7 << 3);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x3000, 36044);
    process();
    check("ASL (x).w");

    //MOVE.L    #$45780782, $3000
    //MOVE      #0, CCR
    //ASL       ($3000).W
    //MOVE.L    $3000, D3
    setUp();
    opcode = 0xe0c0 | (1 << 8) | (7 << 3);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.writeLong(0x3000, 0x45780782);
    process();
    check("ASL (x).w alt");
}
