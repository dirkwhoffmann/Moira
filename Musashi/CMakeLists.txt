add_library(musashi STATIC

m68kfpu.c
m68kcpu.c
m68kdasm.c
m68kops.c
)

target_compile_options(musashi PUBLIC -pedantic)
target_compile_options(musashi PUBLIC -Wno-strict-aliasing)
target_compile_options(musashi PUBLIC -Wno-string-concatenation)

#add_compile_options(musashi -Wall -Wextra -Werror -pedantic)
#add_compile_options(musashi -Wno-strict-aliasing)
#add_compile_options(musashi -Wno-string-concatenation)
  
#OBJECTS   = m68kfpu.o m68kcpu.o m68kdasm.o m68kops.o
#CC        = gcc
#WARNINGS  = -Wall -Wextra -pedantic -Wno-strict-aliasing -Wno-string-concatenation
#OPTIMIZE  = -flto -O3
#CFLAGS    = $(WARNINGS) $(OPTIMIZE)
#
#.PHONY: all clean
#
#all: $(OBJECTS)
#
#clean:
#	rm -f *.o
#
#$(OBJECTS): %.o: %.c *.h
#	$(CC) -c $(CFLAGS) $<
