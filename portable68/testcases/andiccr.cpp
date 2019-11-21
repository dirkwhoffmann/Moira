
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleAndiCcr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ANDI.B #$1b, CCR");
    oObj->setN();
    oObj->setCycleCount(20);
}

void Tester_68k::testAndiCcr() {
    u16 opcode;

    //MOVE    #$c, CCR
    //ANDI.B  #$1b, CCR
    setUp();
    opcode = 0x23c;
    addWord(opcode);
    addWord(0x1b);
    addWord(0xffff);
    power();
    setCCR(0xc);
    process();
    check("ANDI.B #$1b, CCR");

}
