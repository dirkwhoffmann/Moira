
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleOri() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("ORI.B #$12, D1");
    oObj->setRegD(1, 0x1234567a);
    oObj->setCycleCount(8);

    oObj = new Results("ORI.L #$ffff0000, ($3000).W");
    oObj->setN();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0xffffffff);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(32);
}

void Tester_68k::testOri() {
    u16 opcode;

    //MOVE.L    #$12345678, D1
    //MOVE      #$0, CCR
    //ORI.B    #$12, D1
    setUp();
    opcode = (0 << 6) | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0x0012);
    addWord(0xffff);
    power();
    setRegD(1, 0x12345678);
    process();
    check("ORI.B #$12, D1");

    //MOVE.L    #$0000ffff, $3000
    //MOVE      #$0, CCR
    //ORI.L    #$ffff0000, ($3000).W
    //MOVE.L    $3000, D1
    setUp();
    opcode = (2 << 6) | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0xffff);
    addWord(0);
    addWord(0x3000);
    addWord(0xffff);
    power();
    memoryblock.writeLong(0x3000, 0x0000ffff);
    process();
    check("ORI.L #$ffff0000, ($3000).W");

}
