#ifndef TESTER_H
#define TESTER_H

#include "68k/68k.h"
#include "memoryblock.h"
// #include "MoiraDelegate.h"

class Results;

class Tester_68k : public Core_68k
{
public:
    
    MemoryBlock memoryblock;

    unsigned adrCounter;

    void sync(u8 cycles) { cycleCounter += cycles; }
    unsigned getUserVector( u8 level ) { return 0 ; }
    void cpuHalted() {}
    void sampleIrq();
    void op_illegal(u16 opcode);
    void setPrivilegeException();
    unsigned irqSampleCycle;
    void group0exception(u8 type);
    void trapException(u8 vector);
    bool illegalOpcode;
    bool privilege;
    bool addressError;
    bool busError;
    bool group2exception;
    unsigned errorCounter;
    unsigned testCounter;

    u8 memRead(u32 addr);
    void memWrite(u32 addr, u8 data);
    u8 memRead2(u32 addr);
    void memWrite2(u32 addr, u8 data);

    u16 memWordRead(u32 addr);
    void memWordWrite(u32 addr, u16 data);
    u16 memWordRead2(u32 addr);
    void memWordWrite2(u32 addr, u16 data);

    void setUp();
    void check(string ident);
    Results* sampleResult();
    unsigned getEA(ADM _adm, unsigned reg = 0);

    void addWord(u16 _word) {
        memoryblock.write(adrCounter++, _word >> 8);
        memoryblock.write(adrCounter++, _word & 0xff);
    }

public:
    Tester_68k();

    void printErrorCounter();
    void printInfo(const char *s) {
        static int i = 0; printf("*\n* %d: Testing %s\n*\n\n", ++i, s);
    }
    void runTestCases();

    void testAdd(); //c
    void sampleAdd();

    void testSub(); //c
    void sampleSub();

    void testLea(); //c
    void sampleLea();

    void testAsl(); //c
    void sampleAsl();

    void testAsr(); //c
    void sampleAsr();

    void testLsr(); //c
    void sampleLsr();

    void testLsl(); //c
    void sampleLsl();

    void testAnd();  //p
    void sampleAnd();

    void testClr(); //c
    void sampleClr();

    void testNop(); //c
    void sampleNop();

    void testExt(); //c
    void sampleExt();

    void testDbcc();
    void sampleDbcc();

    void testTst(); //c
    void sampleTst();

    void testAbcd();
    void sampleAbcd();

    void testNbcd();
    void sampleNbcd();

    void testSbcd();
    void sampleSbcd();

    void testMoveq();
    void sampleMoveq();

    void testOr();
    void sampleOr();

    void testEor();
    void sampleEor();

    void testMovea();
    void sampleMovea();

    void testMulu();
    void sampleMulu();

    void testMuls();
    void sampleMuls();

    void testRol();
    void sampleRol();

    void testRoxl();
    void sampleRoxl();

    void testRor();
    void sampleRor();

    void testRoxr();
    void sampleRoxr();

    void testBchg();
    void sampleBchg();

    void testBclr();
    void sampleBclr();

    void testBset();
    void sampleBset();

    void testBtst();
    void sampleBtst();

    void testNeg();
    void sampleNeg();

    void testNegx();
    void sampleNegx();

    void testNot();
    void sampleNot();

    void testScc();
    void sampleScc();

    void testTas();
    void sampleTas();

    void testAdda();
    void sampleAdda();

    void testCmp();
    void sampleCmp();

    void testCmpa();
    void sampleCmpa();

    void testSuba();
    void sampleSuba();

    void testDivu();
    void sampleDivu();

    void testDivs();
    void sampleDivs();

    void testMove();
    void sampleMove();

    void testAddi();
    void sampleAddi();

    void testAddq();
    void sampleAddq();

    void testAndi();
    void sampleAndi();

    void testCmpi();
    void sampleCmpi();

    void testEori();
    void sampleEori();

    void testOri();
    void sampleOri();

    void testSubi();
    void sampleSubi();

    void testSubq();
    void sampleSubq();

    void testBcc();
    void sampleBcc();

    void testBra();
    void sampleBra();

    void testBsr();
    void sampleBsr();

    void testJmp();
    void sampleJmp();

    void testJsr();
    void sampleJsr();

    void testPea();
    void samplePea();

    void testMovem();
    void sampleMovem();

    void testAddx();
    void sampleAddx();

    void testCmpm();
    void sampleCmpm();

    void testSubx();
    void sampleSubx();

    void testAndiCcr();
    void sampleAndiCcr();

    void testAndiSr();
    void sampleAndiSr();

    void testChk();
    void sampleChk();

    void testEoriCcr();
    void sampleEoriCcr();

    void testOriCcr();
    void sampleOriCcr();

    void testOriSr();
    void sampleOriSr();

    void testEoriSr();
    void sampleEoriSr();

    void testMoveFromSr();
    void sampleMoveFromSr();

    void testMoveToCcr();
    void sampleMoveToCcr();

    void testMoveToSr();
    void sampleMoveToSr();

    void testExg();
    void sampleExg();

    void testLink();
    void sampleLink();

    void testMoveUsp();
    void sampleMoveUsp();

    void testSwap();
    void sampleSwap();

    void testUnlk();
    void sampleUnlk();

    void testMovep();
    void sampleMovep();

    void testTrap();
    void sampleTrap();

    void testTrapv();
    void sampleTrapv();

    void testRtr();
    void sampleRtr();

    void testRts();
    void sampleRts();

    void power();
    void setSR(u16 value);
    void setSSP(u32 value);
    void setCCR(u8 value);
    void setRegA(u8 reg, u32 value);
    void setRegD(u8 reg, u32 value);
    void process();

    void dump();
    bool compare();
    void comparePre();
    void comparePost();
};

#endif // TESTER_H
