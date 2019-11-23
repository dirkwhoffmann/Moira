
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleDivu() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("DIVU.W #$1, D1");
    oObj->setX()->setN()->setV();
    oObj->setRegD(1, 0x4768f231);
    oObj->setCycleCount(14);

    oObj = new Results("DIVU.W #$1234, D1");
    oObj->setX()->setN()->setV();
    oObj->setRegD(1, 0x4768f231);
    oObj->setCycleCount(14);

    oObj = new Results("DIVU.W #$eeff, D1");
    oObj->setX();
    oObj->setRegD(1, 0x8bae4c7d);
    oObj->setCycleCount(108);

    oObj = new Results("DIVU.W #$3333, D1");
    oObj->setX()->setN();
    oObj->setRegD(1, 0x1fffa000);
    oObj->setCycleCount(136);

    oObj = new Results("DIVU.W #0, D1");
    oObj->setX();
    oObj->expectGroup2Exception(true);
    oObj->setRegD(1, 0x1fffffff);
    oObj->setRegA(7, 0xfffffffa);
    oObj->setRegIRD(0);
    oObj->setCycleCount(42);
}

void Tester_68k::testDivu() {
    u16 opcode;

    //MOVE.L    #$4768f231, D1
    //MOVE      #$1f, CCR
    //DIVU.W    #$1, D1
    setUp();
    opcode = 0x80c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x1);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x4768f231);
    process();
    check("DIVU.W #$1, D1");

    //MOVE.L    #$4768f231, D1
    //MOVE      #$1f, CCR
    //DIVU.W    #$1234, D1
    setUp();
    opcode = 0x80c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x1234);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x4768f231);
    process();
    check("DIVU.W #$1234, D1");

    //MOVE.L    #$4768f231, D1
    //MOVE      #$1f, CCR
    //DIVU.W    #$eeff, D1
    setUp();
    opcode = 0x80c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xeeff);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x4768f231);
    process();
    check("DIVU.W #$eeff, D1");

    //MOVE.L    #$1fffffff, D1
    //MOVE      #$1f, CCR
    //DIVU.W    #0, D1
    setUp();
    opcode = 0x80c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x1fffffff);
    process();
    check("DIVU.W #0, D1");
}
