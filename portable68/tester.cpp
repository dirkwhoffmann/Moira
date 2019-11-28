#include "tester.h"
#include "results.h"
#include "exception.h"
#include "helper.h"
#include <stdlib.h>

#include "Moira.h"
extern Moira::CPU *moira;

/*
#include "testcases/add.cpp"
#include "testcases/lea.cpp"
#include "testcases/asl.cpp"
#include "testcases/asr.cpp"
#include "testcases/lsr.cpp"
#include "testcases/lsl.cpp"
#include "testcases/sub.cpp"
#include "testcases/and.cpp"
#include "testcases/clr.cpp"
#include "testcases/nop.cpp"
#include "testcases/ext.cpp"
#include "testcases/dbcc.cpp"
#include "testcases/tst.cpp"
#include "testcases/abcd.cpp"
#include "testcases/nbcd.cpp"
#include "testcases/sbcd.cpp"
#include "testcases/moveq.cpp"
#include "testcases/or.cpp"
#include "testcases/eor.cpp"
#include "testcases/movea.cpp"
#include "testcases/mulu.cpp"
#include "testcases/muls.cpp"
#include "testcases/rol.cpp"
#include "testcases/roxl.cpp"
#include "testcases/ror.cpp"
#include "testcases/roxr.cpp"
#include "testcases/bchg.cpp"
#include "testcases/bclr.cpp"
#include "testcases/bset.cpp"
#include "testcases/btst.cpp"
#include "testcases/neg.cpp"
#include "testcases/negx.cpp"
#include "testcases/not.cpp"
#include "testcases/scc.cpp"
#include "testcases/tas.cpp"
#include "testcases/adda.cpp"
#include "testcases/cmp.cpp"
#include "testcases/cmpa.cpp"
#include "testcases/suba.cpp"
#include "testcases/divu.cpp"
#include "testcases/divs.cpp"
#include "testcases/move.cpp"
#include "testcases/addi.cpp"
#include "testcases/addq.cpp"
#include "testcases/andi.cpp"
#include "testcases/cmpi.cpp"
#include "testcases/eori.cpp"
#include "testcases/ori.cpp"
#include "testcases/subi.cpp"
#include "testcases/subq.cpp"
#include "testcases/bcc.cpp"
#include "testcases/bra.cpp"
#include "testcases/bsr.cpp"
#include "testcases/jmp.cpp"
#include "testcases/jsr.cpp"
#include "testcases/pea.cpp"
#include "testcases/movem.cpp"
#include "testcases/addx.cpp"
#include "testcases/cmpm.cpp"
#include "testcases/subx.cpp"
#include "testcases/andiccr.cpp"
#include "testcases/andisr.cpp"
#include "testcases/chk.cpp"
#include "testcases/eoriccr.cpp"
#include "testcases/oriccr.cpp"
#include "testcases/orisr.cpp"
#include "testcases/eorisr.cpp"
#include "testcases/movefromsr.cpp"
#include "testcases/movetoccr.cpp"
#include "testcases/movetosr.cpp"
#include "testcases/exg.cpp"
#include "testcases/link.cpp"
#include "testcases/moveusp.cpp"
#include "testcases/swap.cpp"
#include "testcases/unlk.cpp"
#include "testcases/movep.cpp"
#include "testcases/trap.cpp"
#include "testcases/trapv.cpp"
#include "testcases/rtr.cpp"
#include "testcases/rts.cpp"
*/

Tester_68k::Tester_68k() : Core_68k()
{
    errorCounter = 0;
    testCounter = 0;

    //samples were generated on amiga
    sampleAdd();
    sampleLea();
    sampleAsl(); //
    sampleAsr();
    sampleLsr();
    sampleLsl();
    sampleSub();
    sampleAnd();
    sampleClr();
    sampleNop();
    sampleExt();
    sampleDbcc();
    sampleTst();
    sampleAbcd();
    sampleNbcd();
    sampleSbcd();
    sampleMoveq();
    sampleOr();
    sampleEor();
    sampleMovea();
    sampleMulu();
    sampleMuls();
    sampleRol();
    sampleRoxl();
    sampleRor();
    sampleRoxr();
    sampleBchg();
    sampleBclr();
    sampleBset();
    sampleBtst();
    sampleNeg();
    sampleNegx();
    sampleNot();
    sampleScc();
    sampleTas();
    sampleAdda();
    sampleCmp();
    sampleCmpa();
    sampleSuba();
    sampleDivu();
    sampleDivs();
    sampleMove();
    sampleAddi();
    sampleAddq();
    sampleAndi();
    sampleCmpi();
    sampleEori();
    sampleOri();
    sampleSubi();
    sampleSubq();
    sampleBcc();
    sampleBra();
    sampleBsr();
    sampleJmp();
    sampleJsr();
    samplePea();
    sampleMovem();
    sampleAddx();
    sampleCmpm();
    sampleSubx();
    sampleAndiCcr();
    sampleAndiSr();
    sampleChk();
    sampleEoriCcr();
    sampleOriCcr();
    sampleOriSr();
    sampleEoriSr();
    sampleMoveFromSr();
    sampleMoveToCcr();
    sampleMoveToSr();
    sampleExg();
    sampleLink();
    sampleMoveUsp();
    sampleSwap();
    sampleUnlk();
    sampleMovep();
    sampleTrap();
    sampleTrapv();
    sampleRtr();
    sampleRts();
}

