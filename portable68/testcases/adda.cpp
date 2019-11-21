
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleAdda() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ADDA.L #$12345678, A2");
    oObj->setRegA(2, 0xc0780195);
    oObj->setC()->setN()->setX()->setZ()->setV();
    oObj->setCycleCount(16);
    oObj->setIrqSampleCyclePos(10);

    oObj = new Results("ADDA.W #$5678, A2");
    oObj->setRegA(2, 0xae440195);
    oObj->setCycleCount(12);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("ADDA.W #$f678, A2");
    oObj->setRegA(2, 0xae43a195);
    oObj->setCycleCount(12);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("ADDA.W #$f000, A2");
    oObj->setRegA(2, 0xfffff000);
    oObj->setCycleCount(12);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("ADDA.L -(A2), A2");
    oObj->setRegA(2, 0x70022000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x2000, 0x7002);
    oSampleMem->writeWord(0x2002, 0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);
    oObj->setIrqSampleCyclePos(12);
}

void Tester_68k::testAdda() {
    u16 opcode;

    //MOVE.L #$ae43ab1d, A2
    //MOVE  #$1f, CCR
    //ADDA.L #$12345678, A2
    setUp();
    opcode = 0xd000 | (2 << 9) | (7 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x1234);
    addWord(0x5678);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegA(2, 0xae43ab1d);
    process();
    check("ADDA.L #$12345678, A2");

    //MOVE.L #$ae43ab1d, A2
    //MOVE  #$0, CCR
    //ADDA.W #$5678, A2
    setUp();
    opcode = 0xd000 | (2 << 9) | (3 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x5678);
    addWord(0xffff);
    power();
    setRegA(2, 0xae43ab1d);
    process();
    check("ADDA.W #$5678, A2");

    //MOVE.L #$ae43ab1d, A2
    //MOVE  #$0, CCR
    //ADDA.W #$f678, A2
    setUp();
    opcode = 0xd000 | (2 << 9) | (3 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xf678);
    addWord(0xffff);
    power();
    setRegA(2, 0xae43ab1d);
    process();
    check("ADDA.W #$f678, A2");

    //MOVE.L #0, A2
    //MOVE  #$0, CCR
    //ADDA.W #$f000, A2
    setUp();
    opcode = 0xd000 | (2 << 9) | (3 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xf000);
    addWord(0xffff);
    power();
    setRegA(2, 0);
    process();
    check("ADDA.W #$f000, A2");

    //MOVE.L #$2004, A2
    //MOVE.W #$7002, $2000
    //MOVE.W #0, $2002
    //MOVE   #0, CCR
    //ADDA.L -(A2), A2
    setUp();
    opcode = 0xd000 | (2 << 9) | (7 << 6) | getEA(AR_INDIRECT_DEC, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0x2004);
    memoryblock.writeWord(0x2000, 0x7002);
    memoryblock.writeWord(0x2002, 0);
    process();
    check("ADDA.L -(A2), A2");
}
