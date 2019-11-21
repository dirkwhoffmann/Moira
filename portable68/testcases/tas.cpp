
#include "tester.h"
#include "results.h"
#include "memoryblock.h"

void Tester_68k::sampleTas() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("TAS D0");
    oObj->setRegD(0, 0x123456f8);
    oObj->setCycleCount(4);

    oObj = new Results("TAS D0 alt");
    oObj->setRegD(0, 0x123456f0);
    oObj->setN();
    oObj->setCycleCount(4);

    oObj = new Results("TAS ($3000).L");
    oObj->setX();
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x91);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(22);
}

void Tester_68k::testTas() {
    u16 opcode;

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //TAS       D0
    setUp();
    opcode = 0x4ac0 | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("TAS D0");

    //MOVE.L    #$123456f0, D0
    //MOVE      #0, CCR
    //TAS       D0
    setUp();
    opcode = 0x4ac0 | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x123456f0);
    process();
    check("TAS D0 alt");

    //MOVE.B    #$11, $3000
    //MOVE      #$1f, CCR
    //TAS       ($3000).L
    //MOVE.B    $3000, D0
    setUp();
    opcode = 0x4ac0 | getEA(ABS_LONG);
    addWord(opcode);
    addWord(0);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    memoryblock.write(0x3000, 0x11);
    process();
    check("TAS ($3000).L");
}
