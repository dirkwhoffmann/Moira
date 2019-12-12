
#include "tester.h"
#include "results.h"

void Tester_68k::sampleMovem() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("movem.l D0-D1, -(A1)");
    oObj->setRegD(0, 0x12345678);
    oObj->setRegD(1, 0x87654321);
    oObj->setRegA(1, 0x2ff8);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x2ff8, 0x12345678);
    oSampleMem->writeLong(0x2ffc, 0x87654321);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(24);

    oObj = new Results("movem.l D0-D1/A1, -(A1)");
    oObj->setRegD(0, 0x12345678);
    oObj->setRegD(1, 0x87654321);
    oObj->setRegA(1, 0x2ff4);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x2ff4, 0x12345678);
    oSampleMem->writeLong(0x2ff8, 0x87654321);
    oSampleMem->writeLong(0x2ffc, 0x3000);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(32);

    oObj = new Results("movem.l D0-D7/A0-A7, -(A4)");
    oObj->setRegD(0, 0x11111111);
    oObj->setRegD(1, 0x22222222);
    oObj->setRegD(2, 0x33333333);
    oObj->setRegD(3, 0x44444444);
    oObj->setRegD(4, 0x55555555);
    oObj->setRegD(5, 0x66666666);
    oObj->setRegD(6, 0x77777777);
    oObj->setRegD(7, 0x88888888);
    oObj->setRegA(0, 0x99999999);
    oObj->setRegA(1, 0xaaaaaaaa);
    oObj->setRegA(2, 0xbbbbbbbb);
    oObj->setRegA(3, 0xcccccccc);
    oObj->setRegA(4, 0x3fc0);
    oObj->setRegA(5, 0xdddddddd);
    oObj->setRegA(6, 0xeeeeeeee);
    oObj->setRegA(7, 0xffffffff);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3ffc, 0xffffffff);
    oSampleMem->writeLong(0x3ff8, 0xeeeeeeee);
    oSampleMem->writeLong(0x3ff4, 0xdddddddd);
    oSampleMem->writeLong(0x3ff0, 0x4000);
    oSampleMem->writeLong(0x3fec, 0xcccccccc);
    oSampleMem->writeLong(0x3fe8, 0xbbbbbbbb);
    oSampleMem->writeLong(0x3fe4, 0xaaaaaaaa);
    oSampleMem->writeLong(0x3fe0, 0x99999999);
    oSampleMem->writeLong(0x3fdc, 0x88888888);
    oSampleMem->writeLong(0x3fd8, 0x77777777);
    oSampleMem->writeLong(0x3fd4, 0x66666666);
    oSampleMem->writeLong(0x3fd0, 0x55555555);
    oSampleMem->writeLong(0x3fcc, 0x44444444);
    oSampleMem->writeLong(0x3fc8, 0x33333333);
    oSampleMem->writeLong(0x3fc4, 0x22222222);
    oSampleMem->writeLong(0x3fc0, 0x11111111);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(136);

    oObj = new Results("MOVEM.W D4, -(A4)");
    oObj->setRegD(0, 0xffffffff);
    oObj->setRegD(4, 0x111a1111);
    oObj->setRegA(4, 0x3ffe);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3ffe, 0x1111);
    oSampleMem->writeWord(0x3ffc, 0x0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(12);

    oObj = new Results("MOVEM.W D4/D0, -(A4)");
    oObj->setRegD(0, 0xffffffff);
    oObj->setRegD(4, 0x11111111);
    oObj->setRegA(4, 0x4000);
    oObj->setRegA(7, 0xfffffff2);
    oObj->setRegIRD(0);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3ffe, 0x1111);
    oSampleMem->writeWord(0x3ffc, 0x0);
    oObj->setCodeBlock(oSampleMem);
    oObj->expectBusError(true);
    oObj->setCycleCount(66);

    oObj = new Results("MOVEM.L (A1)+, D1-D2/A1-A2");
    oObj->setRegD(1, 0x11111111);
    oObj->setRegD(2, 0x22222222);
    oObj->setRegA(1, 0x4010);
    oObj->setRegA(2, 858993459); // 0x333333333);
    oObj->setCycleCount(44);

    oObj = new Results("MOVEM.W (A1)+, D1");
    oObj->setRegD(1, 0xffff8000);
    oObj->setRegA(1, 0x4002);
    oObj->setCycleCount(16);

    oObj = new Results("MOVEM.W (A1), A1/D1-D2");
    oObj->setRegD(1, 0xffff8000);
    oObj->setRegD(2, 0x2222);
    oObj->setRegA(1, 0x3333);
    oObj->setCycleCount(24);

    oObj = new Results("MOVEM.W A1/D1-D2, (A1)");
    oObj->setRegD(1, 0x11111111);
    oObj->setRegD(2, 0x22222222);
    oObj->setRegA(1, 0x4000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x4000, 0x1111);
    oSampleMem->writeWord(0x4002, 0x2222);
    oSampleMem->writeWord(0x4004, 0x4000);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
}

