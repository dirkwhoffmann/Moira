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
add_subdirectory(Runner/ThirdParty/Binutils)
add_subdirectory(Runner/ThirdParty/Musashi)

# The test runner. It is built once per tested Moira core (testRunnerUAE tests
# moiraUAE against UAE, testRunnerMusashi tests moiraMusashi against Musashi).
set(RUNNER_SRC
Runner/main.cpp
Runner/Testrunner.cpp
Runner/TestCPU.cpp
Runner/Shared.cpp
Runner/Sandbox.cpp
Runner/Randomizer.cpp
Runner/ThirdParty/MusashiGlue/musashi.cpp
Runner/ThirdParty/BinutilsGlue/binutils.cpp
Runner/ThirdParty/UAEGlue/uae.cpp
)

function(add_test_runner NAME MOIRALIB)
  add_executable(${NAME} ${RUNNER_SRC})
  target_link_libraries(${NAME} PRIVATE ${MOIRALIB} binutils musashi uaecpu)
  if(MSVC)
    target_compile_options(${NAME} PRIVATE /wd4100 /wd4201 /wd4324 /wd4458)
  else()
    target_compile_options(${NAME} PRIVATE -Wno-unused-parameter -Wno-unused-variable
      -Wno-gnu-anonymous-struct -Wno-nested-anon-types)
  endif()
  target_include_directories(${NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/.
  ${CMAKE_SOURCE_DIR}/Moira
  ${CMAKE_SOURCE_DIR}/Runner
  ${CMAKE_SOURCE_DIR}/Runner/ThirdParty
  ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/Binutils
  ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/Musashi
  )
endfunction()

add_test_runner(testRunnerMusashi moiraMusashi)
add_test_runner(testRunnerUAE moiraUAE)

# Add the FPU unit tests
add_executable(fpuTests Runner/FpuTests.cpp)
target_link_libraries(fpuTests moira)
target_include_directories(fpuTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

add_executable(fpuExecTests Runner/FpuExecTests.cpp)
target_link_libraries(fpuExecTests moira)
target_include_directories(fpuExecTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

# Add the MMU unit tests
add_executable(mmuExecTests Runner/MmuExecTests.cpp)
target_link_libraries(mmuExecTests moira)
target_include_directories(mmuExecTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

# UAE sources (GPLv3, used by the test runners only)
set(UAE_DIR ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/UAE CACHE PATH "UAE source tree")
add_library(uaeref STATIC
Runner/ThirdParty/UAEGlue/UAERef.cpp
${UAE_DIR}/src/softfloat/softfloat.cpp
${UAE_DIR}/src/softfloat/softfloat_decimal.cpp
${UAE_DIR}/src/softfloat/softfloat_fpsp.cpp
${UAE_DIR}/src/fpp_softfloat.cpp
)
target_include_directories(uaeref PRIVATE
${CMAKE_SOURCE_DIR}/Runner/ThirdParty/UAEGlue/shim
${CMAKE_SOURCE_DIR}/Runner/ThirdParty/UAEGlue
${UAE_DIR}/src/include
${UAE_DIR}/src
)
target_include_directories(uaeref PUBLIC ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/UAEGlue)
target_compile_options(uaeref PRIVATE -w)

add_executable(fpuRefTests Runner/FpuRefTests.cpp)
target_link_libraries(fpuRefTests moira uaeref)
target_include_directories(fpuRefTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_SOURCE_DIR}/Runner ${CMAKE_SOURCE_DIR}/Runner/ThirdParty)
add_test(FpuRefTests fpuRefTests)

# Add UAE's CPU core (the reference of testRunnerUAE)
# UAE includes <SDL3/SDL.h> for a few string helpers. The CPU core doesn't need
# SDL, so a minimal shim header is used instead (see UAEGlue/sdl-shim).
set(AMI ${UAE_DIR}/src)
add_library(uaecpu STATIC
Runner/ThirdParty/UAEGlue/UAECpu.cpp
Runner/ThirdParty/UAEGlue/UAEStubs.cpp
${AMI}/newcpu_common.cpp
${AMI}/disasm.cpp
${AMI}/cpustbl.cpp
${AMI}/readcpu.cpp
${AMI}/cpudefs.cpp
${AMI}/cpummu.cpp
${AMI}/cpummu30.cpp
${AMI}/fpp.cpp
${AMI}/fpp_softfloat.cpp
${AMI}/softfloat/softfloat.cpp
${AMI}/softfloat/softfloat_decimal.cpp
${AMI}/softfloat/softfloat_fpsp.cpp
${AMI}/cpuemu_0.cpp ${AMI}/cpuemu_11.cpp ${AMI}/cpuemu_13.cpp
${AMI}/cpuemu_20.cpp ${AMI}/cpuemu_21.cpp ${AMI}/cpuemu_22.cpp
${AMI}/cpuemu_23.cpp ${AMI}/cpuemu_24.cpp ${AMI}/cpuemu_31.cpp
${AMI}/cpuemu_32.cpp ${AMI}/cpuemu_33.cpp ${AMI}/cpuemu_34.cpp
${AMI}/cpuemu_35.cpp ${AMI}/cpuemu_40.cpp ${AMI}/cpuemu_50.cpp
)
set_target_properties(uaecpu PROPERTIES CXX_STANDARD 17)
target_compile_definitions(uaecpu PRIVATE AMIBERRY AMIBERRY_NO_JIT AMIBERRY_NO_THREADED_CPU)
target_compile_options(uaecpu PRIVATE -w -O1)
target_include_directories(uaecpu PRIVATE
${CMAKE_SOURCE_DIR}/Runner/ThirdParty/UAEGlue/sdl-shim
${AMI} ${AMI}/include ${AMI}/osdep ${AMI}/archivers ${AMI}/threaddep)
target_include_directories(uaecpu PUBLIC ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/UAEGlue)


# Add tests
add_test(UnitTestMusashi testRunnerMusashi)
add_test(UnitTestUAE testRunnerUAE)
add_test(FpuExecTests fpuExecTests)
add_test(MmuExecTests mmuExecTests)
add_test(FpuTests fpuTests)
