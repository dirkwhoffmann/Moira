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

# Include the standard headers instead of forward declaring std types, which
# newer MSVC versions reject (C5285)
add_compile_definitions(DOCTEST_CONFIG_USE_STD_HEADERS)

add_executable(unit
    main.cpp
    FpuTests.cpp
    FpuExecTests.cpp
    FpuRefTests.cpp
    MmuExecTests.cpp
    BusErrorTests.cpp
    DebuggerTests.cpp
    IrqFcTests.cpp
    FastPathTests.cpp
    StackFrameTests.cpp
    ../Common/Common.cpp)
    
target_link_libraries(unit moiraMAME uaecpu)
target_include_directories(unit PUBLIC
    ${CMAKE_SOURCE_DIR}/Moira
    ${CMAKE_SOURCE_DIR}/Tests/Common)
target_include_directories(unit SYSTEM PUBLIC
    ${CMAKE_SOURCE_DIR}/Tests/ThirdParty)

add_test(NAME Unit COMMAND unit)
set_tests_properties(Unit PROPERTIES LABELS "unit")

# The same configuration tests against the default library (no bus error
# emulation): The 68020 family runs on the fast path.
add_executable(unitfast
    main.cpp
    FastPathTests.cpp
    ../Common/Common.cpp)

target_link_libraries(unitfast moira)
target_include_directories(unitfast PUBLIC
    ${CMAKE_SOURCE_DIR}/Moira
    ${CMAKE_SOURCE_DIR}/Tests/Common)
target_include_directories(unitfast SYSTEM PUBLIC
    ${CMAKE_SOURCE_DIR}/Tests/ThirdParty)

add_test(NAME UnitFast COMMAND unitfast)
set_tests_properties(UnitFast PROPERTIES LABELS "unit")
