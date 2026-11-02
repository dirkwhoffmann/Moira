cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Set the project name
project(testRunner)

# Enable testing
enable_testing()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)
  
# Add sub directories
add_subdirectory(Moira)
add_subdirectory(Runner/Binutils)
add_subdirectory(Runner/Musashi)

# Add the executable
add_executable(testRunner
Runner/main.cpp
Runner/Testrunner.cpp
Runner/TestCPU.cpp
Runner/musashi.cpp
Runner/Sandbox.cpp
Runner/Randomizer.cpp
)
target_link_libraries(testRunner binutils)
target_link_libraries(testRunner moira)
target_link_libraries(testRunner musashi)

# Add compile options
if(MSVC)
  target_compile_options(testRunner PUBLIC /wd4100 /wd4201 /wd4324 /wd4458)
else()
  target_compile_options(testRunner PUBLIC -Wno-unused-parameter)
  target_compile_options(testRunner PUBLIC -Wno-unused-variable)
  target_compile_options(testRunner PUBLIC -Wno-gnu-anonymous-struct)
  target_compile_options(testRunner PUBLIC -Wno-nested-anon-types)
endif()

# Add include paths
target_include_directories(testRunner PUBLIC

${CMAKE_SOURCE_DIR}/.
${CMAKE_SOURCE_DIR}/Moira
${CMAKE_SOURCE_DIR}/Runner
${CMAKE_SOURCE_DIR}/Runner/Binutils
${CMAKE_SOURCE_DIR}/Runner/Musashi
)

# Add the FPU unit tests
add_executable(fpuTests Runner/FpuTests.cpp)
target_link_libraries(fpuTests moira)
target_include_directories(fpuTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

add_executable(fpuExecTests Runner/FpuExecTests.cpp)
target_link_libraries(fpuExecTests moira)
target_include_directories(fpuExecTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

# Add the reference tests against Amiberry's floating-point back-end (optional)
set(AMIBERRY_DIR "" CACHE PATH "Amiberry source tree (enables the fpuRefTests target)")
if (AMIBERRY_DIR)
  add_library(amiberryref STATIC
  Runner/Amiberry/AmiberryRef.cpp
  ${AMIBERRY_DIR}/src/softfloat/softfloat.cpp
  ${AMIBERRY_DIR}/src/softfloat/softfloat_decimal.cpp
  ${AMIBERRY_DIR}/src/softfloat/softfloat_fpsp.cpp
  ${AMIBERRY_DIR}/src/fpp_softfloat.cpp
  )
  target_include_directories(amiberryref PRIVATE
  ${CMAKE_SOURCE_DIR}/Runner/Amiberry/shim
  ${CMAKE_SOURCE_DIR}/Runner/Amiberry
  ${AMIBERRY_DIR}/src/include
  ${AMIBERRY_DIR}/src
  )
  target_include_directories(amiberryref PUBLIC ${CMAKE_SOURCE_DIR}/Runner/Amiberry)
  target_compile_options(amiberryref PRIVATE -w)

  add_executable(fpuRefTests Runner/FpuRefTests.cpp)
  target_link_libraries(fpuRefTests moira amiberryref)
  target_include_directories(fpuRefTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_SOURCE_DIR}/Runner)
  add_test(FpuRefTests fpuRefTests)
endif()

# Add tests
add_test(UnitTest testRunner)
add_test(FpuExecTests fpuExecTests)
add_test(FpuTests fpuTests)
