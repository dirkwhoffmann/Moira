add_library(moira STATIC

Moira.cpp
MoiraDebugger.cpp
)

target_compile_options(moira PUBLIC -Wno-unused-parameter)
