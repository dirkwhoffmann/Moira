
#include "tester.h"
#include "results.h"

void Tester_68k::sampleAnd() {
    Results* oObj;
    MemoryBlock* oSampleMem;

    oObj = new Results("and.b D4, D3");
    oObj->setRegD(3, 0x54ff7844);
    oObj->setRegD(4, 0x9853abcd);
    oObj->setCycleCount(4);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("and.w D4, D3");
    oObj->setRegD(3, 0x54fff844);
    oObj->setRegD(4, 0x9853fbcd);
    oObj->setN();
    oObj->setCycleCount(4);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("and.l D4, D3");
    oObj->setRegD(3, 0x1053f844);
    oObj->setRegD(4, 0x9853fbcd);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(2);

    oObj = new Results("and.b (A4), D3");
    oObj->setRegD(3, 0x54fff800);
    oObj->setRegA(4, 0x3000);
    oObj->setZ();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0053fb00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("and.w (A4), D3");
    oObj->setRegD(3, 0x54ff0052);
    oObj->setRegA(4, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0053fb00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("and.l (A4), D3");
    oObj->setRegD(3, 0x0053f800);
    oObj->setRegA(4, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0053fb00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(14);
    oObj->setIrqSampleCyclePos(10);
//untested
    oObj = new Results("and.b (A4)+, D3");
    oObj->setRegD(3, 0x54fff800);
    oObj->setRegA(4, 0x3001);
    oObj->setZ();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0053fb00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("and.b (A7)+, D3");
    oObj->setRegD(3, 0x54fff800);
    oObj->setRegA(7, 0x3002);
    oObj->setZ();
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0053fb00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("and.w (A4)+, D3");
    oObj->setRegD(3, 0x54ff0052);
    oObj->setRegA(4, 0x3002);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0053fb00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("and.w (A4)+, D3 address error");
    oObj->expectAddressError(true);
    oObj->setRegA(7, 0xfffffff2);
    oObj->setRegA(4, 0x3001);
    oObj->setRegD(3, 0x54fff856);
    oObj->setRegIRD(0);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0053fb00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(58);

    oObj = new Results("and.l (A4)+, D3");
    oObj->setRegD(3, 0x0053f800);
    oObj->setRegA(4, 0x3004);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0053fb00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(14);
    oObj->setIrqSampleCyclePos(10);

    oObj = new Results("and.l -(A4), D3");
    oObj->setRegD(3, 0x0053f800);
    oObj->setRegA(4, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x0053fb00);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(16);
    oObj->setIrqSampleCyclePos(12);

    oObj = new Results("and.l -6(A4), D3");
    oObj->setRegD(3, 0x1053f814);
    oObj->setRegA(4, 0x3006);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x1253fb34);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(18);
    oObj->setIrqSampleCyclePos(14);

    oObj = new Results("and.l 6(A4, D6.W), D3");
    oObj->setRegD(3, 0x1053f814);
    oObj->setRegD(6, 0xfffffffd);
    oObj->setRegA(4, 0x2ffd);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x1253fb34);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
    oObj->setIrqSampleCyclePos(16);

    oObj = new Results("and.l -2(A4, D6.W), D3");
    oObj->setRegD(3, 0x1053f814);
    oObj->setRegD(6, 0xf001fffd);
    oObj->setRegA(4, 0x3005);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x1253fb34);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
    oObj->setIrqSampleCyclePos(16);

    oObj = new Results("and.l 1(A4, D6.L), D3");
    oObj->setRegD(3, 0x1053f814);
    oObj->setRegD(6, 0xffffffff);
    oObj->setRegA(4, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x1253fb34);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);
    oObj->setIrqSampleCyclePos(16);

    oObj = new Results("and.l $3000.w, D3");
    oObj->setRegD(3, 0x1053f814);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x1253fb34);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(18);
    oObj->setIrqSampleCyclePos(14);

    oObj = new Results("and.l $11170.l, D3");
    oObj->setRegD(3, 0x1053f814);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x11170, 0x1253fb34);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(22);
    oObj->setIrqSampleCyclePos(18);

    oObj = new Results("and.l -6(PC), D3");
    oObj->setRegD(3, 0x103c1014);
    oObj->setCycleCount(18);
    oObj->setIrqSampleCyclePos(14);

    oObj = new Results("and.l -10(PC, D1), D3");
    oObj->setRegD(3, 0x103c1014);
    oObj->setRegD(1, 4);
    oObj->setCycleCount(20);
    oObj->setIrqSampleCyclePos(16);

    oObj = new Results("and.b #$34, D3");
    oObj->setRegD(3, 0x54fff814);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("and.w #$1234, D3");
    oObj->setRegD(3, 0x54ff1014);
    oObj->setCycleCount(8);
    oObj->setIrqSampleCyclePos(6);

    oObj = new Results("and.l #$34561234, D3");
    oObj->setRegD(3, 0x14561014);
    oObj->setCycleCount(16);
    oObj->setIrqSampleCyclePos(10);

    oObj = new Results("and.l D3, (A4)");
    oObj->setRegD(3, 0x54fff856);
    oObj->setRegA(4, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x1053f802);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);

    oObj = new Results("and.l D3, (A4)+");
    oObj->setRegD(3, 0x54fff856);
    oObj->setRegA(4, 0x3004);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x1053f802);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(20);

    oObj = new Results("and.l D3, -(A4)");
    oObj->setRegD(3, 0x54fff856);
    oObj->setRegA(4, 0x2ffc);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x2ffc, 0x1053f802);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(22);

    oObj = new Results("and.l D3, 2(A4)");
    oObj->setRegD(3, 0x54fff856);
    oObj->setRegA(4, 0x3000);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3002, 0x1053f802);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(24);

    oObj = new Results("and.l D3, ($3000).w");
    oObj->setRegD(3, 0x54fff856);
    oSampleMem = new MemoryBlock();
    oSampleMem->writeLong(0x3000, 0x1053f802);
    oObj->setCodeBlock(oSampleMem);
    oObj->setCycleCount(24);
}

