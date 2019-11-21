
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleEoriSr() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("EORI.W #$700, SR");
    oObj->setRegA(7, 0x300);
    oObj->setInt(0);
    oObj->setCycleCount(20);

    oObj = new Results("EORI.W #$0, SR");
    oObj->expectPrivilege(true);
    oObj->setRegA(7, 0xfffffffa);
    oObj->setRegIRD(0);
    oObj->setCycleCount(34);

}

void Tester_68k::testEoriSr() {
    u16 opcode;

    //MOVE.L  #$300, A7
    //MOVE    #$2700, SR
    //EORI.W  #$700, SR
    setUp();
    opcode = 0xa7c;
    addWord(opcode);
    addWord(0x700);
    addWord(0xffff);
    power();
    setSR(0x2700);
    setRegA(7, 0x300);
    process();
    check("EORI.W #$700, SR");

    //EORI.W  #$0, SR
    setUp();
    opcode = 0xa7c;
    addWord(opcode);
    addWord(0);
    addWord(0xffff);
    power();
    setSR(0x700);
    process();
    group1exceptions();
    check("EORI.W #$0, SR");

}