void Tester_68k::testMovem() {
    u16 opcode;

    //MOVE.L    #$12345678, D0
    //MOVE.L    #$87654321, D1
    //MOVE.L    #$3000, A1
    //MOVEM.L   D0-D1, -(A1)
    //MOVE.L    $2ffc, D6
    //MOVE.L    $2ff8, D7
    setUp();
    opcode = 0x4880 | (0 << 10) | (1 << 6) | getEA(AR_INDIRECT_DEC, 1);
    addWord(opcode);
    addWord(0xc000);
    addWord(0xffff);
    power();
    setRegA(1, 0x3000);
    setRegD(0, 0x12345678);
    setRegD(1, 0x87654321);
    process();
    check("movem.l D0-D1, -(A1)");

    //MOVE.L    #$12345678, D0
    //MOVE.L    #$87654321, D1
    //MOVE.L    #$3000, A1
    //MOVEM.L   D0-D1/A1, -(A1)
    //MOVE.L    $2ffc, D5
    //MOVE.L    $2ff8, D6
    //MOVE.L    $2ff4, D7
    setUp();
    opcode = 0x4880 | (0 << 10) | (1 << 6) | getEA(AR_INDIRECT_DEC, 1);
    addWord(opcode);
    addWord(0xc040);
    addWord(0xffff);
    power();
    setRegA(1, 0x3000);
    setRegD(0, 0x12345678);
    setRegD(1, 0x87654321);
    process();
    check("movem.l D0-D1/A1, -(A1)");

    //MOVE.L    #$11111111, D0
    //MOVE.L    #$22222222, D1
    //MOVE.L    #$33333333, D2
    //MOVE.L    #$44444444, D3
    //MOVE.L    #$55555555, D4
    //MOVE.L    #$66666666, D5
    //MOVE.L    #$77777777, D6
    //MOVE.L    #$88888888, D7
    //MOVE.L    #$99999999, A0
    //MOVE.L    #$aaaaaaaa, A1
    //MOVE.L    #$bbbbbbbb, A2
    //MOVE.L    #$cccccccc, A3
    //MOVE.L    #$4000, A4
    //MOVE.L    #$dddddddd, A5
    //MOVE.L    #$eeeeeeee, A6
    //MOVE.L    #$ffffffff, A7
    //MOVEM.L   D0-D1/A1-A7, -(A4)
    //MOVE.L    $3ffc, D0
    //MOVE.L    $3ff8, D1
    //MOVE.L    $3ff4, D2
    //MOVE.L    $3ff0, D3
    //MOVE.L    $3fec, D4
    //MOVE.L    $3fe8, D5
    //MOVE.L    $3fe4, D6
    //MOVE.L    $3fe0, D7
    //MOVE.L    $3fdc, A0
    //MOVE.L    $3fd8, A1
    //MOVE.L    $3fd4, A2
    //MOVE.L    $3fd0, A3
    //MOVE.L    $3fcc, A4
    //MOVE.L    $3fc8, A5
    //MOVE.L    $3fc4, A6
    //MOVE.L    $3fc0, A7
    setUp();
    opcode = 0x4880 | (0 << 10) | (1 << 6) | getEA(AR_INDIRECT_DEC, 4);
    addWord(opcode);
    addWord(0xffff);
    addWord(0xffff);
    power();
    setRegD(0, 0x11111111);
    setRegD(1, 0x22222222);
    setRegD(2, 0x33333333);
    setRegD(3, 0x44444444);
    setRegD(4, 0x55555555);
    setRegD(5, 0x66666666);
    setRegD(6, 0x77777777);
    setRegD(7, 0x88888888);
    setRegA(0, 0x99999999);
    setRegA(1, 0xaaaaaaaa);
    setRegA(2, 0xbbbbbbbb);
    setRegA(3, 0xcccccccc);
    setRegA(4, 0x4000);
    setRegA(5, 0xdddddddd);
    setRegA(6, 0xeeeeeeee);
    setRegA(7, 0xffffffff);
    process();
    check("movem.l D0-D7/A0-A7, -(A4)");

    //MOVE.L    #$111a1111, D4
    //MOVE.L    #$ffffffff, D0
    //MOVE.L    #$4000, A4
    //MOVE.L    #$0, $3ffc
    //MOVEM.W   D4, -(A4)
    //MOVE.W    $3ffe, D0
    //MOVE.W    $3ffc, D1
    setUp();
    opcode = 0x4880 | (0 << 10) | (0 << 6) | getEA(AR_INDIRECT_DEC, 4);
    addWord(opcode);
    addWord(0x0800);
    addWord(0xffff);
    power();
    setRegA(4, 0x4000);
    setRegD(4, 0x111a1111);
    setRegD(0, 0xffffffff);
    memoryblock.writeLong(0x3ffc, 0);
    process();
    check("MOVEM.W D4, -(A4)");

    /*
    //MOVE.L    #$11111111, D4
    //MOVE.L    #$ffffffff, D0
    //MOVE.L    #$4000, A4
    //MOVE.L    #$0, $3ffc
    //MOVEM.W   D4/D0, -(A4)
    //MOVE.W    $3ffe, D0
    //MOVE.W    $3ffc, D1
    setUp();
    opcode = 0x4880 | (0 << 10) | (0 << 6) | getEA(AR_INDIRECT_DEC, 4);
    addWord(opcode);
    addWord(0x8800);
    addWord(0xffff);
    power();
    setRegA(4, 0x4000);
    setRegD(4, 0x11111111);
    setRegD(0, 0xffffffff);
    memoryblock.writeLong(0x3ffc, 0);
    memoryblock.setBusError(0x3ffc);
    process();
    check("MOVEM.W D4/D0, -(A4)");
    */

    //MOVE.L    #$4000, A1
    //MOVE.L    #$11111111, $4000
    //MOVE.L    #$22222222, $4004
    //MOVE.L    #$33333333, $400c
    //MOVEM.L    (A1)+, D1-D2/A1-A2
    setUp();
    opcode = 0x4880 | (1 << 10) | (1 << 6) | getEA(AR_INDIRECT_INC, 1);
    addWord(opcode);
    addWord(0x0606);
    addWord(0xffff);
    power();
    setRegA(1, 0x4000);
    memoryblock.writeLong(0x4000, 0x11111111);
    memoryblock.writeLong(0x4004, 0x22222222);
    memoryblock.writeLong(0x400c, 0x33333333);
    process();
    check("MOVEM.L (A1)+, D1-D2/A1-A2");

    //MOVE.L    #$4000, A1
    //MOVE.W    #$8000, $4000
    //MOVEM.W    (A1)+, D1
    setUp();
    opcode = 0x4880 | (1 << 10) | (0 << 6) | getEA(AR_INDIRECT_INC, 1);
    addWord(opcode);
    addWord(0x0002);
    addWord(0xffff);
    power();
    setRegA(1, 0x4000);
    memoryblock.writeWord(0x4000, 0x8000);
    process();
    check("MOVEM.W (A1)+, D1");

    //MOVE.L    #$4000, A1
    //MOVE.W    #$8000, $4000
    //MOVE.W    #$2222, $4002
    //MOVE.W    #$3333, $4004
    //MOVEM.W   (A1), A1/D1-D2
    setUp();
    opcode = 0x4880 | (1 << 10) | (0 << 6) | getEA(AR_INDIRECT, 1);
    addWord(opcode);
    addWord(0x0206);
    addWord(0xffff);
    power();
    setRegA(1, 0x4000);
    memoryblock.writeWord(0x4000, 0x8000);
    memoryblock.writeWord(0x4002, 0x2222);
    memoryblock.writeWord(0x4004, 0x3333);
    process();
    check("MOVEM.W (A1), A1/D1-D2");

    //MOVE.L    #$4000, A1
    //MOVE.L    #$11111111, D1
    //MOVE.L    #$22222222, D2
    //MOVEM.W   A1/D1-D2, (A1)
    //MOVE.W    $4000, D3
    //MOVE.W    $4002, D4
    //MOVE.W    $4004, D5
    setUp();
    opcode = 0x4880 | (0 << 10) | (0 << 6) | getEA(AR_INDIRECT, 1);
    addWord(opcode);
    addWord(0x0206);
    addWord(0xffff);
    power();
    setRegA(1, 0x4000);
    setRegD(1, 0x11111111);
    setRegD(2, 0x22222222);
    process();
    check("MOVEM.W A1/D1-D2, (A1)");

}
