
#include "tester.h"
#include "results.h"
#include "memoryblock.h"

void Tester_68k::sampleCmpa() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("CMPA.L D1, A2");
    oObj->setRegA(2, 0x12345680);
    oObj->setRegD(1, 0x1234567f);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("CMPA.L D1, A2 alt");
    oObj->setRegA(2, 0x12345680);
    oObj->setRegD(1, 0xfd234577);
    oObj->setC();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("CMPA.L D1, A2 alt2");
    oObj->setRegA(2, 0x22563245);
    oObj->setRegD(1, 0x85678943);
    oObj->setC()->setV()->setN();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("CMPA.W D1, A2");
    oObj->setRegA(2, 0x22563245);
    oObj->setRegD(1, 0x85678943);
    oObj->setC();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("CMPA.W D1, A2 alt");
    oObj->setRegA(2, 0xffffffff);
    oObj->setRegD(1, 0x0000ffff);
    oObj->setZ();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);
}

void Tester_68k::testCmpa() {
    u16 opcode;

    //MOVE.L    #$12345680, A2
    //MOVE.L    #$1234567f, D1
    //MOVE      #0, CCR
    //CMPA.L    D1, A2
    setUp();
    opcode = 0xb000 | (7 << 6) | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0x12345680);
    setRegD(1, 0x1234567f);
    process();
    check("CMPA.L D1, A2");

    //MOVE.L    #$12345680, A2
    //MOVE.L    #$fd234577, D1
    //MOVE      #0, CCR
    //CMPA.L    D1, A2
    setUp();
    opcode = 0xb000 | (7 << 6) | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0x12345680);
    setRegD(1, 0xfd234577);
    process();
    check("CMPA.L D1, A2 alt");

    //MOVE.L    #$22563245, A2
    //MOVE.L    #$85678943, D1
    //MOVE      #0, CCR
    //CMPA.L    D1, A2
    setUp();
    opcode = 0xb000 | (7 << 6) | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0x22563245);
    setRegD(1, 0x85678943);
    process();
    check("CMPA.L D1, A2 alt2");

    //MOVE.L    #$22563245, A2
    //MOVE.L    #$85678943, D1
    //MOVE      #0, CCR
    //CMPA.W    D1, A2
    setUp();
    opcode = 0xb000 | (3 << 6) | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0x22563245);
    setRegD(1, 0x85678943);
    process();
    check("CMPA.W D1, A2");

    //MOVE.L    #$ffffffff, A2
    //MOVE.L    #$0000ffff, D1
    //MOVE      #0, CCR
    //CMPA.W    D1, A2
    setUp();
    opcode = 0xb000 | (3 << 6) | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0xffffffff);
    setRegD(1, 0x0000ffff);
    process();
    check("CMPA.W D1, A2 alt");
}
