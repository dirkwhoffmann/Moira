
#include "tester.h"
#include "results.h"

void Tester_68k::samplePea() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("PEA (A1)");
    oObj->setRegA(1, 0x3000ffff);
    oObj->setRegA(7,0x1992);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x1992, 0x3000ffff);
    oObj->setCodeBlock(oSampleMem);
    oObj->setRegIRD(0xffff);
    oObj->setCycleCount(12);

    oObj = new Results("PEA ($3000).W");
    oObj->setRegA(7,0x1992);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x1992, 0x3000);
    oObj->setCodeBlock(oSampleMem);
    oObj->setRegIRD(0xffff);
    oObj->setCycleCount(16);

}

void Tester_68k::testPea() {
    u16 opcode;

    //MOVE.L    #$3000ffff, A1
    //PEA       (A1)
    //MOVE.L    (A7), D1
    setUp();
    opcode = 0x4840 | getEA(AR_INDIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(1, 0x3000ffff);
    setRegA(7, 0x1996);
    process();
    check("PEA (A1)");

    //PEA       ($3000).W
    //MOVE.L    (A7), D1
    setUp();
    opcode = 0x4840 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setRegA(7, 0x1996);
    process();
    check("PEA ($3000).W");
}
