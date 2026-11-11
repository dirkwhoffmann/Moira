# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# -----------------------------------------------------------------------------
# Randomized differential tests
#
# The tools execute randomized instruction sequences on Moira and a reference
# core and compare their results. Each tool tests a different Moira compatibility
# mode and reference implementation.
#
#   randomuae       Moira (UAE compatibility) vs UAE
#
#   randommusashi   Moira (Musashi compatibility) vs Musashi
#
#   randommame      Moira (MAME compatibility, without address errors) vs MAME's
#                   microcoded 68000
# -----------------------------------------------------------------------------

set(RUNNER_SRC
    RunnerMain.cpp
    TestRunner.cpp
    ExecRunner.cpp
    ExecMMU.cpp
    ExecCompare.cpp
    DasmRunner.cpp
    Reference.cpp
    MusashiRef.cpp
    UAERef.cpp
    MAMERef.cpp
    TestCPU.cpp
    Shared.cpp
    Sandbox.cpp
    Randomizer.cpp
    ../ThirdParty/MusashiGlue/musashi.cpp
    ../ThirdParty/BinutilsGlue/binutils.cpp
    ../ThirdParty/UAEGlue/uae.cpp
    ../ThirdParty/MAMEGlue/mame.cpp)

set(RUNNER_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Moira
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Tests/ThirdParty
    ${CMAKE_SOURCE_DIR}/Tests/ThirdParty/Binutils
    ${CMAKE_SOURCE_DIR}/Tests/ThirdParty/Musashi)

if(MSVC)

    set(RUNNER_OPTIONS
        /wd4100
        /wd4201
        /wd4324
        /wd4458)
        
else()

    set(RUNNER_OPTIONS
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-gnu-anonymous-struct
        -Wno-nested-anon-types)
        
endif()

#
# Randomized differential runners
#

add_executable(randommusashi RandomMusashi.cpp ${RUNNER_SRC})
target_link_libraries(randommusashi PRIVATE moiraMusashi binutils musashi uaecpu mameref)
target_include_directories(randommusashi PRIVATE ${RUNNER_INCLUDE_DIRS})
target_compile_options(randommusashi PRIVATE ${RUNNER_OPTIONS})

add_executable(randomuae RandomUAE.cpp ${RUNNER_SRC})
target_link_libraries(randomuae PRIVATE moiraUAE binutils musashi uaecpu mameref)
target_include_directories(randomuae PRIVATE ${RUNNER_INCLUDE_DIRS})
target_compile_options(randomuae PRIVATE ${RUNNER_OPTIONS})

add_executable(randommame RandomMAME.cpp ${RUNNER_SRC})
target_link_libraries(randommame PRIVATE moiraMAMENoAE binutils musashi uaecpu mameref)
target_include_directories(randommame PRIVATE ${RUNNER_INCLUDE_DIRS})
target_compile_options(randommame PRIVATE ${RUNNER_OPTIONS})

# A fixed seed keeps automated runs reproducible. Without --seed, the runners
# derive the seed from the date.

add_test(NAME RandomMusashi COMMAND randommusashi --seed 1)
add_test(NAME RandomUAE COMMAND randomuae --seed 1)
add_test(NAME RandomMAME COMMAND randommame --seed 1)

set_tests_properties(
    RandomMusashi RandomUAE RandomMAME
    PROPERTIES LABELS "random"
)

# -----------------------------------------------------------------------------
# Disassembler tests
#
# The tools disassemble every opcode and compare the output of Moira with the
# reference disassemblers.
#
#   dasmmot   Motorola syntax. Moira's GNU, MUSASHI and UAE syntaxes are
#             compared with Binutils, Musashi and UAE. Moira's own syntax is
#             checked for plausibility only.
#
#   dasmmit   MIT syntax. Moira's GNU_MIT syntax is compared with Binutils
#             (compiled without MOTOROLA). Moira's own MOIRA_MIT syntax is
#             checked for plausibility only.
# -----------------------------------------------------------------------------

add_executable(dasmmot DasmMot.cpp ${RUNNER_SRC})
target_link_libraries(dasmmot PRIVATE moiraUAE binutils musashi uaecpu mameref)
target_include_directories(dasmmot PRIVATE ${RUNNER_INCLUDE_DIRS})
target_compile_options(dasmmot PRIVATE ${RUNNER_OPTIONS})

add_executable(dasmmit DasmMit.cpp ${RUNNER_SRC})
target_link_libraries(dasmmit PRIVATE moiraUAE binutilsmit musashi uaecpu mameref)
target_include_directories(dasmmit PRIVATE ${RUNNER_INCLUDE_DIRS})
target_compile_options(dasmmit PRIVATE ${RUNNER_OPTIONS})

add_test(NAME DasmMot COMMAND dasmmot --seed 1)
add_test(NAME DasmMit COMMAND dasmmit --seed 1)

set_tests_properties(
    DasmMot DasmMit
    PROPERTIES LABELS "dasm")