void Tester_68k::runTestCases() {
    printf("*\n* Testing LEA\n*\n\n");
    testLea();
    printf("*\n* Testing ADD\n*\n\n");
    testAdd();
    printf("*\n* Testing ASL\n*\n\n");
    testAsl();
    testAsr();
    testLsr();
    testLsl();
    testSub();
    testAnd();
    testClr();
    testNop();
    testExt();
    testDbcc();
    testTst();
    testAbcd();
    testNbcd();
    testSbcd();
    testMoveq();
    testOr();
    testEor();
    testMovea();
    testMulu();
    testMuls();
    testRol();
    testRoxl();
    testRor();
    testRoxr();
    testBchg();
    testBclr();
    testBset();
    testBtst();
    testNeg();
    testNegx();
    testNot();
    testScc();
    testTas();
    testAdda();
    testCmp();
    testCmpa();
    testSuba();
    testDivu();
    testDivs();
    testMove();
    testAddi();
    testAddq();
    testAndi();
    testCmpi();
    testEori();
    testOri();
    testSubi();
    testSubq();
    testBcc();
    testBra();
    testBsr();
    testJmp();
    testJsr();
    testPea();
    testMovem();
    testAddx();
    testCmpm();
    testSubx();
    testAndiCcr();
    testAndiSr();
    testChk();
    testEoriCcr();
    testOriCcr();
    testOriSr();
    testEoriSr();
    testMoveFromSr();
    testMoveToCcr();
    testMoveToSr();
    testExg();
    testLink();
    testMoveUsp();
    testSwap();
    testUnlk();
    testMovep();
    testTrap();
    testTrapv();
    testRtr();
    testRts();
}

void Tester_68k::printErrorCounter() {
    cout << endl << "Errors: " + Helper::convertIntToString(errorCounter) << endl << endl;
}

void Tester_68k::setUp() {
    irqSampleCycle = 0;
    illegalOpcode = false;
    privilege = false;
    addressError = false;
    busError = false;
    group2exception = false;
    memoryblock.init();
    cycleCounter = -40;
    adrCounter = 8;
    memoryblock.write(7, 8);
}

u16 Tester_68k::memWordRead(u32 addr) {
    u16 res = memRead(addr) << 8;
    res |= memRead(addr + 1);
    return res;
}
u16 Tester_68k::memWordRead2(u32 addr) {
    u16 res = memRead2(addr) << 8;
    res |= memRead2(addr + 1);
    return res;
}

void Tester_68k::memWordWrite(u32 addr, u16 data) {
    memWrite(addr, data >> 8);
    memWrite(addr + 1, data & 0xff);
}
void Tester_68k::memWordWrite2(u32 addr, u16 data) {
    memWrite2(addr, data >> 8);
    memWrite2(addr + 1, data & 0xff);
}

u8 Tester_68k::memRead(u32 addr) {
    if (memoryblock.isBusError(addr)) {
        raiseBusError(addr);
    }
    return memoryblock.read1(addr);
}
u8 Tester_68k::memRead2(u32 addr) {
    if (memoryblock.isBusError(addr)) {
        raiseBusError(addr);
    }
    return memoryblock.read2(addr);
}

void Tester_68k::memWrite(u32 addr, u8 data) {
    if (memoryblock.isBusError(addr)) {
        raiseBusError(addr);
    }
    memoryblock.write1(addr, data);
}
void Tester_68k::memWrite2(u32 addr, u8 data) {
    if (memoryblock.isBusError(addr)) {
        raiseBusError(addr);
    }
    memoryblock.write2(addr, data);
}

unsigned Tester_68k::getEA(ADM _adm, unsigned reg) {
    if ((_adm >> 3) == 7) {
        return _adm;
    } else {
        return _adm | reg;
    }
}

void Tester_68k::check(string ident) {
    Results* oSampled = Results::getResult(ident);

    if (oSampled == 0) {
        throw Exception(" ident not found: " + ident);
    }

    Results* oCalced = sampleResult();

    if (!Results::compareContent(oSampled, oCalced)) {
        cout << ident + " -> error: expected " + oSampled->getError() + oCalced->getError(true) << endl;
        errorCounter++;
    } else {
        cout << "    " + ident + " -> OK";
    }

    // Compare result with Moira
    comparePost();

    if (testCounter++ > 100) {
        testCounter = 0;
        cout << endl;
        system("PAUSE");
    }
}

