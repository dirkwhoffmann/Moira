# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Runs Easy68K programs (S-record files) on the console. TRAP #15 is emulated.

# Executable
add_executable(easy68k main.cpp)
target_link_libraries(easy68k PRIVATE moira)
target_include_directories(easy68k PRIVATE ${CMAKE_SOURCE_DIR}/Moira)

# Tests
add_test(NAME Easy68kHello COMMAND easy68k -q ${CMAKE_CURRENT_SOURCE_DIR}/hello.s68)
set_tests_properties(Easy68kHello PROPERTIES
    LABELS "example"
    PASS_REGULAR_EXPRESSION "Hello, world!")

add_test(NAME Easy68kTasks COMMAND easy68k -q ${CMAKE_CURRENT_SOURCE_DIR}/tasks.s68)
set_tests_properties(Easy68kTasks PROPERTIES
    LABELS "example"
    PASS_REGULAR_EXPRESSION "RESULT: -12345 DEADBEEF 1010 \\[    42\\] OK abcab")
