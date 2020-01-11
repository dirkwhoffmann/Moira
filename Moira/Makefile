MOIRACFILES = \
Moira.cpp \
MoiraDebugger.cpp

MOIRAHFILES = \
Moira.h \
MoiraDataflow.h \
MoiraInit_cpp.h \
MoiraALU.h \
MoiraDataflow_cpp.h \
MoiraTypes.h \
MoiraALU_cpp.h \
MoiraDebugger.h \
StrWriter.h \
MoiraConfig.h \
MoiraExec.h \
StrWriter_cpp.h \
MoiraDasm.h \
MoiraExec_cpp.h \
MoiraDasm_cpp.h \
MoiraInit.h

CC        = g++
WARNINGS  = -Wall -Wno-nested-anon-types -Wno-gnu-anonymous-struct
STD       = -std=c++14
OPTIMIZE  = -O3
CFLAGS    = $(WARNINGS) $(STD) $(OPTIMIZE)

all: $(MOIRAHFILES) $(MOIRACFILES)
	$(CC) -c $(CFLAGS) $(MOIRACFILES)

clean:
	rm -f *.o
