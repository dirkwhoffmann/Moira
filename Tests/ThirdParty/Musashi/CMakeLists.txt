add_library(musashi STATIC

m68kcpu.c
m68kdasm.c
m68kops.c
musashi_softfloat/musashi_softfloat.c
)

# Third-party code: no warnings
if(MSVC)
    target_compile_options(musashi PRIVATE /w)
else()
    target_compile_options(musashi PRIVATE -w)
endif()
