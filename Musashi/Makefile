OBJECTS   = m68kfpu.o m68kcpu.o m68kdasm.o m68kops.o
CC        = gcc
WARNINGS  = -Wall -Wextra -pedantic -Wno-strict-aliasing
OPTIMIZE  = -flto -O3
CFLAGS    = $(WARNINGS) $(OPTIMIZE)

.PHONY: all clean

all: $(OBJECTS)

clean:
	rm -f *.o

$(OBJECTS): %.o: %.c *.h
	$(CC) -c $(CFLAGS) $<

