
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleMoveToSr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("MOVE #$0, SR");
    oObj->setRegA(7, 0);
    oObj->setInt(0)->resetS();
    oObj->setCycleCount(16);
}

void Tester_68k::testMoveToSr() {
    u16 opcode;

    //MOVE.L  #$300, A7
    //MOVE    #$700, SR
    setUp();
    opcode = 0x46c0 | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0);
    addWord(0xffff);
    power();
    setRegA(7, 0x300);
    process();
    check("MOVE #$0, SR");
}
