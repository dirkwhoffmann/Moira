
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleExg() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("EXG D1, D2");
    oObj->setRegD(2, 0x11111111);
    oObj->setRegD(1, 0x22222222);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("EXG A1, A2");
    oObj->setRegA(2, 0x11111111);
    oObj->setRegA(1, 0x22222222);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("EXG A1, D1");
    oObj->setRegA(1, 0x11111111);
    oObj->setRegD(1, 0x22222222);
    oObj->setCycleCount(6);
    oObj->setIrqSampleCyclePos(2);
}

void Tester_68k::testExg() {
    u16 opcode;

    //MOVE.L    #$11111111, D1
    //MOVE.L    #$22222222, D2
    //EXG       D1, D2
    setUp();
    opcode = 0xc100 | (1 << 9) | 2 | (8 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0x11111111);
    setRegD(2, 0x22222222);
    process();
    check("EXG D1, D2");

    //MOVE.L    #$11111111, D1
    //MOVE.L    #$22222222, D2
    //EXG       D1, D2
    setUp();
    opcode = 0xc100 | (1 << 9) | 2 | (9 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegA(1, 0x11111111);
    setRegA(2, 0x22222222);
    process();
    check("EXG A1, A2");

    //MOVE.L    #$11111111, D1
    //MOVE.L    #$22222222, A1
    //EXG       A1, D1
    setUp();
    opcode = 0xc100 | (1 << 9) | 1 | (17 << 3);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(1, 0x11111111);
    setRegA(1, 0x22222222);
    process();
    check("EXG A1, D1");

}
