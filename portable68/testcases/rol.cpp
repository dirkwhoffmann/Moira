
#include "tester.h"
#include "results.h"

void Tester_68k::sampleRol() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ROL.B #8, D0");
    oObj->setRegD(0, 0xce3dd567);
    oObj->setC();
    oObj->setCycleCount(22);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.B #1, D0");
    oObj->setRegD(0, 0xce3dd5ce);
    oObj->setN();
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.B #2, D0");
    oObj->setRegD(0, 0xce3dd59d);
    oObj->setN()->setX()->setC();
    oObj->setCycleCount(10);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.B #7, D0");
    oObj->setRegD(0, 0xce3dd5b3);
    oObj->setN()->setX()->setC();
    oObj->setCycleCount(20);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.W #8, D0");
    oObj->setRegD(0, 0xce3d67d5);
    oObj->setX()->setC();
    oObj->setCycleCount(22);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.L #3, D0");
    oObj->setRegD(0, 0x71eeab3e);
    oObj->setX();
    oObj->setCycleCount(14);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.W D1, D0 count 20");
    oObj->setRegD(0, 0xce3d567d);
    oObj->setRegD(1, 20);
    oObj->setX()->setC();
    oObj->setCycleCount(46);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.W D1, D0 count 36");
    oObj->setRegD(0, 0xce3d567d);
    oObj->setRegD(1, 36);
    oObj->setX()->setC();
    oObj->setCycleCount(78);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.W D1, D0 count 0");
    oObj->setRegD(0, 0xce3dd567);
    oObj->setRegD(1, 0);
    oObj->setN()->setX();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.L D1, D0 count 200");
    oObj->setRegD(0, 0x3dd567ce);
    oObj->setRegD(1, 200);
    oObj->setX();
    oObj->setCycleCount(24);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROL.W ($3000).W");
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3000, 0xaacf);
    oObj->setCodeBlock(oSampleMem);
    oObj->setN()->setC();
    oObj->setCycleCount(16);

    oObj = new Results("ROL.W ($3000).W alt");
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3000, 0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setZ();
    oObj->setCycleCount(16);
}

void Tester_68k::testRol() {
    u16 opcode;

    //MOVE.L    #$ce3dd567, D0
    //MOVE      #0, CCR
    //ROL.B     #8, D0
    setUp();
    opcode = 0xe018 | (1 << 8);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0xce3dd567);
    process();
    check("ROL.B #8, D0");

    //MOVE.L    #$ce3dd567, D0
    //MOVE      #0, CCR
    //ROL.B     #1, D0
    setUp();
    opcode = 0xe018 | (1 << 8) | (1 << 9);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0xce3dd567);
    process();
    check("ROL.B #1, D0");

    //MOVE.L    #$ce3dd567, D0
    //MOVE      #$1f, CCR
    //ROL.B     #2, D0
    setUp();
    opcode = 0xe018 | (1 << 8) | (2 << 9);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xce3dd567);
    process();
    check("ROL.B #2, D0");

    //MOVE.L    #$ce3dd567, D0
    //MOVE      #$1f, CCR
    //ROL.B     #7, D0
    setUp();
    opcode = 0xe018 | (1 << 8) | (7 << 9);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xce3dd567);
    process();
    check("ROL.B #7, D0");

    //MOVE.L    #$ce3dd567, D0
    //MOVE      #$1f, CCR
    //ROL.W     #8, D0
    setUp();
    opcode = 0xe018 | (1 << 8) | (0 << 9) | (1 << 6);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xce3dd567);
    process();
    check("ROL.W #8, D0");

    //MOVE.L    #$ce3dd567, D0
    //MOVE      #$1f, CCR
    //ROL.L     #3, D0
    setUp();
    opcode = 0xe018 | (1 << 8) | (3 << 9) | (2 << 6);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xce3dd567);
    process();
    check("ROL.L #3, D0");

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #20, D1
    //MOVE      #$1f, CCR
    //ROL.W     D1, D0
    setUp();
    opcode = 0xe018 | (1 << 8) | (1 << 9) | (1 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xce3dd567);
    setRegD(1, 20);
    process();
    check("ROL.W D1, D0 count 20");

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #36, D1
    //MOVE      #$1f, CCR
    //ROL.W     D1, D0
    setUp();
    opcode = 0xe018 | (1 << 8) | (1 << 9) | (1 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xce3dd567);
    setRegD(1, 36);
    process();
    check("ROL.W D1, D0 count 36");

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #0, D1
    //MOVE      #$1f, CCR
    //ROL.W     D1, D0
    setUp();
    opcode = 0xe018 | (1 << 8) | (1 << 9) | (1 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xce3dd567);
    setRegD(1, 0);
    process();
    check("ROL.W D1, D0 count 0");

    //MOVE.L    #$ce3dd567, D0
    //MOVE.L    #200, D1
    //MOVE      #$1f, CCR
    //ROL.L     D1, D0
    setUp();
    opcode = 0xe018 | (1 << 8) | (1 << 9) | (2 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xce3dd567);
    setRegD(1, 200);
    process();
    check("ROL.L D1, D0 count 200");

    //MOVE.W    #$d567, $3000
    //MOVE #$0, CCR
    //ROL.W ($3000).W
    //MOVE.W $3000, D2
    setUp();
    opcode = 0xe7c0 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0);
    memoryblock.writeWord(0x3000, 0xd567);
    process();
    check("ROL.W ($3000).W");

    //MOVE.W    #$0, $3000
    //MOVE #$0, CCR
    //ROL.W ($3000).W
    //MOVE.W $3000, D2
    setUp();
    opcode = 0xe7c0 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0);
    memoryblock.writeWord(0x3000, 0x0);
    process();
    check("ROL.W ($3000).W alt");
}
