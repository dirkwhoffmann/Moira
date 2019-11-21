
#include "tester.h"
#include "results.h"

void Tester_68k::sampleOr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("or.b D4, D3");
    oObj->setRegD(3, 0x54ff00df);
    oObj->setRegD(4, 0x9853abcd);
    oObj->setX()->setN();
    oObj->setCycleCount(4);

    oObj = new Results("or.l -6(A4), D3");
    oObj->setRegD(3, 0x56fffb76);
    oObj->setRegA(4, 0x3006);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x1253fb34);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(18);
    oObj->setIrqSampleCyclePos(14);

    oObj = new Results("or.l D3, -6(A4)");
    oObj->setRegD(3, 0x54fff856);
    oObj->setRegA(4, 0x3006);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x56fffb76);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(24);
}

void Tester_68k::testOr() {
    u16 opcode;

    //MOVE.L    #$9853abcd, D4
    //MOVE.L    #$54ff0056, D3
    //MOVE      #$13, CCR
    //OR.B     D4, D3
    setUp();
    opcode = 0x8000 | (3 << 9) | (0 << 6) | getEA(DR_DIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x13);
    setRegD(3, 0x54ff0056);
    setRegD(4, 0x9853abcd);
    process();
    check("or.b D4, D3");

    //MOVE.L    #$3006, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb34, $3000
    //MOVE      #$0, CCR
    //OR.L     -6(A4), D3
    setUp();
    opcode = 0x8000 | (3 << 9) | (2 << 6) | getEA(AR_INDIRECT_D16, 4);
    addWord(opcode);
    addWord(0xfffa);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3006);
    memoryblock.writeLong(0x3000, 0x1253fb34);
    process();
    check("or.l -6(A4), D3");

    //MOVE.L    #$3006, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb34, $3000
    //MOVE      #$0, CCR
    //OR.L      D3, -6(A4)
    setUp();
    opcode = 0x8000 | (3 << 9) | (6 << 6) | getEA(AR_INDIRECT_D16, 4);
    addWord(opcode);
    addWord(0xfffa);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3006);
    memoryblock.writeLong(0x3000, 0x1253fb34);
    process();
    check("or.l D3, -6(A4)");
}
