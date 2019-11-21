#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <iostream>

using namespace std;

class Exception {
    string msg;
public:
    Exception(string msg) : msg(msg) {}

    int handle() const {
        if (msg.length() != 0) {
            cout << "exception: " + msg << endl;
        } else {
            cout << "unhadeled error";
        }
        return 1;
    }
};

#endif
