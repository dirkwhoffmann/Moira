MUSASHICFILES = \
m68kfpu.c \
m68kcpu.c \
m68kdasm.c \
m68kops.c

MUSASHIHFILES = \
m68k.h \
m68kcpu.h \
m68kconf.h \
m68kops.h \

CC        = gcc
WARNINGS  = -Wall -Wno-strict-aliasing
STD       = 
OPTIMIZE  = -O3
CFLAGS    = $(WARNINGS) $(STD) $(OPTIMIZE)

all: $(MUSASHIHFILES) $(MUSASHICFILES)
	$(CC) -c $(CFLAGS) $(MUSASHICFILES)

clean:
	rm -f *.o
