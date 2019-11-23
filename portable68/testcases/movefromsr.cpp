
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleMoveFromSr() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("MOVE.W SR, D1");
    oObj->setRegS(0x27ff);
    oObj->setRegD(1, 0x271f);
    oObj->setCycleCount(6);
    oObj->setX()->setN()->setV()->setC()->setZ();
    oObj->setIrqSampleCyclePos(2);
}

void Tester_68k::testMoveFromSr() {
    u16 opcode;

    //MOVE  #$ff, CCR
    //MOVE.W SR, D1
    setUp();
    opcode = 0x40c0 | getEA(DR_DIRECT, 1);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0xff);
    process();
    check("MOVE.W SR, D1");

}
