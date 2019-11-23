
#include "tester.h"
#include "results.h"
//#include "../memoryblock.h"

void Tester_68k::sampleDivs() {
    Results* oObj;
    // MemoryBlock* oSampleMem;

    oObj = new Results("DIVS.W #$1, D1");
    oObj->setX()->setN()->setV();
    oObj->setRegD(1, 0x4768f231);
    oObj->setCycleCount(20);

    oObj = new Results("DIVS.W #$1234, D1");
    oObj->setX()->setN()->setV();
    oObj->setRegD(1, 0x4768f231);
    oObj->setCycleCount(20);

    oObj = new Results("DIVS.W #$eeff, D1");
    oObj->setX()->setN()->setV();
    oObj->setRegD(1, 0x4768f231);
    oObj->setCycleCount(20);

    oObj = new Results("DIVS.W #$eef0, D1");
    oObj->setX()->setN();
    oObj->setRegD(1, 0x026190D3);
    oObj->setCycleCount(138);

    oObj = new Results("DIVS.W #$ffff, D1");
    oObj->setX();
    oObj->setRegD(1, 0x1);
    oObj->setCycleCount(158);

    oObj = new Results("DIVS.W #$3333, D1");
    oObj->setX()->setN();
    oObj->setRegD(1, 0xfffffffb);
    oObj->setCycleCount(158);

    oObj = new Results("DIVS.W #$23, D1");
    oObj->setX();
    oObj->setRegD(1, 0xb03de);
    oObj->setCycleCount(138);

    oObj = new Results("DIVS.W #$8765, D1"); //easy68k
    oObj->setX()->setZ();
    oObj->setRegD(1, 0x650000);
    oObj->setCycleCount(156);

    oObj = new Results("DIVS.W #$ffff, D1 alt");
    oObj->setX()->setN()->setV();
    oObj->setRegD(1, 0x80000000);
    oObj->setCycleCount(158);

    oObj = new Results("DIVS.W #$fffd, D1");
    oObj->setX();
    oObj->setRegD(1, 0x1);
    oObj->setCycleCount(158);

    oObj = new Results("DIVS.W #$7aee, D1");
    oObj->setX()->setN();
    oObj->setRegD(1, 0xc97eb240);
    oObj->setCycleCount(148);

    oObj = new Results("DIVS.W #$7fff, D1");
    oObj->setX();
    oObj->setRegD(1, 0x305e105f);
    oObj->setCycleCount(142);

    oObj = new Results("DIVS.W #$f32, D1");
    oObj->setX();
    oObj->setRegD(1, 0x0bfa00ed);
    oObj->setCycleCount(144);

    oObj = new Results("DIVS.W #$af32, D1");
    oObj->setX()->setN();
    oObj->setRegD(1, 0x39dcffd4);
    oObj->setCycleCount(150);

    oObj = new Results("DIVS.W #0, D1");
    oObj->setX();
    oObj->expectGroup2Exception(true);
    oObj->setRegD(1, 0x1fffffff);
    oObj->setRegA(7, 0xfffffffa);
    oObj->setRegIRD(0);
    oObj->setCycleCount(42);
}

void Tester_68k::testDivs() {
    u16 opcode;

    //MOVE.L    #$4768f231, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$1, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x1);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x4768f231);
    process();
    check("DIVS.W #$1, D1");

    //MOVE.L    #$4768f231, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$1234, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x1234);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x4768f231);
    process();
    check("DIVS.W #$1234, D1");

    //MOVE.L    #$4768f231, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$eeff, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xeeff);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x4768f231);
    process();
    check("DIVS.W #$eeff, D1");

    //MOVE.L    #$0768f231, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$eef0, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xeef0);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x0768f231);
    process();
    check("DIVS.W #$eef0, D1");

    //MOVE.L    #$ffffffff, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$ffff, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xffff);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0xffffffff);
    process();
    check("DIVS.W #$ffff, D1");

    //MOVE.L    #$ffff0000, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$3333, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x3333);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0xffff0000);
    process();
    check("DIVS.W #$3333, D1");

    //MOVE.L    #$8765, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$23, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x23);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x8765);
    process();
    check("DIVS.W #$23, D1");

    //MOVE.L    #$65, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$8765, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x8765);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x65);
    process();
    check("DIVS.W #$8765, D1");

    //MOVE.L    #$80000000, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$ffff, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xffff);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x80000000);
    process();
    check("DIVS.W #$ffff, D1 alt");

    //MOVE.L    #$fffffffd, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$fffd, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xfffd);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0xfffffffd);
    process();
    check("DIVS.W #$fffd, D1");

    //MOVE.L    #$daaa00fe, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$7aee, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x7aee);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0xdaaa00fe);
    process();
    check("DIVS.W #$7aee, D1");

    //MOVE.L    #$82f9fff, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$7fff, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x7fff);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x82f9fff);
    process();
    check("DIVS.W #$7fff, D1");

    //MOVE.L    #$00e1d44, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$f32, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xf32);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x00e1d44);
    process();
    check("DIVS.W #$f32, D1");

    //MOVE.L    #$e1d44, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #$af32, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0xaf32);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0xe1d44);
    process();
    check("DIVS.W #$af32, D1");

    //MOVE.L    #$1fffffff, D1
    //MOVE      #$1f, CCR
    //DIVS.W    #0, D1
    setUp();
    opcode = 0x81c0 | (1 << 9) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0);
    addWord(0xffff);
    power();
    setCCR(0x1f);
    setRegD(1, 0x1fffffff);
    process();
    check("DIVS.W #0, D1");
}
