
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleAddi() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ADDI.L #$11111111, D1");
    oObj->setRegD(1, 0x411132C4);
    oObj->setCycleCount(16);
    oObj->setIrqSampleCyclePos(10);

    oObj = new Results("ADDI.W #$7aaa, $(3000).W");
    oSampleMem = new MemoryBlock();
    oObj->setX()->setC();
    oSampleMem->writeWord(0x3000, 0x2554);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
}

void Tester_68k::testAddi() {
    u16 opcode;

    //MOVE.L    #$300021b3, D1
    //MOVE      #$0, CCR
    //ADDI.L    #$11111111, D1
    setUp();
    opcode = 0x600 | (2 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0x1111);
    addWord(0x1111);
    addWord(0xffff);
    power();
    setRegD(1, 0x300021b3);
    process();
    check("ADDI.L #$11111111, D1");

    //MOVE.W    #$aaaa, $3000
    //MOVE      #$0, CCR
    //ADDI.W    #$7aaa, ($3000).W
    //MOVE.W    $3000, D1
    setUp();
    opcode = 0x600 | (1 << 6) | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x7aaa);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x3000, 0xaaaa);
    process();
    check("ADDI.W #$7aaa, $(3000).W");
}
