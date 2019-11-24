
#include "tester.h"
#include "exception.h"
#include <time.h>
#include "Moira.h"

Moira::CPU *moira;

int main()
{
    moira = new Moira::CPU();

    try {
        Tester_68k tester;
        moira->memory = &tester;

        tester.runTestCases();
        tester.printErrorCounter();

    } catch(Exception& exception) {
        exception.handle();
    }


    return 0;
}

