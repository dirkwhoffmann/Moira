# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Runs a small 68000 program that computes the Fibonacci numbers

# Executable
add_executable(fibonacci main.cpp)
target_link_libraries(fibonacci PRIVATE moira)
target_include_directories(fibonacci PRIVATE ${CMAKE_SOURCE_DIR}/Moira)

# Test
add_test(NAME Fibonacci COMMAND fibonacci)
set_tests_properties(Fibonacci PROPERTIES LABELS "example")
