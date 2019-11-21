
#include "tester.h"
#include "results.h"
#include "memoryblock.h"

void Tester_68k::sampleScc() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("SF D0");
    oObj->setRegD(0, 0x12345600);
    oObj->setX();
    oObj->setCycleCount(4);

    oObj = new Results("ST D0");
    oObj->setRegD(0, 0x123456ff);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("SLS D0");
    oObj->setRegD(0, 0x123456ff);
    oObj->setX()->setN()->setC()->setV()->setZ();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("SGT 2(A0)");
    oObj->setRegA(0, 0x3000);
    oObj->setX();
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3002, 0xff);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);

    oObj = new Results("SGT 2(A0) alt");
    oObj->setRegA(0, 0x3000);
    oObj->setX()->setC()->setV()->setN()->setZ();
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3002, 0x00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);
}

void Tester_68k::testScc() {
    u16 opcode;

    //MOVE.L    #$12345678, D0
    //MOVE      #$10, CCR
    //SF       D0
    setUp();
    opcode = 0x50c0 | (1 << 8) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(0, 0x12345678);
    process();
    check("SF D0");

    //MOVE.L    #$12345678, D0
    //MOVE      #$0, CCR
    //ST       D0
    setUp();
    opcode = 0x50c0 | (0 << 8) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(0, 0x12345678);
    process();
    check("ST D0");

    //MOVE.L    #$12345678, D0
    //MOVE      #$1f, CCR
    //SLS       D0
    setUp();
    opcode = 0x50c0 | (3 << 8) | getEA(DR_DIRECT, 0);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x01f);
    setRegD(0, 0x12345678);
    process();
    check("SLS D0");

    //MOVE.B    #$88, $3002
    //MOVE.L    #$3000, A0
    //MOVE      #$10, CCR
    //SGT       2(A0)
    //MOVE.B    $3002, D0
    setUp();
    opcode = 0x50c0 | (0xe << 8) | getEA(AR_INDIRECT_D16, 0);
    addWord(opcode);
    addWord(2);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegA(0, 0x3000);
    memoryblock.write(0x3002, 0x88);
    process();
    check("SGT 2(A0)");

    //MOVE.B    #$88, $3002
    //MOVE.L    #$3000, A0
    //MOVE      #$1f, CCR
    //SGT       2(A0)
    //MOVE.B    $3002, D0
    setUp();
    opcode = 0x50c0 | (0xe << 8) | getEA(AR_INDIRECT_D16, 0);
    addWord(opcode);
    addWord(2);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegA(0, 0x3000);
    memoryblock.write(0x3002, 0x88);
    process();
    check("SGT 2(A0) alt");
}