Results* Tester_68k::sampleResult() {
    Results* oCalced = new Results("", false);
    for (unsigned i=0; i < 8; i++) {
        oCalced->setRegA(i, getRegA(i));
        oCalced->setRegD(i, getRegD(i));
    }
    oCalced->setRegS(getSR());
    oCalced->setRegIRD(getRegIrd());
    oCalced->setCycleCount(cycleCounter);
    oCalced->setIrqSampleCyclePos(irqSampleCycle);
    oCalced->expectIllegal(illegalOpcode);
    oCalced->expectPrivilege(privilege);
    oCalced->expectAddressError(addressError);
    oCalced->expectBusError(busError);
    oCalced->expectGroup2Exception(group2exception);
    oCalced->setCodeBlock(&memoryblock);
    return oCalced;
}

void Tester_68k::sampleIrq() {
    irqSampleCycle = cycleCounter;
    Core_68k::sampleIrq();
}

void Tester_68k::op_illegal(u16 opcode) {
    illegalOpcode = true;
    Core_68k::op_illegal(opcode);
}

void Tester_68k::setPrivilegeException() {
    privilege = true;
    Core_68k::setPrivilegeException();
}

void Tester_68k::group0exception(u8 type) {
    if (type == ADDRESS_ERROR) {
        addressError = true;
    } else {
        busError = true;
    }
    Core_68k::group0exception(type);
}

void Tester_68k::trapException(u8 vector) {
    group2exception = true;
    Core_68k::trapException(vector);
}

void Tester_68k::power()
{
    Core_68k::power();
    moira->power();
}

void Tester_68k::setRegA(u8 reg, u32 value)
{
    Core_68k::setRegA(reg, value);
    moira->writeA(reg, value);
}

void Tester_68k::setRegD(u8 reg, u32 value)
{
    Core_68k::setRegD(reg, value);
    moira->writeD(reg, value);
}

void Tester_68k::process()
{
    char instr[256];

    // Save register IRD
    u32 ird = reg_ird;

    // Disassemble the instruction
    moira->disassemble(moira->getPC(), instr);

    // Run a pre check
    comparePre();

    // Run portable68000
    Core_68k::process();

    // Run Moira
    moira->process(ird);

    // Compare the result
    // comparePost();
}

void Tester_68k::dump()
{
    u16 pc = reg_pc, mpc = moira->getPC();
    u16 irc = reg_irc, mirc = moira->getIRC();
    u16 ird = reg_ird, mird = moira->getIRD();

    u16 sr = reg_s, msr = moira->getSR();
    bool s = (sr >> 13) & 1, ms = (msr >> 13) & 1;
    bool c = (sr >> 0) & 1, mc = (msr >> 0) & 1;
    bool v = (sr >> 1) & 1, mv = (msr >> 1) & 1;
    bool z = (sr >> 2) & 1, mz = (msr >> 2) & 1;
    bool n = (sr >> 3) & 1, mn = (msr >> 3) & 1;
    bool x = (sr >> 4) & 1, mx = (msr >> 4) & 1;
    int ipl = (sr >> 8) & 7, mipl = (msr >> 8) & 7;

    printf("PC: %x / %x %s\n", pc, mpc, pc != mpc ? "<--" : "");
    printf("IRC: %x / %x %s\n", irc, mirc, irc != mirc ? "<--" : "");
    printf("IRD: %x / %x %s\n", ird, mird, ird != mird ? "<--" : "");
    printf("\n");

    for (int i = 0; i < 8; i++) {
        printf("D%i: %x / %x %s\n", i, getRegD(i), moira->readD(i),
               getRegD(i) != moira->readD(i) ? "<--" : "");
    }
    for (int i = 0; i < 8; i++) {
        printf("A%i: %x / %x %s\n", i, getRegA(i), moira->readA(i),
               getRegA(i) != moira->readA(i) ? "<--" : "");
    }

    printf("\nStatus register:\n");
    printf("S: %d / %d %s\n", s, ms, s != ms ? "<--" : "");
    printf("C: %d / %d %s\n", c, mc, c != mc ? "<--" : "");
    printf("V: %d / %d %s\n", v, mv, v != mv ? "<--" : "");
    printf("Z: %d / %d %s\n", z, mz, z != mz ? "<--" : "");
    printf("N: %d / %d %s\n", n, mn, n != mn ? "<--" : "");
    printf("X: %d / %d %s\n", x, mx, x != mx ? "<--" : "");
    printf("IPL: %d / %d %s\n", ipl, mipl, ipl != mipl ? "<--" : "");
}

bool Tester_68k::compare()
{
    for (int i = 0; i < 8; i++)
        if (getRegD(i) != moira->readD(i)) return false;

    for (int i = 0; i < 8; i++)
        if (getRegA(i) != moira->readA(i)) return false;

    if (getSR() != moira->getSR()) return false;
    
    return true;
}

void Tester_68k::comparePre()
{
    if (!compare()) {
        printf("Mismatch before running command\n");
        dump();
        exit(1);
    }
}

void Tester_68k::comparePost()
{
    if (!compare()) {
        printf("\n\nMismatch after running command\n");
        dump();
        exit(1);
    } else {
        printf(" -> OK\n\n");
    }
}
