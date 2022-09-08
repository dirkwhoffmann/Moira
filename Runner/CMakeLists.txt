add_library(runner STATIC

musashi.cpp
Testrunner.cpp
TestCPU.cpp
Sandbox.cpp
Randomizer.cpp
)

if (NOT MSVC)
    target_compile_options(runner PUBLIC -Wno-unused-parameter)
    target_compile_options(runner PUBLIC -Wno-unused-variable)
    target_compile_options(runner PUBLIC -Wno-unused-but-set-variable)
    target_compile_options(runner PUBLIC -Wno-implicit-fallthrough)
    target_compile_options(runner PUBLIC -Wno-missing-field-initializers)
endif()

# Add include paths
target_include_directories(runner PUBLIC

${CMAKE_SOURCE_DIR}/.
${CMAKE_SOURCE_DIR}/M68k
${CMAKE_SOURCE_DIR}/Musashi
${CMAKE_SOURCE_DIR}/Moira
)
