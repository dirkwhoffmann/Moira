
#include "tester.h"
#include "exception.h"
#include <time.h>
#include "Moira.h"
#include "musashi.h"

moira::Moira *moiracpu;
Tester_68k tester;
uint8_t mem[0x10000];

int main()
{
    // execTest();

    dasmTest();

    /*
    moiracpu = new moira::Moira();

    try {
        tester.runTestCases();
        tester.printErrorCounter();

    } catch(Exception& exception) {
        exception.handle();
    }
    */

    return 0;
}

