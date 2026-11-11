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
#   smoke      Runs Moira on all tests in the SingleStepTests 68000 suite
#              and compares the outcome with the prerecorded results. Note
#              that Moira has to be run in MAME compat mode, since the
#              results have been derived from MAME's 68000 micro core.
# -----------------------------------------------------------------------------

add_executable(smoke
    Smoke.cpp
    ../Common/SSTFile.cpp)

target_include_directories(smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/Moira
    ../Common)

target_link_libraries(smoke PRIVATE moiraMAME)

add_test(NAME Smoke COMMAND smoke)
set_tests_properties(Smoke PROPERTIES LABELS "smoke")
