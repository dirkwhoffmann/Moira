
#include "tester.h"
#include "results.h"

void Tester_68k::sampleJsr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("JSR ($x).L");
    oObj->setRegIRD(0x2217);
    oObj->setRegA(7, 0x1ffc);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x1ffc, 0xe);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);

    oObj = new Results("JSR GO(PC)");
    oObj->setRegIRD(0x2217);
    oObj->setRegA(7, 0x1ffc);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x1ffc, 0xc);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(18);

}

void Tester_68k::testJsr() {
    u16 opcode;

    //JSR   ($x).L  //x = PC + 8
    //RTS
    //MOVE.L (A7), D1
    setUp();
    opcode = 0x4e80 | getEA(ABS_LONG);
    addWord(opcode);
    addWord(0);
    addWord(0x10);
    addWord(0x4e75);
    addWord(0x2217);
    power();
    setRegA(7, 0x2000);
    process();
    check("JSR ($x).L");

    //JSR   GO(PC)
    //RTS
    //GO:
    //MOVE.L (A7), D1
    setUp();
    opcode = 0x4e80 | getEA(PC_INDIRECT_D16);
    addWord(opcode);
    addWord(4);
    addWord(0x4e75);
    addWord(0x2217);
    power();
    setRegA(7, 0x2000);
    process();
    check("JSR GO(PC)");

}
