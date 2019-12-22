
#include "tester.h"
#include "exception.h"
#include <time.h>
#include "Moira.h"
#include "musashi.h"

moira::Moira *moiracpu;

int main()
{
    // dasmTest();


    moiracpu = new moira::Moira();

    try {
        Tester_68k tester;
        moiracpu->memory = &tester;

        tester.runTestCases();
        tester.printErrorCounter();

    } catch(Exception& exception) {
        exception.handle();
    }
    

    return 0;
}

