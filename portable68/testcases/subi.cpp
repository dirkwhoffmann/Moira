
#include "tester.h"
#include "results.h"
#include "memoryblock.h"

void Tester_68k::sampleSubi() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("SUBI.L #$f1111111, D1");
    oObj->setRegD(1, 0x3eef10a2);
    oObj->setC()->setX();
    oObj->setCycleCount(16);
    oObj->setIrqSampleCyclePos(10);

    oObj = new Results("SUBI.W #$7aaa, $(3000).W");
    oSampleMem = new MemoryBlock();
    oObj->setV();
    oSampleMem->writeWord(0x3000, 0x3000);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
}

void Tester_68k::testSubi() {
    u16 opcode;

    //MOVE.L    #$300021b3, D1
    //MOVE      #$0, CCR
    //SUBI.L    #$f1111111, D1
    setUp();
    opcode = 0x400 | (2 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xf111);
    addWord(0x1111);
    addWord(0xffff);
    power();
    setRegD(1, 0x300021b3);
    process();
    check("SUBI.L #$f1111111, D1");

    //MOVE.W    #$aaaa, $3000
    //MOVE      #$0, CCR
    //SUBI.W    #$7aaa, ($3000).W
    //MOVE.W    $3000, D1
    setUp();
    opcode = 0x400 | (1 << 6) | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x7aaa);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x3000, 0xaaaa);
    process();
    check("SUBI.W #$7aaa, $(3000).W");
}
