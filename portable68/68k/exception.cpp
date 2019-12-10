#include <stdio.h>
#include "68k.h"

void Core_68k::group1exceptions() {
    status_code._instruction = false;
    if (trace) {
        printf("group1exceptions::trace\n");
        traceException();
    }

    if ( ( irqSamplingLevel == 7 ) || ( irqSamplingLevel > reg_s.intr ) ) {
        if ( irqSamplingLevel == 7 ) {
            printf("group1exceptions::IRQ = 7\n");
        }
        if ( irqSamplingLevel > reg_s.intr ) {
            printf("group1exceptions:: %d > %d\n", irqSamplingLevel, reg_s.intr);
        }
        interruptException( irqSamplingLevel );
    }

    if (illegalMode != NONE) {
        // printf("group1exceptions::illegalMode == %d\n", illegalMode);
        illegalException(illegalMode);
    }
    status_code._instruction = true;
}

void Core_68k::setInterrupt(u8 level) {
    irqPendingLevel = level & 7;
}

/*
 * interrupts are not sampled at opcode edge, if so it would generate extra cycles.
 * interrupts are sampled during last bus cycle
 * too late irqs are not serviced till end of following opcode
 */
void Core_68k::sampleIrq() {
    irqSamplingLevel = irqPendingLevel;
}

void Core_68k::interruptException(u8 level) {
    irqSamplingLevel = 0;
    stop = false;

    u16 SR = reg_s;
    reg_s.intr = level;
    switchToSupervisor();
    reg_s.trace = 0;

    sync(6);
    reg_a[7] -= 6;
    writeWord(reg_a[7] + 4, reg_pc & 0xFFFF);
    u8 vector = getInterruptVector(level);

    sync(4);
    writeWord(reg_a[7] + 0, SR);
    writeWord(reg_a[7] + 2, (reg_pc >> 16) & 0xFFFF);
    executeAt(vector);
}

unsigned Core_68k::getInterruptVector(u8 level) {
    level &= 7;
    sync(4);

    switch (interrupt) {
        default:
        case AUTO_VECTOR: return 24 + level;
        case USER_VECTOR: return getUserVector(level) & 0xff;
        case SPURIOUS: return 24;
        case UNINITIALIZED: return 15;
    }
}

void Core_68k::op_illegal(u16 opcode) {
    // printf("Core_68k::op_illegal(%x)\n", opcode);
    u8 nibble = opcode >> 12; //line A OR line F detection
    illegalMode = ILLEGAL_OPCODE;

    if (nibble == 0xA) {
        illegalMode = LINE_A;
        // printf("illegalMode = LINE_A\n");
    } else if (nibble == 0xF) {
        illegalMode = LINE_F;
        // printf("illegalMode = LINE_F\n");
    }
    trace = false;
}

void Core_68k::setPrivilegeException() {
    illegalMode = PRIVILEGE;
    trace = false;
    //real cpu finds out privilege violation in decode phase,
    //so pc is not incremented, emulation checks in opcode handler,
    //pc was incremented already
    reg_pc -= 2; //for stack frame
}

void Core_68k::illegalException(u8 iType) {
    illegalMode = NONE;
    u16 SR = reg_s;
    switchToSupervisor();
    reg_s.trace = 0;

    sync(4);
    reg_a[7] -= 6;
    /*
    writeWord(reg_a[7] + 4, reg_pc & 0xFFFF);
    writeWord(reg_a[7] + 0, SR);
    writeWord(reg_a[7] + 2, (reg_pc >> 16) & 0xFFFF);
    */
    writeWord(reg_a[7] + 4, reg_pc & 0xFFFF);
    writeWord(reg_a[7] + 2, (reg_pc >> 16) & 0xFFFF);
    writeWord(reg_a[7] + 0, SR);
    
    if (iType == ILLEGAL_OPCODE) {
        executeAt(4);
    } else if (iType == LINE_A) {
        executeAt(10);
    } else if (iType == LINE_F) {
        executeAt(11);
    } else if (iType == PRIVILEGE) {
        executeAt(8);
    }
}

void Core_68k::traceException() {
    u16 SR = reg_s;
    switchToSupervisor();
    reg_s.trace = 0;
    stop = false;

    sync(4);
    reg_a[7] -= 6;
    writeWord(reg_a[7] + 4, reg_pc & 0xFFFF);
    writeWord(reg_a[7] + 0, SR);
    writeWord(reg_a[7] + 2, (reg_pc >> 16) & 0xFFFF);
    executeAt(9);
}

void Core_68k::trapException(u8 vector) { //group 2 exceptions will triggered within opcode
    u16 SR = reg_s;
    reg_s.trace = 0;
    switchToSupervisor();

    reg_a[7] -= 6;
    writeWord(reg_a[7] + 4, reg_pc & 0xFFFF);
    writeWord(reg_a[7] + 2, (reg_pc >> 16) & 0xFFFF);
    writeWord(reg_a[7] + 0, SR);
    executeAt(vector);
}

void Core_68k::group0exception(u8 type) { //bus or address error
    //interrupts opcodes or group 1/2 exception stacking
    if (doubleFault) {
        throw CpuException(); //another group 0 exception during last one ... cpu halted
    }
    doubleFault = true; //assume worst case
    u16 SR = reg_s;

    u16 _status = (status_code._read ? 16 : 0) | (status_code._instruction ? 0 : 8);
    _status |= (SR & 0x2000 ? 4 : 0) | (status_code._program ? 2 : 1);

    printf("_status = %x SR = %x\n", _status, SR);

    reg_s.trace = trace = 0;
    switchToSupervisor();

    u32 fault_address = this->fault_address;

    sync(8);
    reg_a[7] -= 14;
    writeWord(reg_a[7] + 12, reg_pc & 0xFFFF);
    writeWord(reg_a[7] + 10, (reg_pc >> 16) & 0xFFFF);
    writeWord(reg_a[7] + 8, SR);
    writeWord(reg_a[7] + 6, reg_ird);
    writeWord(reg_a[7] + 4, (fault_address) & 0xFFFF);
    writeWord(reg_a[7] + 2, ((fault_address) >> 16) & 0xFFFF);
    writeWord(reg_a[7] + 0, _status );
    sync(2);
    executeAt(type == BUS_ERROR ? 2 : 3);
    doubleFault = false; //no further problem
    throw CpuException();
}

void Core_68k::executeAt(u8 vector) {
    reg_pc = readLong(vector << 2);
    fullprefetchFirstStep();
    sync(2);
    prefetch(true);
}

void Core_68k::switchToSupervisor() {
    if (!reg_s.s) {
        reg_usp = reg_a[7];
        reg_a[7] = reg_ssp;
        reg_s.s = true;
    }
}
