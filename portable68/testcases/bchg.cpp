
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleBchg() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("BCHG D1, D0 bit 0");
    oObj->setRegD(1, 0);
    oObj->setRegD(0, 0x12345679);
    oObj->setZ();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("BCHG D1, D0 bit 10");
    oObj->setRegD(1, 10);
    oObj->setRegD(0, 0x12345278);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("BCHG D1, D0 bit 48");
    oObj->setRegD(1, 48);
    oObj->setRegD(0, 0x12355678);
    oObj->setZ();
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("BCHG D1, (A0) bit 48");
    oObj->setRegD(1, 48);
    oObj->setRegA(0, 0x3000);
    oObj->setZ();
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x79);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(12);

    oObj = new Results("BCHG D1, (A0) bit 7");
    oObj->setRegD(1, 7);
    oObj->setRegA(0, 0x3000);
    oObj->setZ();
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0xf8);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(12);

    oObj = new Results("BCHG #31, D0");
    oObj->setRegD(0, 0x92345678);
    oObj->setZ();
    oObj->setCycleCount(12);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("BCHG #4, D0");
    oObj->setRegD(0, 0x12345668);
    oObj->setCycleCount(10);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("BCHG #6, ($3000).W");
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 0x38);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
}

void Tester_68k::testBchg() {
    u16 opcode;

    //MOVE.L    #$12345678, D0
    //MOVE.L    #0, D1
    //MOVE      #0, CCR
    //BCHG      D1, D0
    setUp();
    opcode = 0x140 | (1 << 9) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    setRegD(1, 0);
    process();
    check("BCHG D1, D0 bit 0");

    //MOVE.L    #$12345678, D0
    //MOVE.L    #10, D1
    //MOVE      #0, CCR
    //BCHG      D1, D0
    setUp();
    opcode = 0x140 | (1 << 9) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    setRegD(1, 10);
    process();
    check("BCHG D1, D0 bit 10");

    //MOVE.L    #$12345678, D0
    //MOVE.L    #48, D1
    //MOVE      #0, CCR
    //BCHG      D1, D0
    setUp();
    opcode = 0x140 | (1 << 9) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    setRegD(1, 48);
    process();
    check("BCHG D1, D0 bit 48");

    //MOVE.L    #$3000, A0
    //MOVE.B    #$78, $3000
    //MOVE.L    #48, D1
    //MOVE      #0, CCR
    //BCHG      D1, (A0)
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0x140 | (1 << 9) | getEA(AR_INDIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(0, 0x3000);
    setRegD(1, 48);
    memoryblock.write(0x3000, 0x78);
    process();
    check("BCHG D1, (A0) bit 48");

    //MOVE.L    #$3000, A0
    //MOVE.B    #$78, $3000
    //MOVE.L    #7, D1
    //MOVE      #0, CCR
    //BCHG      D1, (A0)
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0x140 | (1 << 9) | getEA(AR_INDIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(0, 0x3000);
    setRegD(1, 7);
    memoryblock.write(0x3000, 0x78);
    process();
    check("BCHG D1, (A0) bit 7");

    //static

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //BCHG      #31, D0
    setUp();
    opcode = 0x840 | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(31);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("BCHG #31, D0");

    //MOVE.L    #$12345678, D0
    //MOVE      #0, CCR
    //BCHG      #4, D0
    setUp();
    opcode = 0x840 | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(4);
    addWord(0xffff);
    power();
    setRegD(0, 0x12345678);
    process();
    check("BCHG #4, D0");

    //MOVE.B    #$78, $3000
    //MOVE      #0, CCR
    //BCHG      #6, ($3000).W
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0x840 | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(6);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.write(0x3000, 0x78);
    process();
    check("BCHG #6, ($3000).W");
}
