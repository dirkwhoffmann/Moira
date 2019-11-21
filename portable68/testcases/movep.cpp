
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleMovep() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("MOVEP.W 4(A6), D1");
    oObj->setRegA(6, 0x3000);
    oObj->setRegD(1, 0x1234);
    oObj->setCycleCount(16);

    oObj = new Results("MOVEP.L 2(A6), D1");
    oObj->setRegA(6, 0x3000);
    oObj->setRegD(1, 0x12345678);
    oObj->setCycleCount(24);

    oObj = new Results("MOVEP.W D1, 2(A6)");
    oObj->setRegA(6, 0x3000);
    oObj->setRegD(1, 0x12345678);
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3002, 0x56);
    oSampleMem->write(0x3004, 0x78);
    oSampleMem->write(0x3003, 0);
    oSampleMem->write(0x3005, 0);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);

    oObj = new Results("MOVEP.L D1, 2(A6)");
    oObj->setRegA(6, 0x3000);
    oObj->setRegD(1, 0x12345678);
    oSampleMem = new MemoryBlock();
    oSampleMem->write(0x3002, 0x12);
    oSampleMem->write(0x3004, 0x34);
    oSampleMem->write(0x3006, 0x56);
    oSampleMem->write(0x3008, 0x78);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(24);
}

void Tester_68k::testMovep() {
    u16 opcode;

    //MOVE.L    #0, $3000
    //MOVE.L    #0, $3004
    //MOVE.L    #$3000, A6
    //MOVE.B    #$12, $3004
    //MOVE.B    #$34, $3006
    //MOVEP.W   4(A6), D1
    setUp();
    opcode = 8 | 6 | (1<<9) | (4<<6);
    addWord(opcode);
    addWord(4);
    addWord(0xffff);
    power();
    setRegA(6, 0x3000);
    memoryblock.write(0x3004, 0x12);
    memoryblock.write(0x3006, 0x34);
    process();
    check("MOVEP.W 4(A6), D1");

    //MOVE.L    #0, $3000
    //MOVE.L    #0, $3004
    //MOVE.L    #$3000, A6
    //MOVE.B    #$12, $3002
    //MOVE.B    #$34, $3004
    //MOVE.B    #$56, $3006
    //MOVE.B    #$78, $3008
    //MOVEP.W   2(A6), D1
    setUp();
    opcode = 8 | 6 | (1<<9) | (5<<6);
    addWord(opcode);
    addWord(2);
    addWord(0xffff);
    power();
    setRegA(6, 0x3000);
    memoryblock.write(0x3002, 0x12);
    memoryblock.write(0x3004, 0x34);
    memoryblock.write(0x3006, 0x56);
    memoryblock.write(0x3008, 0x78);
    process();
    check("MOVEP.L 2(A6), D1");

    //MOVE.L    #0, $3002
    //MOVE.L    #0, $3006
    //MOVE.L    #$3000, A6
    //MOVE.L    #$12345678, D1
    //MOVEP.W   D1, 2(A6)
    //MOVE.B    $3002, D2
    //MOVE.B    $3004, D3
    //MOVE.B    $3003, D4
    //MOVE.B    $3005, D5
    setUp();
    opcode = 8 | 6 | (1<<9) | (6<<6);
    addWord(opcode);
    addWord(2);
    addWord(0xffff);
    power();
    setRegA(6, 0x3000);
    setRegD(1, 0x12345678);
    memoryblock.writeLong(0x3002, 0);
    memoryblock.writeLong(0x3006, 0);
    process();
    check("MOVEP.W D1, 2(A6)");

    //MOVE.L    #0, $3002
    //MOVE.L    #0, $3006
    //MOVE.L    #$3000, A6
    //MOVE.L    #$12345678, D1
    //MOVEP.L   D1, 2(A6)
    //MOVE.B    $3002, D2
    //MOVE.B    $3004, D3
    //MOVE.B    $3006, D4
    //MOVE.B    $3008, D5
    setUp();
    opcode = 8 | 6 | (1<<9) | (7<<6);
    addWord(opcode);
    addWord(2);
    addWord(0xffff);
    power();
    setRegA(6, 0x3000);
    setRegD(1, 0x12345678);
    memoryblock.writeLong(0x3002, 0);
    memoryblock.writeLong(0x3006, 0);
    process();
    check("MOVEP.L D1, 2(A6)");


}
