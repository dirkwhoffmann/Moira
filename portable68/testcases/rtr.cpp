#include "tester.h"
#include "results.h"

void Tester_68k::sampleRtr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("RTR");
    oObj->setV()->setN()->setC()->setZ()->setX();
    oObj->setRegA(7, 0x2006);
    oObj->setRegIRD(0x4e71);
    oObj->setCycleCount(20);
}

void Tester_68k::testRtr() {
    u16 opcode;
    //MOVE.L    #$2000, A7
    //MOVE.W    #$ffff, $2000
    //MOVE.L    #$c, $2002
    //RTR
    //NOP
    //NOP
    setUp();
    opcode = 0x4e77;
    addWord(opcode);
    addWord(0x4e71);
    addWord(0x4e71);
    power();
    setRegA(7, 0x2000);
    memoryblock.writeWord(0x2000, 0xffff);
    memoryblock.writeLong(0x2002, 0xc);
    process();
    check("RTR");
}
