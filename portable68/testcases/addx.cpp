
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleAddx() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("addx.l D1, D2");
    oObj->setRegD(1, 0x12345678);
    oObj->setRegD(2, 0x2468acf1);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("addx.b D1, D2");
    oObj->setRegD(1, 0x80);
    oObj->setRegD(2, 0x8000);
    oObj->setC()->setV()->setX();
    oObj->setCycleCount(4);

    oObj = new Results("addx.w D1, D2");
    oObj->setRegD(1, 0x1ffff);
    oObj->setRegD(2, 0x18080);
    oObj->setC()->setN()->setX();
    oObj->setCycleCount(4);

    oObj = new Results("ADDX.L -(A1), -(A1)");
    oObj->setRegA(1, 0x2ff8);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x2ffc, 0x70001ff1);
    oSampleMem->writeLong(0x2ff8, 0x80001ff2);
    oObj->setCodeBlock(oSampleMem);
    oObj->setN()->setV();
    oObj->setCycleCount(30);
}

void Tester_68k::testAddx() {
    u16 opcode;

    //MOVE.L    #$12345678, D1
    //MOVE.L    #$12345678, D2
    //MOVE      #$1f, CCR
    //ADDX.L    D1, D2
    setUp();
    opcode = 0xd100 | (2 << 9) | (1) | (2 << 6) | (0 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x12345678);
    setRegD(2, 0x12345678);
    process();
    check("addx.l D1, D2");

    //MOVE.L    #$80, D1
    //MOVE.L    #$8080, D2
    //MOVE      #$0, CCR
    //ADDX.L    D1, D2
    setUp();
    opcode = 0xd100 | (2 << 9) | (1) | (0 << 6) | (0 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0);
    setRegD(1, 0x80);
    setRegD(2, 0x8080);
    process();
    check("addx.b D1, D2");

    //MOVE.L    #$1ffff, D1
    //MOVE.L    #$18080, D2
    //MOVE      #$1f, CCR
    //ADDX.W    D1, D2
    setUp();
    opcode = 0xd100 | (2 << 9) | (1) | (1 << 6) | (0 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x1ffff);
    setRegD(2, 0x18080);
    process();
    check("addx.w D1, D2");

    //MOVE.L    #$3000, A1
    //MOVE.L    #$70001ff1, $2ffc
    //MOVE.L    #$10000000, $2ff8
    //MOVE      #$1f, CCR
    //ADDX.L    -(A1), -(A1)
    //MOVE.L    $2ff8, D1
    setUp();
    opcode = 0xd100 | (1 << 9) | (1) | (2 << 6) | (1 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegA(1, 0x3000);
    memoryblock.writeLong(0x2ffc, 0x70001ff1);
    memoryblock.writeLong(0x2ff8, 0x10000000);
    process();
    check("ADDX.L -(A1), -(A1)");
}
