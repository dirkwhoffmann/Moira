
#include "tester.h"
#include "results.h"
#include "memoryblock.h"

void Tester_68k::sampleUnlk() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("UNLK A6");
    oObj->setRegA(6, 0x4000);
    oObj->setRegA(7, 0x3004);
    oObj->setCycleCount(12);

    oObj = new Results("UNLK A7");
    oObj->setRegA(7, 0x4000);
    oObj->setCycleCount(12);
}

void Tester_68k::testUnlk() {
    u16 opcode;

    //MOVE.L    #$3000, A6
    //MOVE.L    #$4000, $3000
    //UNLK      A6
    setUp();
    opcode = 0x4e58 | 6;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(6, 0x3000);
    memoryblock.writeLong(0x3000, 0x4000);
    process();
    check("UNLK A6");

    //MOVE.L    #$3000, A7
    //MOVE.L    #$4000, $3000
    //UNLK      A7
    setUp();
    opcode = 0x4e58 | 7;
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(7, 0x3000);
    memoryblock.writeLong(0x3000, 0x4000);
    process();
    check("UNLK A7");

}
