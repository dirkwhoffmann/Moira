# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

# Unit tests (doctest). All tests are compiled into a single executable.
#
# Examples:
#   unit                    Run everything
#   unit -ts="MMU"          Run a test suite
#   unit -tce="*UAE*"       Exclude the (slower) comparison against UAE
#   unit -ltc               List all test cases

add_executable(unit
    main.cpp
    FpuTests.cpp
    FpuExecTests.cpp
    FpuRefTests.cpp
    MmuExecTests.cpp
    BusErrorTests.cpp
    ../Common/Common.cpp)
    
target_link_libraries(unit moira uaecpu)
target_include_directories(unit PUBLIC
    ${CMAKE_SOURCE_DIR}/Moira
    ${CMAKE_SOURCE_DIR}/Tests/Common
    ${CMAKE_SOURCE_DIR}/Tests/ThirdParty)

add_test(NAME Unit COMMAND unit)
set_tests_properties(Unit PROPERTIES LABELS "unit")
