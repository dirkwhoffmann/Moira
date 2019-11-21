
#include "tester.h"
#include "results.h"

void Tester_68k::sampleMuls() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("MULS.W D1, D2");
    oObj->setRegD(1, 0x12345678);
    oObj->setRegD(2, 0x1d34d840);
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(54);

    oObj = new Results("MULS.W D1, D2 alt");
    oObj->setRegD(1, 0x82348678);
    oObj->setRegD(2, 0x3971c188);
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(48);

    oObj = new Results("MULS.W D1, D2 zero");
    oObj->setRegD(1, 0x1);
    oObj->setRegD(2, 0x0);
    oObj->setX()->setZ();
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(42);

    oObj = new Results("MULS.W #$ffff, D2");
    oObj->setRegD(2, 0x1);
    oObj->setX();
    oObj->setIrqSampleCyclePos(6);
    oObj->setCycleCount(44);

    oObj = new Results("MULS.W #$ffff, D2 neg");
    oObj->setRegD(2, 0xf2005001);
    oObj->setX()->setN();
    oObj->setIrqSampleCyclePos(6);
    oObj->setCycleCount(46);

}

void Tester_68k::testMuls() {
    u16 opcode;

    //MOVE.L    #$12345678, D1
    //MOVE.L    #$12345678, D2
    //MOVE      #$0, CCR
    //MULS.W    D1, D2
    setUp();
    opcode = 0xc1c0 | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0x12345678);
    setRegD(2, 0x12345678);
    setCCR(0);
    process();
    check("MULS.W D1, D2");

    //MOVE.L    #$82348678, D1
    //MOVE.L    #$823486ff, D2
    //MOVE      #$0, CCR
    //MULS.W    D1, D2
    setUp();
    opcode = 0xc1c0 | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0x82348678);
    setRegD(2, 0x823486ff);
    setCCR(0);
    process();
    check("MULS.W D1, D2 alt");

    //MOVE.L    #$1, D1
    //MOVE.L    #$0, D2
    //MOVE      #$13, CCR
    //MULS.W    D1, D2
    setUp();
    opcode = 0xc1c0 | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0x1);
    setRegD(2, 0x0);
    setCCR(0x13);
    process();
    check("MULS.W D1, D2 zero");

    //MOVE.L    #$ffff, D2
    //MOVE      #$13, CCR
    //MULS.W    #$ffff, D2
    setUp();
    opcode = 0xc1c0 | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xffff);
    addWord(0xffff);
    power();
    setRegD(2, 0xffff);
    setCCR(0x13);
    process();
    check("MULS.W #$ffff, D2");

    //MOVE.L    #$8fff, D2
    //MOVE      #$13, CCR
    //MULS.W    #$1fff, D2
    setUp();
    opcode = 0xc1c0 | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x1fff);
    addWord(0xffff);
    power();
    setRegD(2, 0x8fff);
    setCCR(0x13);
    process();
    check("MULS.W #$ffff, D2 neg");
}

