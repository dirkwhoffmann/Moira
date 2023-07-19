add_library(binutils STATIC

m68k-dis.c
m68k-opc.c
)

target_include_directories(binutils PUBLIC

${CMAKE_SOURCE_DIR}/Runner
)
