
#include "tester.h"
#include "results.h"

void Tester_68k::sampleRoxl() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ROXL.B D1, D0 count 9");
    oObj->setRegD(0, 0xce3dd567);
    oObj->setRegD(1, 9);
    oObj->setC()->setX();
    oObj->setCycleCount(24);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROXL.B D1, D0 count 9 alt");
    oObj->setRegD(0, 0xce3dd567);
    oObj->setRegD(1, 9);
    oObj->setCycleCount(24);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROXL.W D1, D0 count 17");
    oObj->setRegD(0, 0xce3dd567);
    oObj->setRegD(1, 17);
    oObj->setN();
    oObj->setCycleCount(40);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROXL.L D1, D0 count 33");
    oObj->setRegD(0, 0xce3dd567);
    oObj->setRegD(1, 33);
    oObj->setN()->setC()->setX();
    oObj->setCycleCount(74);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROXL.W D1, D0 count 5");
    oObj->setRegD(0, 0xce3dacfd);
    oObj->setRegD(1, 5);
    oObj->setN();
    oObj->setCycleCount(16);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROXL.W D1, D0 count 22");
    oObj->setRegD(0, 0xce3dacfd);
    oObj->setRegD(1, 22);
    oObj->setN();
    oObj->setCycleCount(50);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROXL.W #4, D0");
    oObj->setRegD(0, 0xce3d6009);
    oObj->setX()->setC();
    oObj->setCycleCount(14);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROXL.W ($3000).W");
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3000, 0xaace);
    oObj->setCodeBlock(oSampleMem);
    oObj->setN()->setC()->setX();
    oObj->setCycleCount(16);
}

void Tester_68k::testRoxl() {
    u16 opcode;

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #9, D1
    //MOVE      #$1f, CCR
    //ROXL.B    D1, D0
    setUp();
    opcode = 0xe010 | (1 << 8) | (1 << 9) | (0 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xce3dd567);
    setRegD(1, 9);
    process();
    check("ROXL.B D1, D0 count 9");

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #9, D1
    //MOVE      #$0, CCR
    //ROXL.B    D1, D0
    setUp();
    opcode = 0xe010 | (1 << 8) | (1 << 9) | (0 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(0, 0xce3dd567);
    setRegD(1, 9);
    process();
    check("ROXL.B D1, D0 count 9 alt");

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #17, D1
    //MOVE      #$1, CCR
    //ROXL.W    D1, D0
    setUp();
    opcode = 0xe010 | (1 << 8) | (1 << 9) | (1 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(1);
    setRegD(0, 0xce3dd567);
    setRegD(1, 17);
    process();
    check("ROXL.W D1, D0 count 17");

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #33, D1
    //MOVE      #$10, CCR
    //ROXL.L    D1, D0
    setUp();
    opcode = 0xe010 | (1 << 8) | (1 << 9) | (2 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0xce3dd567);
    setRegD(1, 33);
    process();
    check("ROXL.L D1, D0 count 33");

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #5, D1
    //MOVE      #$10, CCR
    //ROXL.W    D1, D0
    setUp();
    opcode = 0xe010 | (1 << 8) | (1 << 9) | (1 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0xce3dd567);
    setRegD(1, 5);
    process();
    check("ROXL.W D1, D0 count 5");

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #22, D1
    //MOVE      #$10, CCR
    //ROXL.W    D1, D0
    setUp();
    opcode = 0xe010 | (1 << 8) | (1 << 9) | (1 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0xce3dd567);
    setRegD(1, 22);
    process();
    check("ROXL.W D1, D0 count 22");

    //MOVE.L    #$ce3d3600, D0
    //MOVE      #$10, CCR
    //ROXL.W    #4, D0
    setUp();
    opcode = 0xe010 | (1 << 8) | (4 << 9) | (1 << 6) | (0 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0xce3d3600);
    process();
    check("ROXL.W #4, D0");

    //MOVE.W    #$d567, $3000
    //MOVE #0, CCR
    //ROXL.W ($3000).W
    //MOVE.W $3000, D2
    setUp();
    opcode = 0xe5c0 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0);
    memoryblock.writeWord(0x3000, 0xd567);
    process();
    check("ROXL.W ($3000).W");
}
