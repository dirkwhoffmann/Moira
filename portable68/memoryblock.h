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
    } block1[100];
    struct {
         unsigned addr;
         u8 value;
         bool used;
     } block2[100];

    struct {
        unsigned addr;
        bool used;
    } buserror1[100];
    struct {
         unsigned addr;
         bool used;
     } buserror2[100];

    void init() {
        for(int i = 0; i < maxElements; i++) {
            block1[i].addr = block2[i].addr = 0;
            block1[i].value = block2[i].value = 0;
            block1[i].used = block2[i].used = false;

            buserror1[i].used = buserror2[i].used = false;
            buserror1[i].addr = buserror2[i].addr = 0;
        }
        pos1 = pos2 = 0;
    }

    void setBusError(unsigned addr) {
        for(int i = 0; i < maxElements; i++) {
            if(buserror1[i].addr == addr) {
                return;
            }
        }
        for(int i = 0; i < maxElements; i++) {
            if(!buserror1[i].used) {
                buserror1[i].used = true;
                buserror1[i].addr = addr;
                return;
            }
        }
    }
    void setBusError2(unsigned addr) {
        for(int i = 0; i < maxElements; i++) {
            if(buserror2[i].addr == addr) {
                return;
            }
        }
        for(int i = 0; i < maxElements; i++) {
            if(!buserror2[i].used) {
                buserror2[i].used = true;
                buserror2[i].addr = addr;
                return;
            }
        }
    }

    bool isBusError(unsigned addr) {
        for(int i = 0; i < maxElements; i++) {
            if (buserror1[i].used && buserror1[i].addr == addr) {
                return true;
            }
        }
        return false;
    }
    bool isBusError2(unsigned addr) {
        for(int i = 0; i < maxElements; i++) {
            if (buserror2[i].used && buserror2[i].addr == addr) {
                return true;
            }
        }
        return false;
    }

    u8 read1(unsigned addr) {
        for(int i = 0; i < maxElements; i++) {
            if (block1[i].used && block1[i].addr == addr) {
                return block1[i].value;
            }
        }
        return 0;
    }
    u8 read2(unsigned addr) {
        for(int i = 0; i < maxElements; i++) {
            if (block2[i].used && block2[i].addr == addr) {
                return block2[i].value;
            }
        }
        return 0;
    }

    void write1(unsigned addr, u8 value) {
        for(int i = 0; i < maxElements; i++) {
            if (block1[i].used && block1[i].addr == addr) {
                block1[i].value = value;
                return;
            }
        }
        block1[pos1].addr = addr;
        block1[pos1].value = value;
        block1[pos1].used = true;

        if (++pos1 == maxElements) {
            throw Exception("memory block is too small, increase size");
        }
    }
    void write2(unsigned addr, u8 value) {
        for(int i = 0; i < maxElements; i++) {
            if (block2[i].used && block2[i].addr == addr) {
                block2[i].value = value;
                return;
            }
        }
        block2[pos2].addr = addr;
        block2[pos2].value = value;
        block2[pos2].used = true;

        if (++pos2 == maxElements) {
             throw Exception("memory block is too small, increase size");
         }
    }
    void write(unsigned addr, u8 value) {
        write1(addr, value);
        write2(addr, value);
    }

    u32 readLong1(u32 addr) {
        u32 Dword = readWord1(addr) << 16;
        Dword |= readWord1(addr+2);
        return Dword;
    }
    u32 readLong2(u32 addr) {
        u32 Dword = readWord2(addr) << 16;
        Dword |= readWord2(addr+2);
        return Dword;
    }

    u16 readWord1(u32 addr) {
        return (read1(addr & 0xffffff) << 8) | read1( (addr+1) & 0xffffff);
    }
    u16 readWord2(u32 addr) {
         return (read2(addr & 0xffffff) << 8) | read2( (addr+1) & 0xffffff);
     }

    void writeLong1(u32 addr, u32 value) {
        writeWord1(addr, (value >> 16) & 0xFFFF);
        writeWord1(addr+2, value & 0xFFFF);
    }
    void writeLong2(u32 addr, u32 value) {
         writeWord2(addr, (value >> 16) & 0xFFFF);
         writeWord2(addr+2, value & 0xFFFF);
     }
    void writeLong(u32 addr, u32 value) {
         writeLong1(addr,value);
         writeLong2(addr,value);
      }

    void writeWord1(u32 addr, u16 value) {
        write1(addr & 0xffffff, value >> 8);
        write1((addr+1) & 0xffffff, value & 0xFF);
    }
    void writeWord2(u32 addr, u16 value) {
        write2(addr & 0xffffff, value >> 8);
        write2((addr+1) & 0xffffff, value & 0xFF);
    }
    void writeWord(u32 addr, u16 value) {
        writeWord1(addr,value);
        writeWord2(addr,value);
     }

    void setError(unsigned blockCount) {
        if (blockCount == -1) {
            error = " address not found ";
            return;
        }
        error = "adr: " + Helper::convertIntToHexString(block1[blockCount].addr) + ", val: " + Helper::convertIntToHexString(block1[blockCount].value);
    }

    string getError() {
        return error;
    }

    static bool compare(MemoryBlock* sampled, MemoryBlock* calced) {
        bool found;
        int backup;

        for(int i = 0; i < sampled->getMaxElements(); i++) {
            if (!sampled->block1[i].used) {
                continue;
            }
            found = false;
            backup = -1;
            for(int j = 0; j < calced->getMaxElements(); j++) {
                if (sampled->block1[i].addr == calced->block1[j].addr) {
                    backup = j;
                    if (sampled->block1[i].value == calced->block1[j].value) {
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
    unsigned pos1;
    unsigned pos2;
    unsigned maxElements;
    string error;
};

#endif // CODEBLOCK_H
