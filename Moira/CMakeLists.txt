add_library(moira STATIC

Moira.cpp
MoiraDebugger.cpp
softfloat/softfloat.cpp
)

target_include_directories(moira PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/softfloat)

if (NOT MSVC)
    target_compile_options(moira PUBLIC -Wno-unused-parameter)
    target_compile_options(moira PUBLIC -Wno-unused-but-set-parameter)
    target_compile_options(moira PUBLIC -Wno-unused-but-set-variable)
    target_compile_options(moira PUBLIC -Wno-missing-field-initializers)
endif()

if(MSVC)
    target_compile_options(moira PUBLIC /W4 /bigobj) # /WX disabled for now
    target_compile_options(moira PUBLIC /wd4100 /wd4201 /wd4324 /wd4458 /wd4127)
endif()

if(MINGW)
    target_compile_options(moira PUBLIC -Wa,-mbig-obj)
endif()
