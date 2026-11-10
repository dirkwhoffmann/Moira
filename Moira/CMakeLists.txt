set(MOIRA_SRC

Moira.cpp
Cache/Cache.cpp
MoiraCore68000.cpp
MoiraCore68010.cpp
MoiraCore68020.cpp
MoiraDebugger.cpp
FPU/MFloat80.cpp
FPU/FpuEnv.cpp
FPU/FpuEnvMath.cpp
FPU/FpuFormats.cpp
FPU/FPU.cpp
MMU/MMU030.cpp
MMU/MMU040.cpp
)

add_library(moira STATIC ${MOIRA_SRC})

if (NOT MSVC)
    target_compile_options(moira PUBLIC -Wno-unused-parameter)
    target_compile_options(moira PUBLIC -Wno-unused-but-set-parameter)
    target_compile_options(moira PUBLIC -Wno-unused-but-set-variable)
    target_compile_options(moira PUBLIC -Wno-missing-field-initializers)
endif()

if(MSVC)
    target_compile_options(moira PUBLIC /W4 /bigobj) # /WX disabled for now
    # C4127/C4702: The handlers are templates. Code behind an 'if constexpr (...) return/throw;'
    # is reachable in some instantiations but not in others, which MSVC reports per instantiation.
    target_compile_options(moira PUBLIC /wd4100 /wd4201 /wd4324 /wd4458 /wd4127 /wd4702)
endif()

if(MINGW)
    target_compile_options(moira PUBLIC -Wa,-mbig-obj)
endif()

# Differently configured copies of Moira (used by the test runners).
# The definitions are PUBLIC, so consumers compile against the same configuration.
# All copies live in the default namespace, so a program can link only one of them.
# To link several copies into one program, give each its own MOIRA_NAMESPACE.
function(add_moira_variant NAME COMPAT PRECISE)
    add_library(${NAME} STATIC ${MOIRA_SRC})
    target_compile_definitions(${NAME} PUBLIC
        MOIRA_COMPATIBILITY=${COMPAT}
        MOIRA_PRECISE_TIMING=${PRECISE})
    target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    if (NOT MSVC)
        target_compile_options(${NAME} PUBLIC -Wno-unused-parameter -Wno-unused-but-set-parameter
            -Wno-unused-but-set-variable -Wno-missing-field-initializers)
    else()
        target_compile_options(${NAME} PUBLIC /W4 /bigobj /wd4100 /wd4201 /wd4324 /wd4458 /wd4127 /wd4702)
    endif()
    if(MINGW)
        target_compile_options(${NAME} PUBLIC -Wa,-mbig-obj)
    endif()
endfunction()

add_moira_variant(moiraUAE UAE true)
add_moira_variant(moiraMusashi MUSASHI false)

# The SingleStepTests runner needs address errors and exact timing
add_moira_variant(moiraMAME MAME true)
target_compile_definitions(moiraMAME PUBLIC MOIRA_EMULATE_ADDRESS_ERROR=true)

# The random runner skips address errors (covered by the exception tests) and is much faster without them
add_moira_variant(moiraMAMENoAE MAME true)

# The exception runner needs address errors in UAE mode as well
add_moira_variant(moiraUAEExc UAE true)
target_compile_definitions(moiraUAEExc PUBLIC MOIRA_EMULATE_ADDRESS_ERROR=true)
