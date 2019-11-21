
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleBclr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("BCLR D1, D0 bit 0");
    oObj->setRegD(1, 0);
    oObj->setRegD(0, 0x12345678);
    oObj->setZ();
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("BCLR D1, D0 bit 10");
    oObj->setRegD(1, 10);
    oObj->setRegD(0, 0x12345278);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("BCLR D1, D0 bit 50");
    oObj->setRegD(1, 50);
    oObj->setRegD(0, 0x12305678);
    oObj->setCycleCount(10);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("BCLR D1, (A0) bit 50");
    oObj->setRegD(1, 50);
    oObj->setRegA(0, 0x3000);
    oObj->setZ();
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x78);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(12);

    oObj = new Results("BCLR D1, (A0) bit 3");
    oObj->setRegD(1, 3);
    oObj->setRegA(0, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x70);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(12);

    oObj = new Results("BCLR #28, D0");
    oObj->setRegD(0, 0x02345678);
    oObj->setCycleCount(14);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("BCLR #4, D0");
    oObj->setRegD(0, 0x12345668);
    oObj->setCycleCount(12);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("BCLR #6, ($3000).W");
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x38);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
}

void Tester_68k::testBclr() {
    u16 opcode;

    //MOVE.L    #$12345678, D0
    //MOVE.L    #0, D1
    //MOVE      #0, CCR
    //BCLR      D1, D0
    setUp();
    opcode = 0x180 | (1 << 9) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    setRegD(1, 0);
    process();
    check("BCLR D1, D0 bit 0");

    //MOVE.L    #$12345678, D0
    //MOVE.L    #10, D1
    //MOVE      #0, CCR
    //BCLR      D1, D0
    setUp();
    opcode = 0x180 | (1 << 9) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    setRegD(1, 10);
    process();
    check("BCLR D1, D0 bit 10");

    //MOVE.L    #$12345678, D0
    //MOVE.L    #50, D1
    //MOVE      #0, CCR
    //BCLR      D1, D0
    setUp();
    opcode = 0x180 | (1 << 9) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    setRegD(1, 50);
    process();
    check("BCLR D1, D0 bit 50");

    //MOVE.L    #$3000, A0
    //MOVE.L    #$78, $3000
    //MOVE.L    #50, D1
    //MOVE      #0, CCR
    //BCLR      D1, (A0)
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0x180 | (1 << 9) | getEA(AR_INDIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(0, 0x3000);
    setRegD(1, 50);
    memoryblock.write(0x3000, 0x78);
    process();
    check("BCLR D1, (A0) bit 50");

    //MOVE.L    #$3000, A0
    //MOVE.B    #$78, $3000
    //MOVE.L    #3, D1
    //MOVE      #0, CCR
    //BCLR      D1, (A0)
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0x180 | (1 << 9) | getEA(AR_INDIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(0, 0x3000);
    setRegD(1, 3);
    memoryblock.write(0x3000, 0x78);
    process();
    check("BCLR D1, (A0) bit 3");

    //static

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //BCLR      #28, D0
    setUp();
    opcode = 0x880 | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(28);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("BCLR #28, D0");

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //BCLR      #4, D0
    setUp();
    opcode = 0x880 | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(4);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("BCLR #4, D0");

    //MOVE.B    #$78, $3000
    //MOVE      #0, CCR
    //BCLR      #6, ($3000).W
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0x880 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(6);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.write(0x3000, 0x78);
    process();
    check("BCLR #6, ($3000).W");
}
