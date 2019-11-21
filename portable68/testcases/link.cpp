
#include "tester.h"
#include "results.h"

void Tester_68k::sampleLink() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("LINK A1, #5");
    oObj->setRegA(1, 0x2222221e);
    oObj->setRegA(7, 0x22222223);
    oSampleMem = new MemoryBlock;
    oSampleMem->writeLong(0x2222221e, 0x11111111);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);

    oObj = new Results("LINK A7, #5");
    oObj->setRegA(7, 0x22222223);
    oSampleMem = new MemoryBlock;
    oSampleMem->writeLong(0x2222221e, 0x2222221e);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);

    oObj = new Results("LINK A1, #8000");
    oObj->setRegA(1, 0x2222221e);
    oObj->setRegA(7, 0x2221a21e);
    oSampleMem = new MemoryBlock;
    oSampleMem->writeLong(0x2222221e, 0x11111111);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);
}

void Tester_68k::testLink() {
    u16 opcode;

    //MOVE.L    #$11111111, A1
    //MOVE.L    #$22222222, A7
    //LINK      A1, #5
    //MOVE.L    #$2222221e, D1
    setUp();
    opcode = 0x4e50 | 1;
    addWord(opcode);
    addWord(5);
    addWord(0xffff);
    power();
    setRegA(1, 0x11111111);
    setRegA(7, 0x22222222);
    process();
    check("LINK A1, #5");

    //MOVE.L    #$22222222, A7
    //LINK      A7, #5
    //MOVE.L    #$2222221e, D1
    setUp();
    opcode = 0x4e50 | 7;
    addWord(opcode);
    addWord(5);
    addWord(0xffff);
    power();
    setRegA(7, 0x22222222);
    process();
    check("LINK A7, #5");

    //MOVE.L    #$11111111, A1
    //MOVE.L    #$22222222, A7
    //LINK      A1, #8000
    //MOVE.L    #$2222221e, D1
    setUp();
    opcode = 0x4e50 | 1;
    addWord(opcode);
    addWord(0x8000);
    addWord(0xffff);
    power();
    setRegA(1, 0x11111111);
    setRegA(7, 0x22222222);
    process();
    check("LINK A1, #8000");

}
