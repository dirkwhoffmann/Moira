
#include "tester.h"
#include "results.h"

void Tester_68k::sampleSub() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("sub.b x, D");
    oObj->setRegD(2, 0x9af);
    oObj->setC()->setN()->setX();
    oObj->setCycleCount(8);

    oObj = new Results("sub.b x, D overflow");
    oObj->setRegD(2, 0x88);
    oObj->setC()->setX()->setN()->setV();
    oObj->setCycleCount(8);

    oObj = new Results("sub.b x, D non negative");
    oObj->setRegD(2, 0x74);
    oObj->setC()->setX();
    oObj->setCycleCount(8);

    oObj = new Results("sub.b x, overflow only");
    oObj->setRegD(2, 0xff6e);
    oObj->setV();
    oObj->setCycleCount(8);

    oObj = new Results("sub.b -(A7), D2");
    oObj->setRegD(2, 0x9c19);
    oObj->setRegA(7, 0x2000);
    oObj->setCycleCount(10);

    oObj = new Results("sub.w -(A6), D2");
    oObj->expectAddressError(true);
    oObj->setRegD(2, 0x9c40);
    oObj->setRegA(6, 0x2003);
    oObj->setRegA(7, 0xfffffff2);
    oObj->setRegIRD(0);
    oObj->setCycleCount(60);

    oObj = new Results("sub.w D2, x.W");
    oObj->setRegD(2, 0x2711);
    oSampleMem = new MemoryBlock;
    oSampleMem->writeWord(0x3000, 0x4e8e);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);

    oObj = new Results("sub.l D2, 4(A3)");
    oObj->setRegD(2, 0x45fd5ab4);
    oObj->setRegA(3, 0x3000);
    oSampleMem = new MemoryBlock;
    oSampleMem->writeLong(0x3004, 0xec7cf43f);
    oObj->setCodeBlock(oSampleMem);
    oObj->setC()->setX()->setN();
    oObj->setCycleCount(24);
}

void Tester_68k::testSub() {
    u16 opcode;

    //MOVE.W #$9ae, D2
    //SUB.B #$ff, D2
    setUp();
    opcode = (0x9 << 12) | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xff);
    addWord(0xffff);
    power();
    setRegD(2, 0x9ae);
    process();
    check("sub.b x, D");

    //MOVE.W #$a, D2
    //SUB.B #$82, D2
    setUp();
    opcode = (0x9 << 12) | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x82);
    addWord(0xffff);
    power();
    setRegD(2, 0xa);
    process();
    check("sub.b x, D overflow");

    //MOVE.W #$64, D2
    //SUB.B #$f0, D2
    setUp();
    opcode = (0x9 << 12) | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xf0);
    addWord(0xffff);
    power();
    setRegD(2, 0x64);
    process();
    check("sub.b x, D non negative");

    //MOVE.W #$ff96, D2
    //SUB.B #$28, D2
    setUp();
    opcode = (0x9 << 12) | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x28);
    addWord(0xffff);
    power();
    setRegD(2, 0xff96);
    process();
    check("sub.b x, overflow only");

    //MOVE.L #$9c40, D2
    //MOVE.L #$2002, A7
    //MOVE.W #$2710, $2000
    //SUB.B -(A7), D2
    setUp();
    opcode = (0x9 << 12) | (2 << 9) | (0 << 6) | getEA(AR_INDIRECT_DEC, 7);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(7, 0x2002);
    setRegD(2, 0x9c40);
    memoryblock.writeWord(0x2000, 0x2710);
    process();
    check("sub.b -(A7), D2");

    //MOVE.L #$9c40, D2
    //MOVE.L #$2003, A6
    //MOVE.B #$32, $2001
    //MOVE.B #$32, $2002
    //SUB.W -(A6), D2
    setUp();
    opcode = (0x9 << 12) | (2 << 9) | (1 << 6) | getEA(AR_INDIRECT_DEC, 6);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(6, 0x2003);
    setRegD(2, 0x9c40);
    memoryblock.write(0x2001, 0x32);
    memoryblock.write(0x2002, 0x32);
    process();
    check("sub.w -(A6), D2");

    //MOVE.W #$2711, D2
    //MOVE.W #$759f, $3000
    //SUB.W D2, ($3000).W
    //MOVE.W $3000, D3
    setUp();
    opcode = (0x9 << 12) | (2 << 9) | (5 << 6) | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setRegD(2, 0x2711);
    memoryblock.writeWord(0x3000, 0x759f);
    process();
    check("sub.w D2, x.W");

    //MOVE.L #$45fd5ab4, D2
    //MOVE.L #$327a4ef3, $3004
    //MOVE.L #$3000, A3
    //SUB.L D2, 4(A3)
    //MOVE.L $3004, D4
    setUp();
    opcode = (0x9 << 12) | (2 << 9) | (6 << 6) | getEA(AR_INDIRECT_D16, 3);
    addWord(opcode);
    addWord(4);
    addWord(0xffff);
    power();
    setRegD(2, 0x45fd5ab4);
    setRegA(3, 0x3000);
    memoryblock.writeLong(0x3004, 0x327a4ef3);
    process();
    check("sub.l D2, 4(A3)");

}
