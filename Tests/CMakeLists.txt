# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Tests and test tools
#
#   Unit/       unit                        Unit tests using the doctest framework
#   Smoke/      smoke                       SingleStepTests Moira vs. reference results
#   SST/        sstmame                     SingleStepTests Moira vs. MAME
#               sstmusashi                  SingleStepTests Moira vs. Musashi
#               sstuae                      SingleStepTests Moira vs. UAE
#   Random/     randommame                  Randomized tests Moira vs. MAME
#               randommusashi               Randomized tests Moira vs. Musashi
#               randomuae                   Randomized tests Moira vs. UAE
#   Exception/  exceptionmame               Exceptions frames Moira vs. MAME
#               exceptionuae                Exceptions frames Moira vs. MAME
#   Dasm/       dasmmot                     Disassembler test in Motorola format
#               dasmmit                     Disassembler tests in MIT format

# Differently configured copies of Moira (one per compatibility mode)

get_target_property(MOIRA_DIR moira SOURCE_DIR)
get_target_property(MOIRA_SOURCES moira SOURCES)
get_target_property(MOIRA_OPTIONS moira INTERFACE_COMPILE_OPTIONS)
list(TRANSFORM MOIRA_SOURCES PREPEND "${MOIRA_DIR}/")

# UAE compatibility

add_library(moiraUAE STATIC ${MOIRA_SOURCES})
target_include_directories(moiraUAE PUBLIC ${MOIRA_DIR})
target_compile_options(moiraUAE PUBLIC ${MOIRA_OPTIONS})
target_compile_definitions(moiraUAE PUBLIC

    MOIRA_COMPATIBILITY=UAE
    MOIRA_EMULATE_ADDRESS_ERROR=true
    MOIRA_PRECISE_TIMING=true
)

# Musashi compatibility

add_library(moiraMusashi STATIC ${MOIRA_SOURCES})
target_include_directories(moiraMusashi PUBLIC ${MOIRA_DIR})
target_compile_options(moiraMusashi PUBLIC ${MOIRA_OPTIONS})
target_compile_definitions(moiraMusashi PUBLIC

    MOIRA_COMPATIBILITY=MUSASHI
    MOIRA_EMULATE_ADDRESS_ERROR=true
    MOIRA_PRECISE_TIMING=false
)

# MAME compatibility (address errors and exact timing)

add_library(moiraMAME STATIC ${MOIRA_SOURCES})
target_include_directories(moiraMAME PUBLIC ${MOIRA_DIR})
target_compile_options(moiraMAME PUBLIC ${MOIRA_OPTIONS})
target_compile_definitions(moiraMAME PUBLIC

    MOIRA_COMPATIBILITY=MAME
    MOIRA_EMULATE_ADDRESS_ERROR=true
    MOIRA_PRECISE_TIMING=true
)

# MAME compatibility without address errors (the random runner skips them, covered by the
# exception tests, and runs much faster without them)

add_library(moiraMAMENoAE STATIC ${MOIRA_SOURCES})
target_include_directories(moiraMAMENoAE PUBLIC ${MOIRA_DIR})
target_compile_options(moiraMAMENoAE PUBLIC ${MOIRA_OPTIONS})
target_compile_definitions(moiraMAMENoAE PUBLIC

    MOIRA_COMPATIBILITY=MAME
    MOIRA_PRECISE_TIMING=true
)

# Compiler-specific options

#if(MSVC)
#    foreach(TARGET moiraUAE moiraMusashi moiraMAME)
#        target_compile_options(${TARGET} PUBLIC /bigobj)
#    endforeach()
#elseif(MINGW)
#    foreach(TARGET moiraUAE moiraMusashi moiraMAME)
#        target_compile_options(${TARGET} PUBLIC -Wa,-mbig-obj)
#    endforeach()
#endif()

add_subdirectory(ThirdParty)
add_subdirectory(Unit)
add_subdirectory(Smoke)
add_subdirectory(SST)
add_subdirectory(Random)
add_subdirectory(Exception)
add_subdirectory(Dasm)
