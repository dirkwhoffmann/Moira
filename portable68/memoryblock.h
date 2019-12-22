#ifndef MEMORYBLOCK_H
#define MEMORYBLOCK_H

#include "dataTypes.h"
#include <string>
#include "helper.h"
#include "exception.h"
using namespace std;

class MemoryBlock {

public:
    MemoryBlock() {
        maxElements = 100;
        init();
    }

    unsigned getMaxElements() {
        return maxElements;
    }

    struct {
        unsigned addr;
        u8 value;
        bool used;
    } block[100];

    struct {
        unsigned addr;
        bool used;
    } buserror[100];

    void init() {
        for(int i = 0; i < maxElements; i++) {
            block[i].addr = 0;
            block[i].value = 0;
            block[i].used = false;

            buserror[i].used = false;
            buserror[i].addr = 0;
        }
        pos = 0;
    }

    void setBusError(unsigned addr) {
        for(int i = 0; i < maxElements; i++) {
            if(buserror[i].addr == addr) {
                return;
            }
        }
        for(int i = 0; i < maxElements; i++) {
            if(!buserror[i].used) {
                buserror[i].used = true;
                buserror[i].addr = addr;
                return;
            }
        }
    }

    bool isBusError(unsigned addr) {
        for(int i = 0; i < maxElements; i++) {
            if (buserror[i].used && buserror[i].addr == addr) {
                return true;
            }
        }
        return false;
    }

    u8 read(unsigned addr) {
        for(int i = 0; i < maxElements; i++) {
            if (block[i].used && block[i].addr == addr) {
                return block[i].value;
            }
        }
        return 0;
    }

    void write(unsigned addr, u8 value) {
        for(int i = 0; i < maxElements; i++) {
            if (block[i].used && block[i].addr == addr) {
                block[i].value = value;
                return;
            }
        }
        assert(pos < maxElements);
        block[pos].addr = addr;
        block[pos].value = value;
        block[pos].used = true;

        if (++pos == maxElements) {
            throw Exception("memory block is too small, increase size");
        }
    }

    u32 readLong(u32 addr) {
        u32 Dword = readWord(addr) << 16;
        Dword |= readWord(addr+2);
        return Dword;
    }

    u16 readWord(u32 addr) {
        return (read(addr & 0xffffff) << 8) | read( (addr+1) & 0xffffff);
    }

    void writeLong(u32 addr, u32 value) {
        writeWord(addr, (value >> 16) & 0xFFFF);
        writeWord(addr+2, value & 0xFFFF);
    }

    void writeWord(u32 addr, u16 value) {
        write(addr & 0xffffff, value >> 8);
        write((addr+1) & 0xffffff, value & 0xFF);
    }

    void setError(unsigned blockCount) {
        if (blockCount == -1) {
            error = " address not found ";
            return;
        }
        error = "adr: " + Helper::convertIntToHexString(block[blockCount].addr) + ", val: " + Helper::convertIntToHexString(block[blockCount].value);
    }

    string getError() {
        return error;
    }

    static bool compare(MemoryBlock* sampled, MemoryBlock* calced) {
        bool found;
        int backup;

        for(int i = 0; i < sampled->getMaxElements(); i++) {
            if (!sampled->block[i].used) {
                continue;
            }
            found = false;
            backup = -1;
            for(int j = 0; j < calced->getMaxElements(); j++) {
                if (sampled->block[i].addr == calced->block[j].addr) {
                    backup = j;
                    if (sampled->block[i].value == calced->block[j].value) {
                        found = true;
                    }
                    break;
                }
            }
            if (!found) {
                sampled->setError(i);
                calced->setError(backup);
                return false;
            }
        }
        return true;
    }
    
private:
    unsigned pos = 0;
    unsigned maxElements;
    string error;
};

#endif // CODEBLOCK_H
