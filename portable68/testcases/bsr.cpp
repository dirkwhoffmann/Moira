
#include "tester.h"
#include "results.h"

void Tester_68k::sampleBsr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("BSR.B Label");
    oObj->setRegA(7, 0x2ffc);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x2ffc, 10);
    oObj->setCodeBlock(oSampleMem);
    oObj->setRegIRD(0x22e7);
    oObj->setCycleCount(18);

    oObj = new Results("BSR.W Label");
    oObj->setRegA(7, 0x2ffc);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x2ffc, 12);
    oObj->setCodeBlock(oSampleMem);
    oObj->setRegIRD(0x22e7);
    oObj->setCycleCount(18);
}

void Tester_68k::testBsr() {
    u16 opcode;

    //BSR.B Label
    //RTS
//Label:
    //MOVE.L (A7), D1
    //RTS
    setUp();
    opcode = 0x6102;
    addWord(opcode); //pc: 8
    addWord(0x4e75); //pc: 10
    addWord(0x22e7);
    power();
    setRegA(7, 0x3000);
    process();
    check("BSR.B Label");

    //BSR.W Label
    //RTS
//Label:
    //MOVE.L (A7), D1
    //RTS
    setUp();
    opcode = 0x6100;
    addWord(opcode); //pc: 8
    addWord(0x0004); //pc: 10
    addWord(0x4e75); //pc: 12
    addWord(0x22e7);
    power();
    setRegA(7, 0x3000);
    process();
    check("BSR.W Label");

}
