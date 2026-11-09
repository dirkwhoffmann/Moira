# MMU unit tests
add_executable(mmuExecTests MmuExecTests.cpp)
target_link_libraries(mmuExecTests moira)
target_include_directories(mmuExecTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

add_test(MmuExecTests mmuExecTests)
