
#include "tester.h"
#include "exception.h"
#include <time.h>

int main()
{
    try {
        Tester_68k tester;
        tester.runTestCases();
        tester.printErrorCounter();
    } catch(Exception& exception) {
        exception.handle();
    }
    return 0;
}

