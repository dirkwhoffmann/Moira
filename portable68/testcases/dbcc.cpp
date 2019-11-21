
#include "tester.h"
#include "results.h"

void Tester_68k::sampleDbcc() {
    Results* oObj;

    oObj = new Results("dbt D2, GO");
    oObj->setRegD(2, 1);
    oObj->setCycleCount(12);

    oObj = new Results("dbf D2, GO");
    oObj->setRegD(2, 0);
    oObj->setCycleCount(10);

    oObj = new Results("dbhi D2, GO");
    oObj->setRegD(2, 1);
    oObj->setCycleCount(12);

    oObj = new Results("dbhi D2, GO alt");
    oObj->setRegD(2, 0);
    oObj->setC();
    oObj->setCycleCount(10);

    oObj = new Results("dbhi D2, GO alt2");
    oObj->setRegD(2, 0);
    oObj->setZ();
    oObj->setCycleCount(10);

    oObj = new Results("dbls D2, GO");
    oObj->setRegD(2, 1);
    oObj->setZ()->setC();
    oObj->setCycleCount(12);

    oObj = new Results("dbls D2, GO alt");
    oObj->setRegD(2, 1);
    oObj->setC();
    oObj->setCycleCount(12);

    oObj = new Results("dbls D2, GO alt2");
    oObj->setRegD(2, 1);
    oObj->setZ();
    oObj->setCycleCount(12);

    oObj = new Results("dbls D2, GO alt3");
    oObj->setRegD(2, 0);
    oObj->setCycleCount(10);

    oObj = new Results("dbcc D2, GO");
    oObj->setRegD(2, 0);
    oObj->setC();
    oObj->setCycleCount(10);

    oObj = new Results("dbcc D2, GO alt");
    oObj->setRegD(2, 1);
    oObj->setCycleCount(12);

    oObj = new Results("dbcs D2, GO");
    oObj->setRegD(2, 0);
    oObj->setCycleCount(10);

    oObj = new Results("dbcs D2, GO alt");
    oObj->setC();
    oObj->setRegD(2, 1);
    oObj->setCycleCount(12);

    oObj = new Results("dbne D2, GO");
    oObj->setRegD(2, 1);
    oObj->setCycleCount(12);

    oObj = new Results("dbne D2, GO alt");
    oObj->setRegD(2, 0);
    oObj->setZ();
    oObj->setCycleCount(10);

    oObj = new Results("dbeq D2, GO");
    oObj->setRegD(2, 0);
    oObj->setCycleCount(10);

    oObj = new Results("dbeq D2, GO alt");
    oObj->setRegD(2, 1);
    oObj->setZ();
    oObj->setCycleCount(12);

    oObj = new Results("dbvc D2, GO");
    oObj->setRegD(2, 1);
    oObj->setCycleCount(12);

    oObj = new Results("dbvc D2, GO alt");
    oObj->setRegD(2, 0);
    oObj->setV();
    oObj->setCycleCount(10);

    oObj = new Results("dbvs D2, GO");
    oObj->setRegD(2, 0);
    oObj->setCycleCount(10);

    oObj = new Results("dbvs D2, GO alt");
    oObj->setRegD(2, 1);
    oObj->setV();
    oObj->setCycleCount(12);

    oObj = new Results("dbpl D2, GO");
    oObj->setRegD(2, 1);
    oObj->setCycleCount(12);

    oObj = new Results("dbpl D2, GO alt");
    oObj->setRegD(2, 0);
    oObj->setN();
    oObj->setCycleCount(10);

    oObj = new Results("dbmi D2, GO");
    oObj->setRegD(2, 0);
    oObj->setCycleCount(10);

    oObj = new Results("dbmi D2, GO alt");
    oObj->setRegD(2, 1);
    oObj->setN();
    oObj->setCycleCount(12);

    oObj = new Results("dbge D2, GO");
    oObj->setRegD(2, 1);
    oObj->setN()->setV();
    oObj->setCycleCount(12);

    oObj = new Results("dbge D2, GO alt");
    oObj->setRegD(2, 1);
    oObj->setCycleCount(12);

    oObj = new Results("dbge D2, GO alt2");
    oObj->setRegD(2, 0);
    oObj->setN();
    oObj->setCycleCount(10);

    oObj = new Results("dbge D2, GO alt3");
    oObj->setRegD(2, 0);
    oObj->setV();
    oObj->setCycleCount(10);

    oObj = new Results("dblt D2, GO");
    oObj->setRegD(2, 0);
    oObj->setN()->setV();
    oObj->setCycleCount(10);

    oObj = new Results("dblt D2, GO alt");
    oObj->setRegD(2, 0);
    oObj->setCycleCount(10);

    oObj = new Results("dblt D2, GO alt2");
    oObj->setRegD(2, 1);
    oObj->setN();
    oObj->setCycleCount(12);

    oObj = new Results("dblt D2, GO alt3");
    oObj->setRegD(2, 1);
    oObj->setV();
    oObj->setCycleCount(12);

    oObj = new Results("dbgt D2, GO");
    oObj->setRegD(2, 1);
    oObj->setCycleCount(12);

    oObj = new Results("dbgt D2, GO alt");
    oObj->setRegD(2, 0);
    oObj->setZ()->setN()->setV();
    oObj->setCycleCount(10);

    oObj = new Results("dbgt D2, GO alt2");
    oObj->setRegD(2, 1);
    oObj->setN()->setV();
    oObj->setCycleCount(12);

    oObj = new Results("dbgt D2, GO alt3");
    oObj->setRegD(2, 0);
    oObj->setZ();
    oObj->setCycleCount(10);

    oObj = new Results("dble D2, GO");
    oObj->setRegD(2, 0);
    oObj->setCycleCount(10);

    oObj = new Results("dble D2, GO alt");
    oObj->setRegD(2, 1);
    oObj->setZ();
    oObj->setCycleCount(12);

    oObj = new Results("dble D2, GO alt2");
    oObj->setRegD(2, 1);
    oObj->setN();
    oObj->setCycleCount(12);

    oObj = new Results("dble D2, GO alt3");
    oObj->setRegD(2, 0);
    oObj->setN()->setV();
    oObj->setCycleCount(10);

    oObj = new Results("dbf D2, GO alt");
    oObj->setRegD(2, 0xffffffff);
    oObj->setCycleCount(14);

    oObj = new Results("dbf D2, GO alt2");
    oObj->setRegD(2, 0x2222fffe);
    oObj->setCycleCount(10);

    oObj = new Results("dbf D2, GO alt3");
    oObj->setRegD(2, 0x2222fffe);
    oObj->setRegIRD(0x44fc);
    oObj->setCycleCount(10);
}

