
#include "tester.h"
#include "results.h"

void Tester_68k::sampleMoveUsp() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("MOVE USP, A1");
    oObj->setRegA(1, 0);
    oObj->setCycleCount(4);
}

void Tester_68k::testMoveUsp() {
    u16 opcode;

    //MOVE.L    #$12345678, A1
    //MOVE      USP, A1
    setUp();
    opcode = 0x4e60 | 1 | (1<<3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(1, 0x12345678);
    process();
    check("MOVE USP, A1");
}
