
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleMoveToCcr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("MOVE #$1f, CCR");
    oObj->setC()->setN()->setZ()->setX()->setV();
    oObj->setCycleCount(16);
}

void Tester_68k::testMoveToCcr() {
    u16 opcode;

    //MOVE  #$1f, CCR
    setUp();
    opcode = 0x44c0 | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xff1f);
    addWord(0xffff);
    power();
    process();
    check("MOVE #$1f, CCR");
}
