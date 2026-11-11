# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# -----------------------------------------------------------------------------
# Tools based on the SingleStepTests suite.
#
# Each tool has a small main file that selects the runner and reference core.
# The runners (SSTRunner.cpp, PlainRunner.cpp, ExceptionRunner.cpp) are shared.
#
#   sst<ref>       Runs instructions on Moira and a reference core and compares
#                  the results. Prerecorded results are ignored. One tool per
#                  compatibility mode.
#
#   exception<ref> Differential exception test. Lets exceptions hit in the
#                  middle of instructions.
# -----------------------------------------------------------------------------

set(SST_SRC
    SSTRunner.cpp
    PlainRunner.cpp
    ExceptionRunner.cpp
    ../Common/SSTFile.cpp
    ../Common/Common.cpp)

set(SST_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/Moira
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common)

#
# SingleStepTests tools
#

add_executable(sstmame
    SSTMame.cpp
    ${SST_SRC})
    
target_link_libraries(sstmame PRIVATE moiraMAME mameref)
target_include_directories(sstmame PRIVATE ${SST_INCLUDE_DIRS})

add_executable(sstuae
    SSTUAE.cpp
    ${SST_SRC})
    
target_link_libraries(sstuae PRIVATE moiraUAEExc mameref uaecpu)
target_include_directories(sstuae PRIVATE ${SST_INCLUDE_DIRS})

add_executable(sstmusashi
    SSTMusashi.cpp
    MusashiCore.cpp
    ${SST_SRC}
)
target_link_libraries(sstmusashi PRIVATE moiraMusashi mameref musashi)
target_include_directories(sstmusashi PRIVATE
    ${SST_INCLUDE_DIRS}
    ../ThirdParty/Musashi
)

add_test(NAME SSTMame COMMAND sstmame)
add_test(NAME SSTUAE COMMAND sstuae)
add_test(NAME SSTMusashi COMMAND sstmusashi)

set_tests_properties(
    SSTMame SSTUAE SSTMusashi
    PROPERTIES LABELS "sst"
)

#
# Exception tests
#

add_executable(exceptionmame
    ExceptionMame.cpp
    ${SST_SRC})
    
target_link_libraries(exceptionmame PRIVATE moiraMAME mameref)
target_include_directories(exceptionmame PRIVATE ${SST_INCLUDE_DIRS})

add_executable(exceptionuae
    ExceptionUAE.cpp
    ${SST_SRC})
    
target_link_libraries(exceptionuae PRIVATE moiraUAEExc mameref uaecpu)
target_include_directories(exceptionuae PRIVATE ${SST_INCLUDE_DIRS})

add_test(NAME ExceptionMame COMMAND exceptionmame --ignore ccr,cycles,nbit)
add_test(NAME ReferenceMame COMMAND exceptionmame --check)
add_test(NAME ExceptionUAE COMMAND exceptionuae --ignore ccr,cycles,nbit)

set_tests_properties(
    ExceptionMame ReferenceMame ExceptionUAE
    PROPERTIES LABELS "exception")
