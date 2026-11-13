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
#               exceptionmame               Exception frames Moira vs. MAME
#               exceptionuae                Exception frames Moira vs. UAE
#               retrymoira                  Bus error retry after RTE (68030/68040)
#   Sequence/   sequenceuae                 Interrupt, trace, STOP and RTE sequences Moira vs. UAE
#               sequencemame                Interrupt, trace, STOP and RTE sequences Moira vs. MAME
#   Random/     randommame                  Randomized tests Moira vs. MAME
#               randommusashi               Randomized tests Moira vs. Musashi
#               randomuae                   Randomized tests Moira vs. UAE
#               dasmmot                     Disassembler test in Motorola format
#               dasmmit                     Disassembler test in MIT format

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
    MOIRA_PRECISE_TIMING=true
)

# Musashi compatibility

add_library(moiraMusashi STATIC ${MOIRA_SOURCES})
target_include_directories(moiraMusashi PUBLIC ${MOIRA_DIR})
target_compile_options(moiraMusashi PUBLIC ${MOIRA_OPTIONS})
target_compile_definitions(moiraMusashi PUBLIC

    MOIRA_COMPATIBILITY=MUSASHI
    MOIRA_PRECISE_TIMING=false
)

# MAME compatibility (address errors and exact timing)

add_library(moiraMAME STATIC ${MOIRA_SOURCES})
target_include_directories(moiraMAME PUBLIC ${MOIRA_DIR})
target_compile_options(moiraMAME PUBLIC ${MOIRA_OPTIONS})
target_compile_definitions(moiraMAME PUBLIC

    MOIRA_COMPATIBILITY=MAME
    MOIRA_EMULATE_ADDRESS_ERROR=true
    MOIRA_EMULATE_BUS_ERROR=true
    MOIRA_PRECISE_TIMING=true
)

# MAME compatibility without address errors (the random runner runs much faster without them)

add_library(moiraMAMENoAE STATIC ${MOIRA_SOURCES})
target_include_directories(moiraMAMENoAE PUBLIC ${MOIRA_DIR})
target_compile_options(moiraMAMENoAE PUBLIC ${MOIRA_OPTIONS})
target_compile_definitions(moiraMAMENoAE PUBLIC

    MOIRA_COMPATIBILITY=MAME
    MOIRA_PRECISE_TIMING=true
)

# UAE compatibility with address errors (needed by the SingleStepTests and exception runners)

add_library(moiraUAEExc STATIC ${MOIRA_SOURCES})
target_include_directories(moiraUAEExc PUBLIC ${MOIRA_DIR})
target_compile_options(moiraUAEExc PUBLIC ${MOIRA_OPTIONS})
target_compile_definitions(moiraUAEExc PUBLIC

    MOIRA_COMPATIBILITY=UAE
    MOIRA_EMULATE_ADDRESS_ERROR=true
    MOIRA_EMULATE_BUS_ERROR=true
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
add_subdirectory(Sequence)
add_subdirectory(SST)
add_subdirectory(Random)
