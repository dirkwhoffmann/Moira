
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleCmpm() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("CMPM.L (A1)+, (A1)+");
    oObj->setRegA(1, 0x3008);
    oObj->setN()->setX()->setC();
    oObj->setCycleCount(20);

    oObj = new Results("CMPM.W (A1)+, (A2)+");
    oObj->setRegA(1, 0x3002);
    oObj->setRegA(2, 0x3004);
    oObj->setZ()->setX();
    oObj->setCycleCount(12);

    oObj = new Results("CMPM.B (A1)+, (A2)+");
    oObj->setRegA(1, 0x3001);
    oObj->setRegA(2, 0x3002);
    oObj->setX()->setN()->setV()->setC();
    oObj->setCycleCount(12);
}

void Tester_68k::testCmpm() {
    u16 opcode;

    //MOVE.L    #$3000, A1
    //MOVE.L    #$70001ff1, $3000
    //MOVE.L    #$10000000, $3004
    //MOVE      #$1f, CCR
    //CMPM.L    (A1)+, (A1)+
    setUp();
    opcode = 0xb108 | (1 << 9) | (1) | (2 << 6);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegA(1, 0x3000);
    memoryblock.writeLong(0x3000, 0x70001ff1);
    memoryblock.writeLong(0x3004, 0x10000000);
    process();
    check("CMPM.L (A1)+, (A1)+");

    //MOVE.L    #$3000, A1
    //MOVE.L    #$3002, A2
    //MOVE.W    #$0000, $3000
    //MOVE.W    #$0000, $3002
    //MOVE      #$1f, CCR
    //CMPM.W    (A1)+, (A2)+
    setUp();
    opcode = 0xb108 | (2 << 9) | (1) | (1 << 6);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegA(1, 0x3000);
    setRegA(2, 0x3002);
    memoryblock.writeWord(0x3000, 0);
    memoryblock.writeWord(0x3002, 0);
    process();
    check("CMPM.W (A1)+, (A2)+");

    //MOVE.L    #$3000, A1
    //MOVE.L    #$3001, A2
    //MOVE.B    #$80, $3000
    //MOVE.B    #$7f, $3001
    //MOVE      #$1f, CCR
    //CMPM.B    (A1)+, (A2)+
    setUp();
    opcode = 0xb108 | (2 << 9) | (1) | (0 << 6);
    addWord(opcode);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegA(1, 0x3000);
    setRegA(2, 0x3001);
    memoryblock.write(0x3000, 0x80);
    memoryblock.write(0x3001, 0x7f);
    process();
    check("CMPM.B (A1)+, (A2)+");
}
