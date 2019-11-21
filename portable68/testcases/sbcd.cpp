
#include "tester.h"
#include "results.h"

void Tester_68k::sampleSbcd() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("SBCD D2, D1");
    oObj->setRegD(1, 0x12345611);
    oObj->setRegD(2, 0xf745ff78);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("SBCD D2, D1 zero");
    oObj->setRegD(1, 0x12345600);
    oObj->setRegD(2, 0xf745ffff);
    oObj->setZ();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("SBCD D2, D1 neg");
    oObj->setRegD(1, 0x12345688);
    oObj->setRegD(2, 0xf745ff45);
    oObj->setX()->setN()->setC();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("SBCD D2, D1 overflow");
    oObj->setRegD(1, 0x12345643);
    oObj->setRegD(2, 0xf745ffff);
    oObj->setX()->setV()->setC();
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("SBCD -(A2), -(A1)");
    oObj->setN();
    oObj->setRegA(1, 0x3000);
    oObj->setRegA(2, 0x4000);
    oSampleMem = new MemoryBlock;
    oSampleMem->write(0x3000, 0x82);
    oSampleMem->write(0x4000, 0x19);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(18);
}

void Tester_68k::testSbcd() {
    u16 opcode;

    //MOVE.L    #$12345689, D1
    //MOVE.L    #$f745ff78, D2
    //MOVE #$4, CCR
    //SBCD D2, D1
    setUp();
    opcode = 0x8100 | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x12345689);
    setRegD(2, 0xf745ff78);
    process();
    check("SBCD D2, D1");

    //MOVE.L    #$12345605, D1
    //MOVE.L    #$f745ff05, D2
    //MOVE #$4, CCR
    //SBCD D2, D1
    setUp();
    opcode = 0x8100 | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x123456ff);
    setRegD(2, 0xf745ffff);
    process();
    check("SBCD D2, D1 zero");

    //MOVE.L    #$12345634, D1
    //MOVE.L    #$f745ff45, D2
    //MOVE #$10, CCR
    //SBCD D2, D1
    setUp();
    opcode = 0x8100 | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(1, 0x12345634);
    setRegD(2, 0xf745ff45);
    process();
    check("SBCD D2, D1 neg");

    //MOVE.L    #$12345634, D1
    //MOVE.L    #$f745ff45, D2
    //MOVE #$10, CCR
    //SBCD D2, D1
    setUp();
    opcode = 0x8100 | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(1, 0x123456a9);
    setRegD(2, 0xf745ffff);
    process();
    check("SBCD D2, D1 overflow");

    //MOVE.L    #$3001, A1
    //MOVE.L    #$4001, A2
    //MOVE.B    #$a2, $3000
    //MOVE.B    #$19, $4000
    //MOVE #$10, CCR
    //SBCD -(A2), -(A1)
    //MOVE.B    $3000, D1
    //MOVE.B    $4000, D2
    setUp();
    opcode = 0x8100 | (1 << 9) | (1 << 3) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    memoryblock.write(0x3000, 0xa2);
    memoryblock.write(0x4000, 0x19);
    setCCR(0x10);
    setRegA(1, 0x3001);
    setRegA(2, 0x4001);
    process();
    check("SBCD -(A2), -(A1)");
}
