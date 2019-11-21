
#include "tester.h"
#include "results.h"

void Tester_68k::sampleJmp() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("JMP (A1)");
    oObj->setRegA(1, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3000, 0xffff);
    oObj->setCodeBlock(oSampleMem);
    oObj->setRegIRD(0xffff);
    oObj->setCycleCount(8);

    oObj = new Results("JMP GO(PC)");
    oObj->setRegIRD(0xffbb);
    oObj->setCycleCount(10);

}

void Tester_68k::testJmp() {
    u16 opcode;

    //MOVE.L    #$3000, A1
    //MOVE.W    #$ffff, $3000
    //JMP       (A1)
    setUp();
    opcode = 0x4ec0 | getEA(AR_INDIRECT, 1);
    addWord(opcode);
    power();
    setRegA(1, 0x3000);
    memoryblock.writeWord(0x3000, 0xffff);
    process();
    check("JMP (A1)");

    //JMP       GO(PC)
    //MOVE      #0, CCR
    //GO:
    setUp();
    opcode = 0x4ec0 | getEA(PC_INDIRECT_D16);
    addWord(opcode);
    addWord(6);
    addWord(0x44fc);
    addWord(0);
    addWord(0xffbb);
    power();
    process();
    check("JMP GO(PC)");
}
