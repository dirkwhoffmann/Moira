add_library(musashi STATIC

m68kcpu.c
m68kdasm.c
m68kops.c
musashi_softfloat/musashi_softfloat.c
)

if(NOT MSVC)
    target_compile_options(musashi PUBLIC -Wno-strict-aliasing)
    target_compile_options(musashi PUBLIC -Wno-string-concatenation)
endif()

if(MINGW)
    target_compile_options(musashi PUBLIC -Wno-format-overflow)
endif()
