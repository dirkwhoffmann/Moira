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

# Tests: Hello World, passed as a string and as a file
set(HELLO "S00F00006F7267303030313A32303030EB S11D200043FA000A700D4E4F70094E4F48656C6C6F2C20776F726C642100C2 S9030000FC")

add_test(NAME Easy68kHello COMMAND easy68k "${HELLO}")
set_tests_properties(Easy68kHello PROPERTIES
    LABELS "example"
    PASS_REGULAR_EXPRESSION "Hello, world!")

add_test(NAME Easy68kHelloFile COMMAND easy68k --file ${CMAKE_CURRENT_SOURCE_DIR}/hello.s68)
set_tests_properties(Easy68kHelloFile PROPERTIES
    LABELS "example"
    PASS_REGULAR_EXPRESSION "Hello, world!")
