
#include "tester.h"
#include "results.h"

void Tester_68k::sampleNop() {
    Results* oObj;

    oObj = new Results("nop");
    oObj->setCycleCount(4);
}

void Tester_68k::testNop() {
    u16 opcode;

    //NOP
    setUp();
    opcode = 0x4e71;
    addWord(opcode);
    addWord(0xffff);
    power();
    process();
    check("nop");
}
