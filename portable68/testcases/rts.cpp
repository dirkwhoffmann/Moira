#include "tester.h"
#include "results.h"

void Tester_68k::sampleRts() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("RTS");
    oObj->setRegA(7, 0x2004);
    oObj->setRegIRD(0x4e71);
    oObj->setCycleCount(16);
}

void Tester_68k::testRts() {
    u16 opcode;
    //MOVE.L    #$2000, A7
    //MOVE.L    #$c, $2000
    //RTS
    //NOP
    //NOP
    setUp();
    opcode = 0x4e75;
    addWord(opcode);
    addWord(0x4e71);
    addWord(0x4e71);
    power();
    setRegA(7, 0x2000);
    memoryblock.writeLong(0x2000, 0xc);
    process();
    check("RTS");
}
