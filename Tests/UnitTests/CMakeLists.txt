# Unit tests (doctest). All tests are compiled into a single executable.
#
# Examples:
#   unitTests                    Run everything
#   unitTests -ts="MMU"          Run a test suite
#   unitTests -tce="*UAE*"       Exclude the (slower) comparison against UAE
#   unitTests -ltc               List all test cases
add_executable(unitTests
    main.cpp
    FpuTests.cpp
    FpuExecTests.cpp
    FpuRefTests.cpp
    MmuExecTests.cpp
    BusErrorTests.cpp)
target_link_libraries(unitTests moira uaeref)
target_include_directories(unitTests PUBLIC
    ${CMAKE_SOURCE_DIR}/Moira
    ${CMAKE_SOURCE_DIR}/Tests/ThirdParty)

add_test(NAME UnitTests COMMAND unitTests)
