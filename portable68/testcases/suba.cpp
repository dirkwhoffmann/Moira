
#include "tester.h"
#include "results.h"

void Tester_68k::sampleSuba() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("SUBA.L #$12345678, A2");
    oObj->setRegA(2, 0x9c0f54a5);
    oObj->setC()->setN()->setX()->setZ()->setV();
    oObj->setCycleCount(16);
    oObj->setIrqSampleCyclePos(10);

    oObj = new Results("SUBA.W #$5678, A2");
    oObj->setRegA(2, 0xae4354a5);
    oObj->setCycleCount(12);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("SUBA.W #$f678, A2");
    oObj->setRegA(2, 0xae43b4a5);
    oObj->setCycleCount(12);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("SUBA.L -(A2), A2");
    oObj->setRegA(2, 0x8ffe2000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x2000, 0x7002);
    oSampleMem->writeWord(0x2002, 0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);
    oObj->setIrqSampleCyclePos(12);
}

void Tester_68k::testSuba() {
    u16 opcode;

    //MOVE.L #$ae43ab1d, A2
    //MOVE  #$1f, CCR
    //SUBA.L #$12345678, A2
    setUp();
    opcode = 0x9000 | (2 << 9) | (7 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x1234);
    addWord(0x5678);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegA(2, 0xae43ab1d);
    process();
    check("SUBA.L #$12345678, A2");

    //MOVE.L #$ae43ab1d, A2
    //MOVE  #$0, CCR
    //SUBA.W #$5678, A2
    setUp();
    opcode = 0x9000 | (2 << 9) | (3 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x5678);
    addWord(0xffff);
    power();
    setRegA(2, 0xae43ab1d);
    process();
    check("SUBA.W #$5678, A2");

    //MOVE.L #$ae43ab1d, A2
    //MOVE  #$0, CCR
    //SUBA.W #$f678, A2
    setUp();
    opcode = 0x9000 | (2 << 9) | (3 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xf678);
    addWord(0xffff);
    power();
    setRegA(2, 0xae43ab1d);
    process();
    check("SUBA.W #$f678, A2");

    //MOVE.L #$2004, A2
    //MOVE.W #$7002, $2000
    //MOVE.W #0, $2002
    //MOVE   #0, CCR
    //SUBA.L -(A2), A2
    setUp();
    opcode = 0x9000 | (2 << 9) | (7 << 6) | getEA(AR_INDIRECT_DEC, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0x2004);
    memoryblock.writeWord(0x2000, 0x7002);
    memoryblock.writeWord(0x2002, 0);
    process();
    check("SUBA.L -(A2), A2");
}
