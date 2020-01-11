OBJECTS   = main.o testRunner.o musashi.o TestCPU.o Sandbox.o
CC        = g++
INCLUDE   =-IMoira -IMusashi
WARNINGS  = -Wall
STD       = -std=c++14
OPTIMIZE  = -flto -O3
CFLAGS    = $(INCLUDE) $(WARNINGS) $(STD) $(OPTIMIZE)

.PHONY: all musashi moira clean

all: musashi moira testRunner

musashi:
	make -C Musashi

moira:
	make -C Moira

clean:
	# make -C Musashi clean
	# make -C Moira clean
	rm -f *.o

testRunner: $(OBJECTS)
	$(CC) -o testRunner $(CFLAGS) $(OBJECTS) Musashi/*.o Moira/*.o

$(OBJECTS): %.o: %.cpp
	$(CC) -c $(CFLAGS) $<
