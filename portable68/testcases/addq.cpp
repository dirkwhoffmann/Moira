
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleAddq() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ADDQ.L #$7, D1");
    oObj->setRegD(1, 0x6);
    oObj->setX()->setC();
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ADDQ.W #$3, D1");
    oObj->setRegD(1, 0xffff0001);
    oObj->setX()->setC();
    oObj->setCycleCount(4);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ADDQ.W #$3, A1");
    oObj->setRegA(1, 0x1);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("ADDQ.W #$8, ($3000).W");
    oObj->setN()->setV();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3000, 0x8005);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);
}

void Tester_68k::testAddq() {
    u16 opcode;

    //MOVE.L    #$ffffffff, D1
    //MOVE      #$0, CCR
    //ADDQ.L    #$7, D1
    setUp();
    opcode = 0x5000 | (7 << 9) | (2 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0xffffffff);
    process();
    check("ADDQ.L #$7, D1");

    //MOVE.L    #$fffffffe, D1
    //MOVE      #$0, CCR
    //ADDQ.W    #$3, D1
    setUp();
    opcode = 0x5000 | (3 << 9) | (1 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0xfffffffe);
    process();
    check("ADDQ.W #$3, D1");

    //MOVE.L    #$fffffffe, A1
    //MOVE      #$0, CCR
    //ADDQ.W    #$3, A1
    setUp();
    opcode = 0x5000 | (3 << 9) | (1 << 6) | getEA(AR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(1, 0xfffffffe);
    process();
    check("ADDQ.W #$3, A1");

    //MOVE.W    #$7ffd, $3000
    //MOVE      #$0, CCR
    //ADDQ.W    #$8, ($3000).W
    //MOVE.W    $3000, D1
    setUp();
    opcode = 0x5000 | (1 << 6) | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x3000, 0x7ffd);
    process();
    check("ADDQ.W #$8, ($3000).W");
}
