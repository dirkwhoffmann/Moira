#include "results.h"
#include "exception.h"
#include "helper.h"

Results* Results::list[1000];
unsigned Results::counter = 0;

Results::Results(string Ident, bool Add)
{
    if (Ident != "")
        setIdent(Ident);
    init();

    if (Add) {
        Results::add(this);
    }
}

void Results::init() {
    for(unsigned i=0; i < 8; i++) {
        regA[i] = regD[i] = 0;
    }
    memoryblock = 0;
    isIllegal = false;
    isPrivilege = false;
    isAddressError = false;
    isBusError = false;
    isGroup2Exception = false;
    cycles = 0;
    irqSampleCycle = -1;
    regS = 0x2700;
    regIRD = 0xffff;
    TypeNames[0] = "RegA";
    TypeNames[1] = "RegD";
    TypeNames[2] = "RegS";
    TypeNames[3] = "RegIRD";
    TypeNames[4] = "Cycles";
    TypeNames[5] = "Memory";
    TypeNames[6] = "Illegal Opcode";
    TypeNames[7] = "AddressError";
    TypeNames[8] = "Group2Exception";
    TypeNames[9] = "IRQ Sample Cycle";
    TypeNames[10] = "BusError";
    TypeNames[11] = "Privilege Exception";

    error.mes = "";
}

bool Results::compareContent(Results* a, Results* b) {
    if (a->isIllegal != b->isIllegal) {
        a->setError(ILLEGAL, a->isIllegal);
        b->setError(ILLEGAL, b->isIllegal);
        return false;
    }

    if (a->isPrivilege != b->isPrivilege) {
        a->setError(PRIVILEGE, a->isPrivilege);
        b->setError(PRIVILEGE, b->isPrivilege);
        return false;
    }

    if (a->isAddressError != b->isAddressError) {
        a->setError(ADDRESS_ERROR, a->isAddressError);
        b->setError(ADDRESS_ERROR, b->isAddressError);
        return false;
    }

    if (a->isBusError != b->isBusError) {
        a->setError(BUS_ERROR, a->isBusError);
        b->setError(BUS_ERROR, b->isBusError);
        return false;
    }

    if (a->isGroup2Exception != b->isGroup2Exception) {
        a->setError(GROUP_2_EXCEPTION, a->isGroup2Exception);
        b->setError(GROUP_2_EXCEPTION, b->isGroup2Exception);
        return false;
    }

    for(unsigned i=0; i < 8; i++) {
        if (a->regA[i] != b->regA[i]) {
            a->setError(REGA, Helper::convertIntToHexString(a->regA[i]), i);
            b->setError(REGA, Helper::convertIntToHexString(b->regA[i]), i);
            return false;
        }
    }
    for(unsigned i=0; i < 8; i++) {
        if (a->regD[i] != b->regD[i]) {
            a->setError(REGD, Helper::convertIntToHexString(a->regD[i]), i);
            b->setError(REGD, Helper::convertIntToHexString(b->regD[i]), i);
            return false;
        }
    }

    if (a->memoryblock != 0) {
        if (b->memoryblock == 0) {
            a->setError(MEMVAL, "codeblock expected");
            b->setError(MEMVAL, "codeblock missing");
            return false;
        }
        if(!MemoryBlock::compare(a->memoryblock, b->memoryblock)) {
            a->setError(MEMVAL, a->memoryblock->getError());
            b->setError(MEMVAL, b->memoryblock->getError());
            return false;
        }
    }

    if (a->regS != b->regS) {
        a->setError(REGS, Helper::convertIntToHexString(a->regS));
        b->setError(REGS, Helper::convertIntToHexString(b->regS));
        return false;
    }
    if (a->regIRD != b->regIRD) {
        a->setError(REGIRD, Helper::convertIntToHexString(a->regIRD));
        b->setError(REGIRD, Helper::convertIntToHexString(b->regIRD));
        return false;
    }
    if (a->cycles != b->cycles) {
        a->setError(CYCLES, Helper::convertIntToString(a->cycles));
        b->setError(CYCLES, Helper::convertIntToString(b->cycles));
        return false;
    }

    if (a->irqSampleCycle == -1) {
        a->irqSampleCycle = a->cycles - 2;
    }
    if (a->irqSampleCycle != b->irqSampleCycle) {
        a->setError(IRQ_SAMPLE_CYCLE, Helper::convertIntToString(a->irqSampleCycle));
        b->setError(IRQ_SAMPLE_CYCLE, Helper::convertIntToString(b->irqSampleCycle));
        return false;
    }

    return true;
}

string Results::getError(bool valueOnly) {
    if (valueOnly) {
        if (error.type == ILLEGAL || error.type == ADDRESS_ERROR)
            return "";
        return " -> " + error.mes;
    }
    string out = TypeNames[error.type];

    if (error.type == ILLEGAL || error.type == ADDRESS_ERROR) {
        if (!error.active) {
            out = "no " + out;
        }
        return out;
    }

    if (error.type == REGA || error.type == REGD) {
        out += Helper::convertIntToString(error.reg);
    }
    out += " " + error.mes;
    return out;
}

void Results::add(Results* result) {
    if (result->ident == "") {
        return;
    }
    if (Results::getResult(result->ident) != 0) {
        throw Exception("ident " + result->ident + " is already in use");
    }

    list[counter++] = result;
}

Results* Results::getResult(string ident) {
    for(unsigned i = 0; i < counter; i++) {
        if (list[i]->ident == ident) {
            return list[i];
        }
    }
    return 0;
}
