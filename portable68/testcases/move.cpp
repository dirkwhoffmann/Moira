
#include "tester.h"
#include "results.h"
#include "memoryblock.h"

void Tester_68k::sampleMove() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("MOVE.B D1, D2");
    oObj->setRegD(1, 0x12345678);
    oObj->setRegD(2, 0x78);
    oObj->setCycleCount(4);

    oObj = new Results("MOVE.L #$8090fea1, D2");
    oObj->setRegD(2, 0x8090fea1);
    oObj->setN();
    oObj->setCycleCount(12);

    oObj = new Results("MOVE #$0, (A2)");
    oObj->setRegA(2, 0x3000);
    oObj->setZ();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3000, 0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(12);

    oObj = new Results("MOVE.L (A2)+, (A2)+");
    oObj->setRegA(2, 0x3008);
    oObj->setN();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0xaaaabbbb);
    oSampleMem->writeLong(0x3004, 0xaaaabbbb);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);

    oObj = new Results("MOVE.L (A2)+, -(A2)");
    oObj->setRegA(2, 0x3000);
    oObj->setN();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0xaaaabbbb);
    oSampleMem->writeLong(0x3004, 0x0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);

    oObj = new Results("MOVE.L -(A2), 4(A2,D1)");
    oObj->setRegA(2, 0x3000);
    oObj->setRegD(1, 0);
    oObj->setN();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0xaaaabbbb);
    oSampleMem->writeLong(0x3004, 0xaaaabbbb);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(28);

    oObj = new Results("MOVE.W D1, ($3000).L");
    oObj->setRegD(1, 0x5678);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeWord(0x3000, 0x5678);
    oSampleMem->writeWord(0x3002, 0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);

    oObj = new Results("MOVE.L ($3000).L, ($3004).L");
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0xeeee);
    oSampleMem->writeLong(0x3004, 0xeeee);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(36);

}

void Tester_68k::testMove() {
    u16 opcode;

    //MOVE.L    #$12345678, D1
    //MOVE      #$0, CCR
    //MOVE.B    D1, D2
    setUp();
    opcode = (1 << 12) | (2 << 9) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0x12345678);
    process();
    check("MOVE.B D1, D2");

    //MOVE      #$0, CCR
    //MOVE.L    #$8090fea1, D2
    setUp();
    opcode = (2 << 12) | (2 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x8090);
    addWord(0xfea1);
    addWord(0xffff);
    power();
    process();
    check("MOVE.L #$8090fea1, D2");

    //MOVE.L    #$3000, A2
    //MOVE.W    #$1234, $3000
    //MOVE      #$0, CCR
    //MOVE.W    #$0000, (A2)
    //MOVE.W    $3000, D1
    setUp();
    opcode = (3 << 12) | (2 << 9) | (2 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0);
    addWord(0xffff);
    power();
    setRegA(2, 0x3000);
    memoryblock.writeWord(0x3000, 0x1234);
    process();
    check("MOVE #$0, (A2)");

    //MOVE.L    #$3000, A2
    //MOVE.L    #$aaaabbbb, $3000
    //MOVE      #$0, CCR
    //MOVE.L    (A2)+, (A2)+
    //MOVE.L    $3004, D1
    setUp();
    opcode = (2 << 12) | (2 << 9) | (3 << 6) | getEA(AR_INDIRECT_INC, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0x3000);
    memoryblock.writeLong(0x3000, 0xaaaabbbb);
    process();
    check("MOVE.L (A2)+, (A2)+");

    //MOVE.L    #$3000, A2
    //MOVE.L    #$aaaabbbb, $3000
    //MOVE      #$0, CCR
    //MOVE.L    (A2)+, -(A2)
    //MOVE.L    $3000, D1
    setUp();
    opcode = (2 << 12) | (2 << 9) | (4 << 6) | getEA(AR_INDIRECT_INC, 2);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(2, 0x3000);
    memoryblock.writeLong(0x3000, 0xaaaabbbb);
    memoryblock.writeLong(0x3004, 0);
    process();
    check("MOVE.L (A2)+, -(A2)");

    //MOVE.L    #$3004, A2
    //MOVE.L    #$0, D1
    //MOVE.L    #$aaaabbbb, $3000
    //MOVE.L    #$0, $3004
    //MOVE      #$0, CCR
    //MOVE.L    -(A2), 4(A2,D1)
    //MOVE.L    $3004, D2
    setUp();
    opcode = (2 << 12) | (2 << 9) | (6 << 6) | getEA(AR_INDIRECT_DEC, 2);
    addWord(opcode);
    addWord(4);
    addWord(0xffff);
    power();
    setRegA(2, 0x3004);
    setRegD(1, 0);
    memoryblock.writeLong(0x3000, 0xaaaabbbb);
    memoryblock.writeLong(0x3004, 0);
    process();
    check("MOVE.L -(A2), 4(A2,D1)");

    //MOVE.L    #$5678, D1
    //MOVE.L    #$0, $3000
    //MOVE      #$0, CCR
    //MOVE.W    D1, ($3000).L
    //MOVE.W    $3000, D2
    //MOVE.W    $3002, D3
    setUp();
    opcode = (3 << 12) | (1 << 9) | (7 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setRegD(1, 0x5678);
    memoryblock.writeWord(0x3000, 0);
    memoryblock.writeWord(0x3002, 0);
    process();
    check("MOVE.W D1, ($3000).L");

    //MOVE.L    #$eeee, $3000
    //MOVE.L    #$0, $3004
    //MOVE      #$0, CCR
    //MOVE.L    ($3000).L, ($3004).L
    //MOVE.L    $3000, D2
    //MOVE.L    $3004, D3
    setUp();
    opcode = (2 << 12) | (1 << 9) | (7 << 6) | getEA(ABS_LONG);
    addWord(opcode);
    addWord(0);
    addWord(0x3000);
    addWord(0);
    addWord(0x3004);
    addWord(0xffff);
    power();
    memoryblock.writeLong(0x3000, 0xeeee);
    memoryblock.writeLong(0x3004, 0);
    process();
    check("MOVE.L ($3000).L, ($3004).L");
}
