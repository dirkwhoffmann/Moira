# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Runs Easy68K programs (S-records) on the console. TRAP #15 is emulated.

# Executable
add_executable(easy68k main.cpp)
target_link_libraries(easy68k PRIVATE moira)
target_include_directories(easy68k PRIVATE ${CMAKE_SOURCE_DIR}/Moira)

# Tests: The built-in Hello World program, and the same program from a file.
# The program prints the text followed by a line break. The pattern requires the
# line break, so that the echoed source code (which also contains the text) does
# not match.
add_test(NAME Easy68kHello COMMAND easy68k)
set_tests_properties(Easy68kHello PROPERTIES
    LABELS "example"
    PASS_REGULAR_EXPRESSION "Hello, world!\n")

add_test(NAME Easy68kHelloFile COMMAND easy68k --file ${CMAKE_CURRENT_SOURCE_DIR}/hello.s68)
set_tests_properties(Easy68kHelloFile PROPERTIES
    LABELS "example"
    PASS_REGULAR_EXPRESSION "Hello, world!\n")
