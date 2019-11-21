#ifndef RESULTS_H
#define RESULTS_H

#include <string>
#include "memoryblock.h"
using namespace std;

class Results
{
public:
    enum Type { REGA = 0, REGD = 1, REGS = 2, REGIRD = 3, CYCLES = 4
                ,MEMVAL = 5, ILLEGAL = 6, ADDRESS_ERROR = 7, GROUP_2_EXCEPTION = 8
                ,IRQ_SAMPLE_CYCLE = 9, BUS_ERROR = 10, PRIVILEGE = 11 };
    string TypeNames[12];

    struct Error {
        Type type;
        unsigned reg;
        bool active;
        string mes;
    } error;

    string ident;
    unsigned regA[8];
    unsigned regD[8];
    unsigned regS;
    unsigned regIRD;
    unsigned cycles;
    int irqSampleCycle;

    bool isIllegal;
    bool isPrivilege;
    bool isAddressError;
    bool isBusError;
    bool isGroup2Exception;

    MemoryBlock* memoryblock;

    Results( string Ident = "", bool Add = true );
    static bool compareContent(Results* a, Results* b);
    string getError(bool valueOnly = false);

    static Results* list[1000];
    static unsigned counter;

    static void add(Results* result);
    static Results* getResult(string ident);

    void setError(Type _type, string mes, unsigned _reg = 0) {
        error.type = _type;
        error.mes = mes;
        error.reg = _reg;
    }

    void setError(Type _type, bool _active) {
        error.type = _type;
        error.active = _active;
    }

    void init();

    void expectPrivilege(bool state) {
        isPrivilege = state;
    }

    void expectIllegal(bool state) {
        isIllegal = state;
    }

    void expectAddressError(bool state) {
        isAddressError = state;
    }

    void expectBusError(bool state) {
        isBusError = state;
    }

    void expectGroup2Exception(bool state) {
        isGroup2Exception = state;
    }

    void setIdent(string _ident) {
        ident = _ident;
    }

    Results* setRegA(unsigned reg, unsigned value) {
        regA[reg] = value;
        return this;
    }

    Results* setRegD(unsigned reg, unsigned value) {
        regD[reg] = value;
        return this;
    }

    Results* setRegS(unsigned value) {
        regS = value;
        return this;
    }

    void setRegIRD(unsigned value) {
        regIRD = value;
    }

    void setCycleCount(unsigned value) {
        cycles = value;
    }

    void setIrqSampleCyclePos(unsigned value) {
        irqSampleCycle = value;
    }

    void setCodeBlock(MemoryBlock* _block) {
        memoryblock = _block;
    }

    Results* setX() {
       regS |= (1 << 4) ;
       return this;
    }
    Results* setN() {
       regS |= (1 << 3);
       return this;
    }
    Results* setZ() {
       regS |= (1 << 2);
       return this;
    }
    Results* setV() {
       regS |= (1 << 1) ;
       return this;
    }
    Results* setC() {
       regS |= (1 << 0) ;
       return this;
    }

    Results* setInt(u8 val) {
        regS &= ~(7 << 8);
        regS |= ((val & 7) << 8) ;
        return this;
    }

    Results* setS() {
        regS |= (1 << 13) ;
        return this;
    }

    Results* resetS() {
        regS &= ~(1 << 13) ;
        return this;
    }

};

#endif // RESULTS_H
