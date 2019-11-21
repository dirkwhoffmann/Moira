
#include "tester.h"
#include "results.h"

void Tester_68k::sampleAbcd() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ABCD D2, D1");
    oObj->setX()->setC();
    oObj->setRegD(1, 0x12345658);
    oObj->setRegD(2, 0xf745ff78);
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(6);

    oObj = new Results("ABCD D2, D1 zero");
    oObj->setZ();
    oObj->setRegD(1, 0x12345600);
    oObj->setRegD(2, 0x12345600);
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(6);

    oObj = new Results("ABCD D2, D1 neg");
    oObj->setN();
    oObj->setRegD(1, 0x12345699);
    oObj->setRegD(2, 0x12345654);
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(6);

    oObj = new Results("ABCD D2, D1 withx");
    oObj->setX()->setC();
    oObj->setRegD(1, 0x12345600);
    oObj->setRegD(2, 0x12345654);
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(6);

    oObj = new Results("ABCD D2, D1 overflow");
    oObj->setN()->setV();
    oObj->setRegD(1, 0x12345683);
    oObj->setRegD(2, 0x1234563e);
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(6);

    oObj = new Results("ABCD -(A2), -(A1)");
    oObj->setX()->setC();
    oObj->setRegA(1, 0x3000);
    oObj->setRegA(2, 0x4000);
    oSampleMem = new MemoryBlock;
    oSampleMem->write(0x3000, 0x22);
    oSampleMem->write(0x4000, 0x19);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(18);

    oObj = new Results("ABCD -(A1), -(A1)");
    oObj->setX()->setC();
    oObj->setRegA(1, 0x3000);
    oSampleMem = new MemoryBlock;
    oSampleMem->write(0x3000, 0x22);
    oSampleMem->write(0x3001, 0xa2);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(18);
}


void Tester_68k::testAbcd() {
    u16 opcode;

    //MOVE.L    #$1234567a, D1
    //MOVE.L    #$f745ff78, D2
    //MOVE #$4, CCR
    //ABCD D2, D1
    setUp();
    opcode = 0xc100 | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x1234567a);
    setRegD(2, 0xf745ff78);
    process();
    check("ABCD D2, D1");

    //MOVE.L    #$12345600, D1
    //MOVE.L    #$12345600, D2
    //MOVE #$4, CCR
    //ABCD D2, D1
    setUp();
    opcode = 0xc100 | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x12345600);
    setRegD(2, 0x12345600);
    process();
    check("ABCD D2, D1 zero");

    //MOVE.L    #$12345645, D1
    //MOVE.L    #$12345654, D2
    //MOVE #$4, CCR
    //ABCD D2, D1
    setUp();
    opcode = 0xc100 | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(1, 0x12345645);
    setRegD(2, 0x12345654);
    process();
    check("ABCD D2, D1 neg");

    //MOVE.L    #$12345645, D1
    //MOVE.L    #$12345654, D2
    //MOVE #$10, CCR
    //ABCD D2, D1
    setUp();
    opcode = 0xc100 | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(1, 0x12345645);
    setRegD(2, 0x12345654);
    process();
    check("ABCD D2, D1 withx");

    //MOVE.L    #$1234563e, D1
    //MOVE.L    #$1234563e, D2
    //MOVE #$10, CCR
    //ABCD D2, D1
    setUp();
    opcode = 0xc100 | (1 << 9) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x10);
    setRegD(1, 0x1234563e);
    setRegD(2, 0x1234563e);
    process();
    check("ABCD D2, D1 overflow");

    //MOVE.L    #$3001, A1
    //MOVE.L    #$4001, A2
    //MOVE.B    #$a2, $3000
    //MOVE.B    #$19, $4000
    //MOVE #$10, CCR
    //ABCD -(A2), -(A1)
    //MOVE.B    $3000, D1
    //MOVE.B    $4000, D2
    setUp();
    opcode = 0xc100 | (1 << 9) | (1 << 3) | 2;
    addWord(opcode);
    addWord(0xffff);
    power();
    memoryblock.write(0x3000, 0xa2);
    memoryblock.write(0x4000, 0x19);
    setCCR(0x10);
    setRegA(1, 0x3001);
    setRegA(2, 0x4001);
    process();
    check("ABCD -(A2), -(A1)");

    //MOVE.L    #$3002, A1
    //MOVE.B    #$a2, $3001
    //MOVE.B    #$19, $3000
    //MOVE #$10, CCR
    //ABCD -(A1), -(A1)
    //MOVE.B    $3001, D1
    //MOVE.B    $3000, D2
    setUp();
    opcode = 0xc100 | (1 << 9) | (1 << 3) | 1;
    addWord(opcode);
    addWord(0xffff);
    power();
    memoryblock.write(0x3001, 0xa2);
    memoryblock.write(0x3000, 0x19);
    setCCR(0x10);
    setRegA(1, 0x3002);
    process();
    check("ABCD -(A1), -(A1)");
}
