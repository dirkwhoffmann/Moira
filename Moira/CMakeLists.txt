set(MOIRA_SRC

Moira.cpp
MoiraCore68000.cpp
MoiraCore68010.cpp
MoiraCore68020.cpp
MoiraDebugger.cpp
Float80.cpp
FloatMath.cpp
FpuFormats.cpp
FPU.cpp
)

add_library(moira STATIC ${MOIRA_SRC})

file(GLOB MOIRA_SOFTFLOAT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/softfloat/*.c)
target_sources(moira PRIVATE ${MOIRA_SOFTFLOAT_SRC})

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

# Differently configured copies of Moira that can be linked into one binary.
# Each copy lives in its own C++ namespace (MOIRA_NAMESPACE) and uses its own
# prefix for the exported softfloat C symbols (MOIRA_SOFTFLOAT_PREFIX).
# The definitions are PUBLIC, so consumers compile against the same configuration.
function(add_moira_variant NAME COMPAT)
    add_library(${NAME} STATIC ${MOIRA_SRC})
    target_sources(${NAME} PRIVATE ${MOIRA_SOFTFLOAT_SRC})
    target_compile_definitions(${NAME} PUBLIC
        MOIRA_NAMESPACE=${NAME}
        MOIRA_SOFTFLOAT_PREFIX=${NAME}_
        MOIRA_COMPATIBILITY=${COMPAT})
    target_include_directories(${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    if (NOT MSVC)
        target_compile_options(${NAME} PUBLIC -Wno-unused-parameter -Wno-unused-but-set-parameter
            -Wno-unused-but-set-variable -Wno-missing-field-initializers)
    else()
        target_compile_options(${NAME} PUBLIC /W4 /bigobj /wd4100 /wd4201 /wd4324 /wd4458 /wd4127)
    endif()
    if(MINGW)
        target_compile_options(${NAME} PUBLIC -Wa,-mbig-obj)
    endif()
endfunction()

add_moira_variant(moiraUAE UAE)
add_moira_variant(moiraMusashi MUSASHI)
