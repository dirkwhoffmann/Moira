
#include "tester.h"
#include "results.h"

void Tester_68k::sampleClr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("clr.w D4");
    oObj->setRegD(4, 0x98530000);
    oObj->setX()->setZ();
    oObj->setCycleCount(4);

    oObj = new Results("clr.l D4");
    oObj->setRegD(4, 0x0);
    oObj->setX()->setZ();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("clr.l ($186a0).l");
    oSampleMem = new MemoryBlock;
    oSampleMem->writeLong(0x186a0, 0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setX()->setZ();
    oObj->setCycleCount(28);

    oObj = new Results("clr.b ($186a0).l");
    oSampleMem = new MemoryBlock;
    oSampleMem->writeLong(0x186a0, 0x0053abcd);
    oObj->setCodeBlock(oSampleMem);
    oObj->setX()->setZ();
    oObj->setCycleCount(20);
}

void Tester_68k::testClr() {
    u16 opcode;

    //MOVE.L #$9853abcd, D4
    //MOVE #$1b, CCR
    //CLR.W D4
    setUp();
    opcode = 0x4200 | (1 << 6) | getEA(DR_DIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(4, 0x9853abcd);
    setCCR(0x1b);
    process();
    check("clr.w D4");

    //MOVE.L #$9853abcd, D4
    //MOVE #$1b, CCR
    //CLR.L D4
    setUp();
    opcode = 0x4200 | (2 << 6) | getEA(DR_DIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(4, 0x9853abcd);
    setCCR(0x1b);
    process();
    check("clr.l D4");

    //MOVE.L #$9853abcd, $186a0
    //MOVE #$1b, CCR
    //CLR.L ($186a0).l
    //MOVE.L $186a0, D4
    setUp();
    opcode = 0x4200 | (2 << 6) | getEA(ABS_LONG);
    addWord(opcode);
    addWord(1);
    addWord(0x86a0);
    addWord(0xffff);
    power();
    memoryblock.writeLong(0x186a0, 0x9853abcd);
    setCCR(0x1b);
    process();
    check("clr.l ($186a0).l");

    //MOVE.L #$9853abcd, $186a0
    //MOVE #$1b, CCR
    //CLR.B ($186a0).l
    //MOVE.L $186a0, D4
    setUp();
    opcode = 0x4200 | (0 << 6) | getEA(ABS_LONG);
    addWord(opcode);
    addWord(1);
    addWord(0x86a0);
    addWord(0xffff);
    power();
    memoryblock.writeLong(0x186a0, 0x9853abcd);
    setCCR(0x1b);
    process();
    check("clr.b ($186a0).l");
}
