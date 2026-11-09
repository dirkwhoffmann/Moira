# FPU unit tests
add_executable(fpuTests FpuTests.cpp)
target_link_libraries(fpuTests moira)
target_include_directories(fpuTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

add_executable(fpuExecTests FpuExecTests.cpp)
target_link_libraries(fpuExecTests moira)
target_include_directories(fpuExecTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

# Compares Moira's FPU against UAE's softfloat implementation
add_executable(fpuRefTests FpuRefTests.cpp)
target_link_libraries(fpuRefTests moira uaeref)
target_include_directories(fpuRefTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_SOURCE_DIR}/Tests/ThirdParty)

add_test(FpuExecTests fpuExecTests)
add_test(FpuTests fpuTests)
add_test(FpuRefTests fpuRefTests)
