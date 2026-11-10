add_library(binutils STATIC

m68k-dis.c
m68k-opc.c
)

target_include_directories(binutils PUBLIC

${CMAKE_SOURCE_DIR}/Tests/Random
)

# The MIT flavor of the disassembler test needs a binutils that is compiled without MOTOROLA
# (see Tests/Random/config.h)
add_library(binutilsmit STATIC

m68k-dis.c
m68k-opc.c
)

target_compile_definitions(binutilsmit PUBLIC RUNNER_STAGES_DASM_MIT)

target_include_directories(binutilsmit PUBLIC

${CMAKE_SOURCE_DIR}/Tests/Random
)

# Third-party code: no warnings
foreach(T binutils binutilsmit)
  if(MSVC)
    target_compile_options(${T} PRIVATE /w)
  else()
    target_compile_options(${T} PRIVATE -w)
  endif()
endforeach()
