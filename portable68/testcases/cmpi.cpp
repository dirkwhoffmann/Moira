
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleCmpi() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("CMPI.W #$ffff, D1");
    oObj->setRegD(1, 0xfff2ffff);
    oObj->setZ();
    oObj->setCycleCount(8);

    oObj = new Results("CMPI.L #$80000000, D1");
    oObj->setRegD(1, 0x7fffffff);
    oObj->setN()->setV()->setC();
    oObj->setCycleCount(14);
    oObj->setIrqSampleCyclePos(10);

    oObj = new Results("CMPI.B #$90, D1");
    oObj->setRegD(1, 0x8f);
    oObj->setN()->setC();
    oObj->setCycleCount(8);
}

void Tester_68k::testCmpi() {
    u16 opcode;

    //MOVE.L    #$fff2ffff, D1
    //MOVE      #$0, CCR
    //CMPI.W    #$ffff, D1
    setUp();
    opcode = 0xc00 | (1 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    addWord(0xffff);
    power();
    setRegD(1, 0xfff2ffff);
    process();
    check("CMPI.W #$ffff, D1");

    //MOVE.L    #$7fffffff, D1
    //MOVE      #$0, CCR
    //CMPI.L    #$80000000, D1
    setUp();
    opcode = 0xc00 | (2 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0x8000);
    addWord(0);
    addWord(0xffff);
    power();
    setRegD(1, 0x7fffffff);
    process();
    check("CMPI.L #$80000000, D1");

    //MOVE.L    #$8f, D1
    //MOVE      #$0, CCR
    //CMPI.B    #$90, D1
    setUp();
    opcode = 0xc00 | (0 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0x0090);
    addWord(0xffff);
    power();
    setRegD(1, 0x8f);
    process();
    check("CMPI.B #$90, D1");
}
