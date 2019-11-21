
#include "tester.h"
#include "results.h"

void Tester_68k::sampleRor() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ROR.B #8, D0");
    oObj->setRegD(0, 0xce3dd599);
    oObj->setC()->setN();
    oObj->setCycleCount(22);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROR.B #1, D0");
    oObj->setRegD(0, 0xce3dd5cc);
    oObj->setC()->setN();
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROR.B #4, D0");
    oObj->setRegD(0, 0xce3dd599);
    oObj->setC()->setN();
    oObj->setCycleCount(14);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROR.B #7, D0");
    oObj->setRegD(0, 0xce3dd533);
    oObj->setCycleCount(20);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROR.W #6, D0");
    oObj->setRegD(0, 0xce3d6756);
    oObj->setCycleCount(18);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROR.L #5, D0");
    oObj->setRegD(0, 0xce71eeac);
    oObj->setC()->setN();
    oObj->setCycleCount(18);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROR.B D1, D0 count 20");
    oObj->setRegD(0, 0xce3dd599);
    oObj->setRegD(1, 20);
    oObj->setN()->setC();
    oObj->setCycleCount(46);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROR.L D1, D0 count 26");
    oObj->setRegD(0, 0x8f756673);
    oObj->setRegD(1, 26);
    oObj->setN()->setC();
    oObj->setCycleCount(60);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ROR.W ($3000).W");
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3000, 0xeab3);
    oObj->setCodeBlock(oSampleMem);
    oObj->setN()->setC();
    oObj->setCycleCount(16);

    oObj = new Results("ROR.W ($3000).W alt");
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3000, 0x6ab0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);
}

void Tester_68k::testRor() {
    u16 opcode;

    //MOVE.L    #$ce3dd599, D0
    //MOVE      #0, CCR
    //ROR.B     #8, D0
    setUp();
    opcode = 0xe018;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0xce3dd599);
    process();
    check("ROR.B #8, D0");

    //MOVE.L    #$ce3dd599, D0
    //MOVE      #0, CCR
    //ROR.B     #1, D0
    setUp();
    opcode = 0xe018 | (1 << 9);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0xce3dd599);
    process();
    check("ROR.B #1, D0");

    //MOVE.L    #$ce3dd599, D0
    //MOVE      #0, CCR
    //ROR.B     #4, D0
    setUp();
    opcode = 0xe018 | (4 << 9);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0xce3dd599);
    process();
    check("ROR.B #4, D0");

    //MOVE.L    #$ce3dd599, D0
    //MOVE      #0, CCR
    //ROR.B     #7, D0
    setUp();
    opcode = 0xe018 | (7 << 9);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0xce3dd599);
    process();
    check("ROR.B #7, D0");

    //MOVE.L    #$ce3dd599, D0
    //MOVE      #0, CCR
    //ROR.W     #6, D0
    setUp();
    opcode = 0xe018 | (6 << 9) | (1 << 6);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0xce3dd599);
    process();
    check("ROR.W #6, D0");

    //MOVE.L    #$ce3dd599, D0
    //MOVE      #0, CCR
    //ROR.L     #5, D0
    setUp();
    opcode = 0xe018 | (5 << 9) | (2 << 6);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0xce3dd599);
    process();
    check("ROR.L #5, D0");

    //MOVE.L    #$ce3dd599, D0
    //MOVE.L    #20, D1
    //MOVE      #$0, CCR
    //ROR.B     D1, D0
    setUp();
    opcode = 0xe018 | (1 << 9) | (0 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(0, 0xce3dd599);
    setRegD(1, 20);
    process();
    check("ROR.B D1, D0 count 20");

    //MOVE.L    #$ce3dd599, D0
    //MOVE.L    #26, D1
    //MOVE      #$0, CCR
    //ROR.L     D1, D0
    setUp();
    opcode = 0xe018 | (1 << 9) | (2 << 6) | (1 << 5);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(0, 0xce3dd599);
    setRegD(1, 26);
    process();
    check("ROR.L D1, D0 count 26");

    //MOVE.W    #$d567, $3000
    //MOVE #$0, CCR
    //ROR.W ($3000).W
    //MOVE.W $3000, D2
    setUp();
    opcode = 0xe6c0 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0);
    memoryblock.writeWord(0x3000, 0xd567);
    process();
    check("ROR.W ($3000).W");

    //MOVE.W    #$d560, $3000
    //MOVE #$0, CCR
    //ROR.W ($3000).W
    //MOVE.W $3000, D2
    setUp();
    opcode = 0xe6c0 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0);
    memoryblock.writeWord(0x3000, 0xd560);
    process();
    check("ROR.W ($3000).W alt");
}
