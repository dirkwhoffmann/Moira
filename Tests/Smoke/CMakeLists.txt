# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

# Runs Moira against the SingleStepTests 68000 suite
add_executable(smoke Smoke.cpp ../Common/SSTFile.cpp)
target_link_libraries(smoke PRIVATE moiraMAME)
target_include_directories(smoke PRIVATE ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR}/../Common)

add_test(NAME Smoke COMMAND smoke)
set_tests_properties(Smoke PROPERTIES LABELS "smoke")
