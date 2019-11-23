
#include "tester.h"
#include "exception.h"
#include <time.h>
#include "Moira.h"

Moira *moira;

int main()
{
    moira = new Moira();

    try {
        Tester_68k tester;
        moira->delegate = &tester;

        tester.runTestCases();
        tester.printErrorCounter();

    } catch(Exception& exception) {
        exception.handle();
    }


    return 0;
}

