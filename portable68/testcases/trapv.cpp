#include "tester.h"
#include "results.h"

void Tester_68k::sampleTrapv() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("TRAPV");
    oObj->setRegS(0x2702);
    oObj->setV();
    oObj->setRegA(7, 0x200);
    oObj->setRegIRD(0x4e71);
    oSampleMem = new MemoryBlock;
    oSampleMem->writeLong(0x202, 0xa);
    oSampleMem->writeWord(0x200, 0x702);
    oObj->setCodeBlock(oSampleMem);
    oObj->expectGroup2Exception(true);
    oObj->setCycleCount(34);

    oObj = new Results("TRAPV alt");
    oObj->setRegIRD(0x4e71);
    oObj->setCycleCount(4);
}

void Tester_68k::testTrapv() {
    u16 opcode;

    //MOVE  #$702, SR
    //TRAPV
    setUp();
    opcode = 0x4e76;
    addWord(opcode);
    power();
    setSR(0x702);
    setRegA(7, 0x206);
    setSSP(0x206);
    memoryblock.writeLong(0x1c, 0xfffe);
    memoryblock.writeWord(0xfffe, 0x4e71);
    process();
    check("TRAPV");

    //MOVE  #$0, CCR
    //TRAPV
    setUp();
    opcode = 0x4e76;
    addWord(opcode);
    addWord(0x4e71);
    power();
    process();
    check("TRAPV alt");
}
