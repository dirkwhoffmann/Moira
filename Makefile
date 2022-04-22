OBJECTS   = main.o testrunner.o musashi.o TestCPU.o Sandbox.o
CC        = g++
INCLUDE   = -IMoira -IMusashi
WARNINGS  = -Wall -Wno-unused-variable
STD       = -std=c++20
OPTIMIZE  = -flto -O3
CFLAGS    = $(INCLUDE) $(WARNINGS) $(STD) $(OPTIMIZE)

.PHONY: all musashi moira clean

all: musashi moira testrunner

musashi:
	make -C Musashi

moira:
	make -C Moira

clean:
	make -C Musashi clean
	make -C Moira clean
	rm -f testrunner
	rm -f *.o

testrunner: $(OBJECTS)
	$(CC) -o testrunner $(CFLAGS) $(OBJECTS) Musashi/*.o Moira/*.o

$(OBJECTS): %.o: %.cpp
	$(CC) -c $(CFLAGS) $<

