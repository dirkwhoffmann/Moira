#include "tester.h"
#include "results.h"

void Tester_68k::sampleTrap() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("TRAP #1");
    oObj->setRegS(0x2700);
    oObj->setRegA(7, 0x200);
    oObj->setRegIRD(0x4e71);
    oSampleMem = new MemoryBlock;
    oSampleMem->writeLong(0x202, 0xa);
    oSampleMem->writeWord(0x200, 0x700);
    oObj->setCodeBlock(oSampleMem);
    oObj->expectGroup2Exception(true);
    oObj->setCycleCount(34);
}

void Tester_68k::testTrap() {
    u16 opcode;

    //MOVE  #$700, SR
    //TRAP  #1
    setUp();
    opcode = 0x4e40 | 1;
    addWord(opcode);
    power();
    setSR(0x700);
    setRegA(7, 0x206);
    setSSP(0x206);
    memoryblock.writeLong(0x84, 0xfffe);
    memoryblock.writeWord(0xfffe, 0x4e71);
    process();
    check("TRAP #1");
}
