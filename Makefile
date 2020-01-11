# Pretty naive Makefile for compiling the test runner application

CFILES    = main.cpp testrunner.cpp musashi.cpp TestCPU.cpp Sandbox.cpp

CC        = g++
INCLUDE   =-IMoira -IMusashi
WARNINGS  = -Wall -Wno-unused-variable
STD       = -std=c++14
OPTIMIZE  = -O3
CFLAGS    = $(INCLUDE) $(WARNINGS) $(STD) $(OPTIMIZE)

.PHONY: clean musashi moira

all: testRunner

clean:
	rm -f *.o

testRunner: musashi moira
	$(CC) -o testRunner $(CFLAGS) $(CFILES) Musashi/*.o Moira/*.o

musashi:
	make -C Musashi

moira:
	make -C Moira
