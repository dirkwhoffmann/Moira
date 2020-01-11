OBJECTS   = Moira.o MoiraDebugger.o
CC        = g++
WARNINGS  = -Wall
STD       = -std=c++14
OPTIMIZE  = -flto -O3
CFLAGS    = $(WARNINGS) $(STD) $(OPTIMIZE)

.PHONY: all clean

all: $(OBJECTS)

clean:
	rm -f *.o

$(OBJECTS): %.o: %.cpp
	$(CC) -c $(CFLAGS) $<
