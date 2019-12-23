
#include "tester.h"
#include "exception.h"
#include <time.h>
#include "Moira.h"
#include "musashi.h"
#include "globals.h"

moira::Moira *moiracpu;
Tester_68k *tester;
uint8_t mem[0x10000];

int main()
{
    moiracpu = new moira::Moira();
    tester = new Tester_68k();

    if (MUSASHI) {

        // Run Moira against Musashi
        execTest();
        // dasmTest();

    } else {

        // Run the portable68000 unit tests

        try {
            tester->runTestCases();
            tester->printErrorCounter();

        } catch(Exception& exception) {
            exception.handle();
        }
    }

    return 0;
}

