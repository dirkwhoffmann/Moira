
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleSubq() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("SUBQ.L #$7, D1");
    oObj->setRegD(1, 0xfffffff8);
    oObj->setN();
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("SUBQ.W #$7, A1");
    oObj->setRegA(1, 0xfffefffa);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("SUBQ.B #$7, ($3000).W");
    oObj->setX()->setN()->setC();
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0xff);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);
}

void Tester_68k::testSubq() {
    u16 opcode;

    //MOVE.L    #$ffffffff, D1
    //MOVE      #$0, CCR
    //SUBQ.L    #$7, D1
    setUp();
    opcode = 0x5100 | (7 << 9) | (2 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0xffffffff);
    process();
    check("SUBQ.L #$7, D1");

    //MOVE.L    #$ffff0001, A1
    //MOVE      #$0, CCR
    //SUBQ.W    #$7, A1
    setUp();
    opcode = 0x5100 | (7 << 9) | (1 << 6) | getEA(AR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(1, 0xffff0001);
    process();
    check("SUBQ.W #$7, A1");

    //MOVE.B    #$6, $3000
    //MOVE      #$0, CCR
    //SUBQ.B    #$7, ($3000).W
    //MOVE.B    $3000, D1
    setUp();
    opcode = 0x5100 | (7 << 9) | (0 << 6) | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.write(0x3000, 0x6);
    process();
    check("SUBQ.B #$7, ($3000).W");
}
