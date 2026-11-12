# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Differential sequence test: whole programs with interrupts, trace, STOP and RTE

add_executable(sequenceuae
    SequenceUAE.cpp
    ../Common/Common.cpp)

target_link_libraries(sequenceuae PRIVATE moiraUAEExc uaecpu)
target_include_directories(sequenceuae PRIVATE
    ${CMAKE_SOURCE_DIR}/Moira
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/UAEGlue)

add_test(NAME SequenceUAE COMMAND sequenceuae --seed 1 --programs 1000 --min 99)
set_tests_properties(SequenceUAE PROPERTIES LABELS "sequence")
