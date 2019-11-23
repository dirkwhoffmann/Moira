
#include "tester.h"
#include "results.h"

void Tester_68k::sampleSwap() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("SWAP D1");
    oObj->setN();
    oObj->setRegD(1, 0x87561234);
    oObj->setCycleCount(4);
}

void Tester_68k::testSwap() {
    u16 opcode;

    //MOVE.L    #$12348756, D1
    //SWAP      D1
    setUp();
    opcode = 0x4840 | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0x12348756);
    process();
    check("SWAP D1");

}
