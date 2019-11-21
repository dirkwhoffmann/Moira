
#include "tester.h"
#include "results.h"

void Tester_68k::sampleEor() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("eor.w D4, D3");
    oObj->setRegD(3, 0x54ff0056);
    oObj->setRegD(4, 0x9853ab9b);
    oObj->setX()->setN();
    oObj->setCycleCount(4);

    oObj = new Results("eor.l D3, (A2)");
    oObj->setRegD(3, 0x54ff0056);
    oObj->setRegA(2, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x5bf00f47);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
}

void Tester_68k::testEor() {
    u16 opcode;

    //MOVE.L    #$9853abcd, D4
    //MOVE.L    #$54ff0056, D3
    //MOVE      #$13, CCR
    //EOR.W     D3, D4
    setUp();
    opcode = 0xb000 | (3 << 9) | (5 << 6) | getEA(DR_DIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x13);
    setRegD(3, 0x54ff0056);
    setRegD(4, 0x9853abcd);
    process();
    check("eor.w D4, D3");

    //MOVE.L    #$54ff0056, D3
    //MOVE.L    #$3000, A2
    //MOVE.L    #$0f0f0f11, $3000
    //MOVE      #$0, CCR
    //MOVE.L    $3000, D4
    //EOR.L     D3, (A2)
    setUp();
    opcode = 0xb000 | (3 << 9) | (6 << 6) | getEA(AR_INDIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    memoryblock.writeLong(0x3000, 0x0f0f0f11);
    setCCR(0x0);
    setRegA(2, 0x3000);
    setRegD(3, 0x54ff0056);
    process();
    check("eor.l D3, (A2)");
}
