
void fullprefetch(bool lastCycle = false) {
    status_code._program = true;
    reg_irc = readWord(reg_pc);
    prefetch(lastCycle);
}

void fullprefetchFirstStep() {
    status_code._program = true;
    reg_irc = readWord(reg_pc);
    status_code._program = false;
}

void prefetch(bool lastCycle = false) {
    status_code._program = true;
    reg_ir = reg_irc;
    reg_irc = readWord(reg_pc + 2, lastCycle);
    reg_ird = reg_ir;
    status_code._program = false;
}

void readExtensionWord() {
    reg_pc += 2;
    status_code._program = true;
    logInstruction(reg_irc, false);
    reg_irc = readWord(reg_pc);
    status_code._program = false;
}

u8 readByte(u32 addr, bool lastCycle = false) {
    status_code._read = true;
    fault_address = addr;
    sync(2); //put address on bus
    if (lastCycle) sampleIrq();
    u8 data = memRead(addr & 0xffffff);
    sync(2);
	return data;
}

u16 readWord(u32 addr, bool lastCycle = false) {
    status_code._read = true;
    fault_address = addr;
    sync(2);
    if ((addr & 1) == 1) {
        group0exception( ADDRESS_ERROR );
    }
    if (lastCycle) sampleIrq();
    u16 word = memWordRead(addr & 0xffffff);
    sync(2);
	return word;
}

u32 readLong(u32 addr, bool lastCycle = false) {
	u32 Dword = readWord(addr) << 16;
    Dword |= readWord(addr+2, lastCycle);
	return Dword;
}


void writeByte(u32 addr, u8 value, bool lastCycle = false) {
    status_code._read = false;
    fault_address = addr;
    sync(2);
    if (lastCycle) sampleIrq();

    memWrite(addr & 0xffffff, value);
    sync(2);
}

void writeWord(u32 addr, u16 value, bool lastCycle = false) {
    status_code._read = false;
    fault_address = addr;
    sync(2);
	if ((addr & 1) == 1) {
        group0exception( ADDRESS_ERROR );
	}
    if (lastCycle) sampleIrq();

    memWordWrite(addr & 0xffffff, value);
    sync(2);
}

void writeLong(u32 addr, u32 value, bool lastCycle = false) {
    writeWord(addr, (value >> 16) & 0xFFFF);
    writeWord(addr+2, value & 0xFFFF, lastCycle);
}

void writeStack(u32 value, bool lastCycle = false) {
	reg_a[7] -= 4;
    writeLong(reg_a[7], value, lastCycle);
}