void Tester_68k::testDbcc() {
    u16 opcode;

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBT D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbt D2, GO");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBF D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (1 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbf D2, GO");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBHI D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (2 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbhi D2, GO");

    //MOVE.L #1, D2
    //MOVE #1, CCR
    //DBHI D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (2 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x01);
    setRegD(2, 1);
    process();
    check("dbhi D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #4, CCR
    //DBHI D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (2 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x04);
    setRegD(2, 1);
    process();
    check("dbhi D2, GO alt2");

    //MOVE.L #1, D2
    //MOVE #5, CCR
    //DBLS D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (3 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x05);
    setRegD(2, 1);
    process();
    check("dbls D2, GO");

    //MOVE.L #1, D2
    //MOVE #1, CCR
    //DBLS D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (3 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x01);
    setRegD(2, 1);
    process();
    check("dbls D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #4, CCR
    //DBLS D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (3 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x04);
    setRegD(2, 1);
    process();
    check("dbls D2, GO alt2");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBLS D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (3 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbls D2, GO alt3");

    //MOVE.L #1, D2
    //MOVE #1, CCR
    //DBCC D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (4 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x1);
    setRegD(2, 1);
    process();
    check("dbcc D2, GO");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBCC D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (4 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbcc D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBCS D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (5 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbcs D2, GO");

    //MOVE.L #1, D2
    //MOVE #1, CCR
    //DBCS D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (5 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x1);
    setRegD(2, 1);
    process();
    check("dbcs D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBNE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (6 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbne D2, GO");

    //MOVE.L #1, D2
    //MOVE #4, CCR
    //DBNE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (6 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x4);
    setRegD(2, 1);
    process();
    check("dbne D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBEQ D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (7 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbeq D2, GO");

    //MOVE.L #1, D2
    //MOVE #4, CCR
    //DBEQ D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (7 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(2, 1);
    process();
    check("dbeq D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBVC D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (8 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbvc D2, GO");

    //MOVE.L #1, D2
    //MOVE #2, CCR
    //DBVC D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (8 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x2);
    setRegD(2, 1);
    process();
    check("dbvc D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBVS D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (9 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbvs D2, GO");

    //MOVE.L #1, D2
    //MOVE #2, CCR
    //DBVS D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (9 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x2);
    setRegD(2, 1);
    process();
    check("dbvs D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBPL D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xa << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbpl D2, GO");

    //MOVE.L #1, D2
    //MOVE #8, CCR
    //DBPL D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xa << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x8);
    setRegD(2, 1);
    process();
    check("dbpl D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #0, CCR
    //DBMI D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xb << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbmi D2, GO");

    //MOVE.L #1, D2
    //MOVE #8, CCR
    //DBMI D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xb << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x8);
    setRegD(2, 1);
    process();
    check("dbmi D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #$a, CCR
    //DBGE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xc << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0xa);
    setRegD(2, 1);
    process();
    check("dbge D2, GO");

    //MOVE.L #1, D2
    //MOVE #$0, CCR
    //DBGE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xc << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbge D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #$8, CCR
    //DBGE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xc << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x8);
    setRegD(2, 1);
    process();
    check("dbge D2, GO alt2");

    //MOVE.L #1, D2
    //MOVE #$2, CCR
    //DBGE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xc << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x2);
    setRegD(2, 1);
    process();
    check("dbge D2, GO alt3");

    //MOVE.L #1, D2
    //MOVE #$a, CCR
    //DBLT D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xd << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0xa);
    setRegD(2, 1);
    process();
    check("dblt D2, GO");

    //MOVE.L #1, D2
    //MOVE #$0, CCR
    //DBLT D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xd << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dblt D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #$8, CCR
    //DBLT D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xd << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x8);
    setRegD(2, 1);
    process();
    check("dblt D2, GO alt2");

    //MOVE.L #1, D2
    //MOVE #$2, CCR
    //DBLT D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xd << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x2);
    setRegD(2, 1);
    process();
    check("dblt D2, GO alt3");

    //MOVE.L #1, D2
    //MOVE #$0, CCR
    //DBGT D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xe << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dbgt D2, GO");

    //MOVE.L #1, D2
    //MOVE #$e, CCR
    //DBGT D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xe << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0xe);
    setRegD(2, 1);
    process();
    check("dbgt D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #$a, CCR
    //DBGT D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xe << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0xa);
    setRegD(2, 1);
    process();
    check("dbgt D2, GO alt2");

    //MOVE.L #1, D2
    //MOVE #$0, CCR
    //DBLE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xf << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x0);
    setRegD(2, 1);
    process();
    check("dble D2, GO");

    //MOVE.L #1, D2
    //MOVE #$4, CCR
    //DBLE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xf << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x4);
    setRegD(2, 1);
    process();
    check("dble D2, GO alt");

    //MOVE.L #1, D2
    //MOVE #$8, CCR
    //DBLE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xf << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x8);
    setRegD(2, 1);
    process();
    check("dble D2, GO alt2");

    //MOVE.L #1, D2
    //MOVE #$a, CCR
    //DBLE D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (0xf << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0xa);
    setRegD(2, 1);
    process();
    check("dble D2, GO alt3");

    //MOVE.L #$ffff0000, D2
    //MOVE #0, CCR
    //DBF D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (1 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    setCCR(0x0);
    setRegD(2, 0xffff0000);
    process();
    check("dbf D2, GO alt");

    //MOVE.L #$2222FFFF, D2
    //MOVE #0, CCR
    //DBF D2, GO
    //MOVE.L #0, D4
    //GO:
    setUp();
    opcode = 0x50c8 | (1 << 8) | 2;
    addWord(opcode);
    addWord(0x8);
    addWord(0xffff);
    power();
    memoryblock.writeWord(0x12, 0xffff);
    setCCR(0x0);
    setRegD(2, 0x2222FFFF);
    process();
    check("dbf D2, GO alt2");

    //MOVE.L #$2222FFFF, D2
    //GO:
    //MOVE #0, CCR
    //DBF D2, GO
    //MOVE.L #0, D4
    setUp();
    addWord(0x44fc);
    addWord(0x0);
    opcode = 0x50c8 | (1 << 8) | 2;
    addWord(opcode);
    addWord(0xfffa);
    addWord(0xffff);
    memoryblock.write(7, 12);
    power();
    setRegD(2, 0x2222FFFF);
    process();
    check("dbf D2, GO alt3");
}
