
#include "tester.h"
#include "results.h"
#include "memoryblock.h"

void Tester_68k::sampleSubx() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("subx.l D1, D2");
    oObj->setRegD(1, 0x12345678);
    oObj->setRegD(2, 0xffffffff);
    oObj->setX()->setC()->setN();
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("subx.b D1, D2");
    oObj->setRegD(1, 0x80);
    oObj->setRegD(2, 0x81);
    oObj->setX()->setC()->setN()->setV();
    oObj->setCycleCount(4);

    oObj = new Results("SUBX.L -(A1), -(A1)");
    oObj->setRegA(1, 0x2ff8);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x2ffc, 0x70001ff1);
    oSampleMem->writeLong(0x2ff8, 0x9fffE00E);
    oObj->setCodeBlock(oSampleMem);
    oObj->setN()->setX()->setC();
    oObj->setCycleCount(30);

}

void Tester_68k::testSubx() {
    u16 opcode;

    //MOVE.L    #$12345678, D1
    //MOVE.L    #$12345678, D2
    //MOVE      #$1f, CCR
    //SUBX.L    D1, D2
    setUp();
    opcode = 0x9100 | (2 << 9) | (1) | (2 << 6) | (0 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x12345678);
    setRegD(2, 0x12345678);
    process();
    check("subx.l D1, D2");

    //MOVE.L    #$80, D1
    //MOVE.L    #$01, D2
    //MOVE      #$1f, CCR
    //SUBX.B    D1, D2
    setUp();
    opcode = 0x9100 | (2 << 9) | (1) | (0 << 6) | (0 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x80);
    setRegD(2, 0x01);
    process();
    check("subx.b D1, D2");

    //MOVE.L    #$3000, A1
    //MOVE.L    #$70001ff1, $2ffc
    //MOVE.L    #$10000000, $2ff8
    //MOVE      #$1f, CCR
    //SUBX.L    -(A1), -(A1)
    //MOVE.L    $2ff8, D1
    setUp();
    opcode = 0x9100 | (1 << 9) | (1) | (2 << 6) | (1 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegA(1, 0x3000);
    memoryblock.writeLong(0x2ffc, 0x70001ff1);
    memoryblock.writeLong(0x2ff8, 0x10000000);
    process();
    check("SUBX.L -(A1), -(A1)");

}
