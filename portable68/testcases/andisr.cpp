
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleAndiSr() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("ANDI.W #$700, SR");
    oObj->setRegA(7, 0);
    oObj->setInt(7)->resetS();
    oObj->setCycleCount(20);

    oObj = new Results("ANDI.W #$0, SR");
    oObj->expectPrivilege(true);
    oObj->setRegA(7, 0xfffffffa);
    oObj->setRegIRD(0);
    oObj->setCycleCount(34);

}

void Tester_68k::testAndiSr() {
    u16 opcode;

    //MOVE.L  #$300, A7
    //MOVE    #$2700, SR
    //ANDI.W  #$700, SR
    setUp();
    opcode = 0x27c;
    addWord(opcode);
    addWord(0x700);
    addWord(0xffff);
    power();
    setSR(0x2700);
    setRegA(7, 0x300);
    process();
    check("ANDI.W #$700, SR");

    //ANDI.W  #$0, SR
    setUp();
    opcode = 0x27c;
    addWord(opcode);
    addWord(0);
    addWord(0xffff);
    power();
    setSR(0x700);
    process();
    group1exceptions();
    check("ANDI.W #$0, SR");

}