void Tester_68k::testAnd() {
    u16 opcode;

    //MOVE.L    #$9853abcd, D4
    //MOVE.L    #$54ff7856, D3
    //AND.B     D4, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (0 << 6) | getEA(DR_DIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54ff7856);
    setRegD(4, 0x9853abcd);
    process();
    check("and.b D4, D3");

    //MOVE.L    #$9853fbcd, D4
    //MOVE.L    #$54fff856, D3
    //AND.W     D4, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (1 << 6) | getEA(DR_DIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegD(4, 0x9853fbcd);
    process();
    check("and.w D4, D3");

    //MOVE.L    #$9853fbcd, D4
    //MOVE.L    #$54fff856, D3
    //AND.L     D4, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(DR_DIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegD(4, 0x9853fbcd);
    process();
    check("and.l D4, D3");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$0053fb00, $3000
    //AND.B     (A4), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (0 << 6) | getEA(AR_INDIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3000, 0x0053fb00);
    process();
    check("and.b (A4), D3");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$0053fb00, $3000
    //AND.W     (A4), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (1 << 6) | getEA(AR_INDIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3000, 0x0053fb00);
    process();
    check("and.w (A4), D3");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$0053fb00, $3000
    //AND.L     (A4), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(AR_INDIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3000, 0x0053fb00);
    process();
    check("and.l (A4), D3");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$0053fb00, $3000
    //AND.B     (A4)+, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (0 << 6) | getEA(AR_INDIRECT_INC, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3000, 0x0053fb00);
    process();
    check("and.b (A4)+, D3");

    //MOVE.L    #$3000, A7
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$0053fb00, $3000
    //AND.B     (A7)+, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (0 << 6) | getEA(AR_INDIRECT_INC, 7);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(7, 0x3000);
    memoryblock.writeLong(0x3000, 0x0053fb00);
    process();
    check("and.b (A7)+, D3");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$0053fb00, $3000
    //AND.W     (A4)+, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (1 << 6) | getEA(AR_INDIRECT_INC, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3000, 0x0053fb00);
    process();
    check("and.w (A4)+, D3");

    //MOVE.L    #$3001, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$0053fb00, $3000
    //AND.W     (A4)+, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (1 << 6) | getEA(AR_INDIRECT_INC, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3001);
    memoryblock.writeLong(0x3000, 0x0053fb00);
    process();
    check("and.w (A4)+, D3 address error");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$0053fb00, $3000
    //AND.L     (A4)+, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(AR_INDIRECT_INC, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3000, 0x0053fb00);
    process();
    check("and.l (A4)+, D3");

    //MOVE.L    #$3004, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$0053fb00, $3000
    //AND.L     -(A4), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(AR_INDIRECT_DEC, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3004);
    memoryblock.writeLong(0x3000, 0x0053fb00);
    process();
    check("and.l -(A4), D3");

    //MOVE.L    #$3006, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb34, $3000
    //AND.L     -6(A4), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(AR_INDIRECT_D16, 4);
    addWord(opcode);
    addWord(0xfffa);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3006);
    memoryblock.writeLong(0x3000, 0x1253fb34);
    process();
    check("and.l -6(A4), D3");

    //MOVE.L    #$2ffd, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$fffffffd, D6
    //MOVE.L    #$1253fb34, $3000
    //AND.L     6(A4,D6.W), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(AR_INDIRECT_D8, 4);
    addWord(opcode);
    addWord(0x6006);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegD(6, 0xfffffffd);
    setRegA(4, 0x2ffd);
    memoryblock.writeLong(0x3000, 0x1253fb34);
    process();
    check("and.l 6(A4, D6.W), D3");

    //MOVE.L    #$3005, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$f001fffd, D6
    //MOVE.L    #$1253fb34, $3000
    //AND.L     -2(A4,D6.W), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(AR_INDIRECT_D8, 4);
    addWord(opcode);
    addWord(0x60fe);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegD(6, 0xf001fffd);
    setRegA(4, 0x3005);
    memoryblock.writeLong(0x3000, 0x1253fb34);
    process();
    check("and.l -2(A4, D6.W), D3");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$ffffffff, D6
    //MOVE.L    #$1253fb34, $3000
    //AND.L     1(A4,D6.L), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(AR_INDIRECT_D8, 4);
    addWord(opcode);
    addWord(0x6801);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegD(6, 0xffffffff);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3000, 0x1253fb34);
    process();
    check("and.l 1(A4, D6.L), D3");

    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb34, $3000
    //AND.L     $3000.w, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    memoryblock.writeLong(0x3000, 0x1253fb34);
    process();
    check("and.l $3000.w, D3");

    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb34, $11170
    //AND.L     $11170.w, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(ABS_LONG);
    addWord(opcode);
    addWord(0x1);
    addWord(0x1170);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    memoryblock.writeLong(0x11170, 0x1253fb34);
    process();
    check("and.l $11170.l, D3");

    //MOVE.L    #$54fff856, D3
    //GO: MOVE.W    #$1234, D4
    //AND.L     GO(PC), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(PC_INDIRECT_D16);
    addWord(0x383c);
    addWord(0x1234);
    addWord(opcode);
    addWord(0xfffa);
    addWord(0xffff);
    memoryblock.write(0x7, 12);
    power();
    setRegD(3, 0x54fff856);
    process();
    check("and.l -6(PC), D3");

    //MOVE.L    #$54fff856, D3
    //GO: MOVE.W    #$4, D1
    //MOVE.W    #$1234, D4
    //AND.L     GO(PC, D1), D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(PC_INDIRECT_D8);
    addWord(0x383c);
    addWord(0x1234);
    addWord(opcode);
    addWord(0x10f6);
    addWord(0xffff);
    memoryblock.write(0x7, 12);
    power();
    setRegD(3, 0x54fff856);
    setRegD(1, 4);
    process();
    check("and.l -10(PC, D1), D3");

    //MOVE.L    #$54fff856, D3
    //AND.B     #$34, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (0 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x1234);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    process();
    check("and.b #$34, D3");

    //MOVE.L    #$54fff856, D3
    //AND.W     #$1234, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (1 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x1234);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    process();
    check("and.w #$1234, D3");

    //MOVE.L    #$54fff856, D3
    //AND.L    #$34561234, D3
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (2 << 6) | getEA(IMMEDIATE);
    addWord(opcode);
    addWord(0x3456);
    addWord(0x1234);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    process();
    check("and.l #$34561234, D3");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb03, $3000
    //AND.L     D3, (A4)
    //MOVE.L    $3000, D2
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (6 << 6) | getEA(AR_INDIRECT, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3000, 0x1253fb03);
    process();
    check("and.l D3, (A4)");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb03, $3000
    //AND.L     D3, (A4)+
    //MOVE.L    $3000, D2
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (6 << 6) | getEA(AR_INDIRECT_INC, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3000, 0x1253fb03);
    process();
    check("and.l D3, (A4)+");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb03, $2ffc
    //AND.L     D3, -(A4)
    //MOVE.L    $3000, D2
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (6 << 6) | getEA(AR_INDIRECT_DEC, 4);
    addWord(opcode);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x2ffc, 0x1253fb03);
    process();
    check("and.l D3, -(A4)");

    //MOVE.L    #$3000, A4
    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb03, $3002
    //AND.L     D3, 2(A4)
    //MOVE.L    $3002, D2
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (6 << 6) | getEA(AR_INDIRECT_D16, 4);
    addWord(opcode);
    addWord(2);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    setRegA(4, 0x3000);
    memoryblock.writeLong(0x3002, 0x1253fb03);
    process();
    check("and.l D3, 2(A4)");

    //MOVE.L    #$54fff856, D3
    //MOVE.L    #$1253fb03, $3000
    //AND.L     D3, ($3000).w
    //MOVE.L    $3000, D2
    setUp();
    opcode = (0xc << 12) | (3 << 9) | (6 << 6) | getEA(ABS_SHORT);
    addWord(opcode);
    addWord(0x3000);
    addWord(0xffff);
    power();
    setRegD(3, 0x54fff856);
    memoryblock.writeLong(0x3000, 0x1253fb03);
    process();
    check("and.l D3, ($3000).w");
}
