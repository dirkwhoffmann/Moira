add_library(musashi STATIC

m68kfpu.c
m68kcpu.c
m68kdasm.c
m68kops.c
)

target_compile_options(musashi PUBLIC -Wno-strict-aliasing)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(musashi PUBLIC -Wno-format-overflow)
else()
  target_compile_options(musashi PUBLIC -Wno-string-concatenation)
endif()
