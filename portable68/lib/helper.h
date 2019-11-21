#ifndef HELPER_H
#define HELPER_H

#include <sstream>
#include <string>
using namespace std;

class Helper {
public:
    static string convertIntToString(int val) {
        stringstream ss;
        ss << val;
        return ss.str();
    }

    static string convertIntToHexString(int val) {
        stringstream ss;
        ss << std::hex << val;
        return "0x" + ss.str();
    }
};

#endif // HELPER_H
