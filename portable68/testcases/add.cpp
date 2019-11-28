
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleAdd() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("add.b x, D");
    oObj->setRegD(2, 0x9ad);
    oObj->setC()->setN()->setX();
    oObj->setCycleCount(8);

    oObj = new Results("add.b x, D overflow");
    oObj->setRegD(2, 0x04);
    oObj->setC()->setX()->setV();
    oObj->setCycleCount(8);

    oObj = new Results("add.b D, x.W");
    oObj->setRegD(2, 0x82);
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3000, 4);
    oObj->setCodeBlock(oSampleMem);
    oObj->setC()->setX()->setV();
    oObj->setCycleCount(16);

    oObj = new Results("add.w D, D");
    oObj->setRegD(2, 2000);
    oObj->setCycleCount(4);

    oObj = new Results("add.l D, (An)+");
    oObj->setRegD(2, 0xb2d05e00);
    oObj->setRegA(2, 8196);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x2000, 0x2a05f200);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
    oObj->setC()->setX();

    oObj = new Results("add.w -(A), D");
    oObj->setRegD(2, 0xFFFF0000);
    oObj->setRegA(2, 8192);
    oObj->setZ();
    oObj->setCycleCount(10);

    oObj = new Results("add.b A, D");
    oObj->expectIllegal(true);
    oObj->setRegIRD(0);
    oObj->setRegA(7, 0xfffffffa);
    oObj->setCycleCount(34);

    oObj = new Results("add.l D1, D2");
    oObj->setRegD(1, 0xfe35aab0);
    oObj->setRegD(2, 0xff5b025c);
    oObj->setN();
    oObj->setIrqSampleCyclePos(2);
    oObj->setCycleCount(8);
}

void Tester_68k::testAdd() {
    u16 opcode;

    //MOVE.W #$9ae, D2
    //ADD.B #$ff, D2
    setUp();
    opcode = (0xd << 12) | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xff);
    addWord(0xffff);
    power();
    setRegD(2, 0x9ae);
    process();
    check("add.b x, D");

    //MOVE.W #$82, D2
    //ADD.B #$82, D2
    setUp();
    opcode = (0xd << 12) | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x82);
    addWord(0xffff);
    power();
    setRegD(2, 0x82);
    process();
    check("add.b x, D overflow");

    //MOVE.B #$82, D2
    //MOVE.B #$82, $3000
    //ADD.B D2, ($3000).W
    //MOVE.B $3000, D3
    setUp();
    opcode = (0xd << 12) | (2 << 9) | (4 << 6) | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setRegD(2, 0x82);
    memoryblock.write(0x3000, 0x82);
    process();
    check("add.b D, x.W");

    //MOVE.W #$3e8, D2
    //ADD.W D2, D2
    setUp();
    opcode = (0xd << 12) | (2 << 9) | (1 << 6)  | getEA(DR_DIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0x3e8);
    process();
    check("add.w D, D");

    //MOVE.L #$b2d05e00, D2
    //MOVE.W #$2000, A2
    //MOVE.L #$77359400, $2000
    //ADD.L D2, (A2)+
    //MOVE.L $2000, D3
    setUp();
    opcode = (0xd << 12) | (2 << 9) | (6 << 6) | getEA(AR_INDIRECT_INC, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(2, 0xb2d05e00);
    setRegA(2, 0x2000);
    memoryblock.writeLong(0x2000, 0x77359400);
    process();
    check("add.l D, (An)+");

    //MOVE.L #$FFFF0000, D2
    //MOVE.W #$2002, A2
    //MOVE.W #0, $2000
    //ADD.W -(A2), D2
    setUp();
    opcode = (0xd << 12) | (2 << 9) | (1 << 6) | getEA(AR_INDIRECT_DEC, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0x2002);
    setRegD(2, 0xFFFF0000);
    memoryblock.writeWord(0x2000, 0);
    process();
    check("add.w -(A), D");

    //MOVE.W #0, A2
    //MOVE.W #0, D2
    //ADD.B A2, D2
    setUp();
    opcode = (0xd << 12) | (2 << 9) | (0 << 6) | getEA(AR_DIRECT, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0);
    setRegD(2, 0);
    process();
    group1exceptions();
    check("add.b A, D");

    //MOVE.L #$fe35aab0, D1
    //MOVE.L #$012557ac, D2
    //ADD.L D1, D2
    setUp();
    opcode = (0xd << 12) | (2 << 9) | (2 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0xfe35aab0);
    setRegD(2, 0x012557ac);
    process();
    check("add.l D1, D2");
}
