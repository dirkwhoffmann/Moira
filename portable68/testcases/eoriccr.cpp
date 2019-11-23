
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleEoriCcr() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("EORI.B #$1b, CCR");
    oObj->setX()->setZ()->setV()->setC();
    oObj->setCycleCount(20);
}

void Tester_68k::testEoriCcr() {
    u16 opcode;

    //MOVE    #$c, CCR
    //EORI.B  #$1b, CCR
    setUp();
    opcode = 0xa3c;
    addWord(opcode);
    addWord(0x1b);
    addWord(0xffff);
    power();
    setCCR(0xc);
    process();
    check("EORI.B #$1b, CCR");
}
