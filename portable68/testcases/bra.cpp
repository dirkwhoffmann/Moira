
#include "tester.h"
#include "results.h"

void Tester_68k::sampleBra() {
    Results* oObj;

    oObj = new Results("BRA.B GO");
    oObj->setCycleCount(10);
    oObj->setRegIRD(0xffaa);

    oObj = new Results("BRA.W GO");
    oObj->setCycleCount(10);
    oObj->setRegIRD(0x44fc);
}


void Tester_68k::testBra() {
    u16 opcode;

    //MOVE #$0, CCR
    //BRA.B GO
    //MOVE.B    #$6, D1
    //GO:
    setUp();
    opcode = 0x6004;
    addWord(opcode);
    addWord(0x123c);
    addWord(0x6);
    addWord(0xffaa);
    power();
    process();
    check("BRA.B GO");

    //GO:
    //MOVE #$0, CCR
    //BRA.W GO
    setUp();
    opcode = 0x6000;
    addWord(0x44fc);
    addWord(0);
    addWord(opcode);
    addWord(0xfffa);
    memoryblock.write(7, 12);
    power();
    process();
    check("BRA.W GO");
}
