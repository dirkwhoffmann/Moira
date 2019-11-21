
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleNot() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("NOT.B D0");
    oObj->setRegD(0, 0x12345687);
    oObj->setN();
    oObj->setCycleCount(4);

    oObj = new Results("NOT.W D0");
    oObj->setRegD(0, 0x1234ffff);
    oObj->setN()->setX();
    oObj->setCycleCount(4);

    oObj = new Results("NOT.L D0");
    oObj->setRegD(0, 0x000000ff);
    oObj->setX();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("NOT.L ($3000).L");
    oObj->setX();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0ffedcee);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(28);
}

void Tester_68k::testNot() {
    u16 opcode;

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //NOT.B     D0
    setUp();
    opcode = 0x4600 | (0 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("NOT.B D0");

    //MOVE.L    #$12340000, D0
    //MOVE      #$1f, CCR
    //NOT.W     D0
    setUp();
    opcode = 0x4600 | (1 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0x12340000);
    process();
    check("NOT.W D0");

    //MOVE.L    #$ffffff00, D0
    //MOVE      #$1f, CCR
    //NOT.L     D0
    setUp();
    opcode = 0x4600 | (2 << 6) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(0, 0xffffff00);
    process();
    check("NOT.L D0");

    //MOVE.L    #$f0012311, $3000
    //MOVE      #$10, CCR
    //NOT.L    ($3000).L
    //MOVE.L    $3000, D0
    setUp();
    opcode = 0x4600 | (2 << 6) | getEA(ABS_LONG);
    addWord(opcode);
    addWord(0);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0x10);
    memoryblock.writeLong(0x3000, 0xf0012311);
    process();
    check("NOT.L ($3000).L");

}
