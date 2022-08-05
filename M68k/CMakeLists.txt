add_library(m68k STATIC

m68k_disasm.cpp
)

target_compile_options(m68k PUBLIC -Wno-unused-parameter)
target_compile_options(m68k PUBLIC -Wno-unused-variable)
target_compile_options(m68k PUBLIC -Wno-unused-but-set-variable)
