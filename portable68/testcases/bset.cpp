
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleBset() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("BSET D1, D0 bit 0");
    oObj->setRegD(1, 0);
    oObj->setRegD(0, 0x12345679);
    oObj->setZ();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("BSET D1, D0 bit 10");
    oObj->setRegD(1, 10);
    oObj->setRegD(0, 0x12345678);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("BSET D1, D0 bit 49");
    oObj->setRegD(1, 49);
    oObj->setRegD(0, 0x12365678);
    oObj->setZ();
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("BSET D1, (A0) bit 50");
    oObj->setRegD(1, 50);
    oObj->setRegA(0, 0x3000);
    oObj->setZ();
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x7c);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(12);

    oObj = new Results("BSET D1, (A0) bit 3");
    oObj->setRegD(1, 3);
    oObj->setRegA(0, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x78);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(12);

    oObj = new Results("BSET #28, D0");
    oObj->setRegD(0, 0x12345678);
    oObj->setCycleCount(12);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("BSET #2, D0");
    oObj->setRegD(0, 0x1234567c);
    oObj->setZ();
    oObj->setCycleCount(10);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("BSET #6, ($3000).W");
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x78);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
}

void Tester_68k::testBset() {
    u16 opcode;

    //MOVE.L    #$12345678, D0
    //MOVE.L    #0, D1
    //MOVE      #0, CCR
    //BSET      D1, D0
    setUp();
    opcode = 0x1c0 | (1 << 9) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    setRegD(1, 0);
    process();
    check("BSET D1, D0 bit 0");

    //MOVE.L    #$12345678, D0
    //MOVE.L    #10, D1
    //MOVE      #0, CCR
    //BSET      D1, D0
    setUp();
    opcode = 0x1c0 | (1 << 9) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    setRegD(1, 10);
    process();
    check("BSET D1, D0 bit 10");

    //MOVE.L    #$12345678, D0
    //MOVE.L    #49, D1
    //MOVE      #0, CCR
    //BSET      D1, D0
    setUp();
    opcode = 0x1c0 | (1 << 9) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    setRegD(1, 49);
    process();
    check("BSET D1, D0 bit 49");

    //MOVE.L    #$3000, A0
    //MOVE.B    #$78, $3000
    //MOVE.L    #50, D1
    //MOVE      #0, CCR
    //BSET      D1, (A0)
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0x1c0 | (1 << 9) | getEA(AR_INDIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(0, 0x3000);
    setRegD(1, 50);
    memoryblock.write(0x3000, 0x78);
    process();
    check("BSET D1, (A0) bit 50");

    //MOVE.L    #$3000, A0
    //MOVE.B    #$78, $3000
    //MOVE.L    #3, D1
    //MOVE      #0, CCR
    //BSET      D1, (A0)
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0x1c0 | (1 << 9) | getEA(AR_INDIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(0, 0x3000);
    setRegD(1, 3);
    memoryblock.write(0x3000, 0x78);
    process();
    check("BSET D1, (A0) bit 3");

    //static

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //BSET      #28, D0
    setUp();
    opcode = 0x8c0 | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(28);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("BSET #28, D0");

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //BSET      #2, D0
    setUp();
    opcode = 0x8c0 | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(2);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("BSET #2, D0");

    //MOVE.B    #$78, $3000
    //MOVE      #0, CCR
    //BSET      #6, ($3000).W
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0x8c0 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(6);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.write(0x3000, 0x78);
    process();
    check("BSET #6, ($3000).W");
}
