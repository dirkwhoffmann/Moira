
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleCmp() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("CMP.B D1, D2");
    oObj->setRegD(2, 0x12345680);
    oObj->setRegD(1, 0x1234567f);
    oObj->setV();
    oObj->setCycleCount(4);

    oObj = new Results("CMP.B D2, D1");
    oObj->setRegD(2, 0x12345680);
    oObj->setRegD(1, 0x1234567f);
    oObj->setV()->setN()->setC();
    oObj->setCycleCount(4);

    oObj = new Results("CMP.W D2, D1");
    oObj->setRegD(2, 0x12348004);
    oObj->setRegD(1, 0x12347002);
    oObj->setV()->setN()->setC();
    oObj->setCycleCount(4);

    oObj = new Results("CMP.W D2, D1 alt");
    oObj->setRegD(2, 0x66666666);
    oObj->setRegD(1, 0x55555555);
    oObj->setC()->setN();
    oObj->setCycleCount(4);

    oObj = new Results("CMP.L D2, D1");
    oObj->setRegD(2, 0x12348004);
    oObj->setRegD(1, 0x12347002);
    oObj->setN()->setC();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);
}

void Tester_68k::testCmp() {
    u16 opcode;

    //MOVE.L    #$12345680, D2
    //MOVE.L    #$1234567f, D1
    //MOVE      #0, CCR
    //CMP.B     D1, D2
    setUp();
    opcode = 0xb000 | (0 << 6) | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x12345680);
    setRegD(1, 0x1234567f);
    process();
    check("CMP.B D1, D2");

    //MOVE.L    #$12345680, D2
    //MOVE.L    #$1234567f, D1
    //MOVE      #0, CCR
    //CMP.B     D2, D1
    setUp();
    opcode = 0xb000 | (0 << 6) | (1 << 9) | getEA(DR_DIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x12345680);
    setRegD(1, 0x1234567f);
    process();
    check("CMP.B D2, D1");

    //MOVE.L    #$12348004, D2
    //MOVE.L    #$12347002, D1
    //MOVE      #0, CCR
    //CMP.W     D2, D1
    setUp();
    opcode = 0xb000 | (1 << 6) | (1 << 9) | getEA(DR_DIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x12348004);
    setRegD(1, 0x12347002);
    process();
    check("CMP.W D2, D1");

    //MOVE.L    #$66666666, D2
    //MOVE.L    #$55555555, D1
    //MOVE      #0, CCR
    //CMP.W     D2, D1
    setUp();
    opcode = 0xb000 | (1 << 6) | (1 << 9) | getEA(DR_DIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x66666666);
    setRegD(1, 0x55555555);
    process();
    check("CMP.W D2, D1 alt");

    //MOVE.L    #$12348004, D2
    //MOVE.L    #$12347002, D1
    //MOVE      #0, CCR
    //CMP.L     D2, D1
    setUp();
    opcode = 0xb000 | (2 << 6) | (1 << 9) | getEA(DR_DIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x12348004);
    setRegD(1, 0x12347002);
    process();
    check("CMP.L D2, D1");
}

