
#include "tester.h"
#include "results.h"

void Tester_68k::sampleBcc() {
    Results* oObj;

    oObj = new Results("BHI.B GO");
    oObj->setCycleCount(10);
    oObj->setRegIRD(0xffaa);

    oObj = new Results("BLO.B GO");
    oObj->setCycleCount(8);
    oObj->setRegIRD(0x123c);

    oObj = new Results("BHI.W GO");
    oObj->setCycleCount(10);
    oObj->setRegIRD(0xffaa);

    oObj = new Results("BLO.W GO");
    oObj->setCycleCount(12);
    oObj->setRegIRD(0x123c);
}

void Tester_68k::testBcc() {
    u16 opcode;

    //MOVE #$0, CCR
    //BHI.B GO
    //MOVE.B    #$6, D1
    //GO:
    setUp();
    opcode = 0x6204;
    addWord(opcode);
    addWord(0x123c);
    addWord(0x6);
    addWord(0xffaa);
    power();
    process();
    check("BHI.B GO");

    //MOVE #$0, CCR
    //BLO.B GO
    //MOVE.B    #$6, D1
    //GO:
    setUp();
    opcode = 0x6504;
    addWord(opcode);
    addWord(0x123c);
    addWord(0x6);
    addWord(0xffaa);
    power();
    process();
    check("BLO.B GO");

    //MOVE #$0, CCR
    //BHI.W GO
    //MOVE.B    #$6, D1
    //GO:
    setUp();
    opcode = 0x6200;
    addWord(opcode);
    addWord(6);
    addWord(0x123c);
    addWord(0x6);
    addWord(0xffaa);
    power();
    process();
    check("BHI.W GO");

    //MOVE #$0, CCR
    //BLO.W GO
    //MOVE.B    #$6, D1
    //GO:
    setUp();
    opcode = 0x6500;
    addWord(opcode);
    addWord(6);
    addWord(0x123c);
    addWord(0x6);
    addWord(0xffaa);
    power();
    process();
    check("BLO.W GO");
}
