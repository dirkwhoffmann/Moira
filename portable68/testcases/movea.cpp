
#include "tester.h"
#include "results.h"

void Tester_68k::sampleMovea() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("MOVEA.L A2, A2");
    oObj->setRegA(2, 0xffffffff);
    oObj->setCycleCount(4);

    oObj = new Results("MOVEA.W D2, A2");
    oObj->setRegA(2, 0x00005678);
    oObj->setRegD(2, 0x12345678);
    oObj->setCycleCount(4);

    oObj = new Results("MOVEA.W D2, A2 alt");
    oObj->setRegA(2, 0xffff8756);
    oObj->setRegD(2, 0x12348756);
    oObj->setCycleCount(4);

    oObj = new Results("MOVEA.L #$1, A2");
    oObj->setRegA(2, 0x1);
    oObj->setCycleCount(12);
}

void Tester_68k::testMovea() {
    u16 opcode;

    //MOVE.L    #$ffffffff, A2
    //MOVEA.L   A2, A2
    setUp();
    opcode = (2 << 12) | (2 << 9) | (1 << 6) | getEA(AR_DIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0xffffffff);
    process();
    check("MOVEA.L A2, A2");

    //MOVE.L    #$ffffffff, A2
    //MOVE.L    #$12345678, D2
    //MOVEA.W   D2, A2
    setUp();
    opcode = (3 << 12) | (2 << 9) | (1 << 6) | getEA(DR_DIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0xffffffff);
    setRegD(2, 0x12345678);
    process();
    check("MOVEA.W D2, A2");

    //MOVE.L    #$f0ffffff, A2
    //MOVE.L    #$12348756, D2
    //MOVEA.W   D2, A2
    setUp();
    opcode = (3 << 12) | (2 << 9) | (1 << 6) | getEA(DR_DIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0xf0ffffff);
    setRegD(2, 0x12348756);
    process();
    check("MOVEA.W D2, A2 alt");

    //MOVE.L    #$f0ffffff, A2
    //MOVEA.L   #$1, A2
    setUp();
    opcode = (2 << 12) | (2 << 9) | (1 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0);
    addWord(0x1);
    addWord(0xffff);
    power();
    setRegA(2, 0xf0ffffff);
    process();
    check("MOVEA.L #$1, A2");
}
