
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleOriCcr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ORI.B #$1b, CCR");
    oObj->setN()->setC()->setX()->setZ()->setV();
    oObj->setCycleCount(20);
}

void Tester_68k::testOriCcr() {
    u16 opcode;

    //MOVE    #$c, CCR
    //ORI.B  #$1b, CCR
    setUp();
    opcode = 0x3c;
    addWord(opcode);
    addWord(0x1b);
    addWord(0xffff);
    power();
    setCCR(0xc);
    process();
    check("ORI.B #$1b, CCR");

}
