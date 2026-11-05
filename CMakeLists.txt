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

# The test runner. It is compiled once and talks to the tested Moira cores through
# the adapter interface in RunnerTypes.h. It only uses Moira's enumerations (Instr,
# Model, ...) and needs no Moira library.
add_library(runnerShared STATIC
Runner/Testrunner.cpp
Runner/Shared.cpp
Runner/Sandbox.cpp
Runner/Randomizer.cpp
Runner/ThirdParty/MusashiGlue/musashi.cpp
)

# One adapter per Moira configuration. It connects the runner to a core that is
# compiled with its own settings and lives in its own namespace.
function(add_runner_adapter NAME MOIRALIB)
  add_library(${NAME} STATIC
  Runner/MoiraAdapter.cpp
  Runner/TestCPU.cpp
  )
  target_link_libraries(${NAME} PRIVATE ${MOIRALIB})
endfunction()

add_runner_adapter(runnerUAE moiraUAE)
add_runner_adapter(runnerMusashi moiraMusashi)

# Add the executable
add_executable(testRunner Runner/main.cpp)
target_link_libraries(testRunner runnerUAE runnerMusashi runnerShared binutils musashi)

# Add compile options
foreach(T testRunner runnerShared runnerUAE runnerMusashi)
if(MSVC)
  target_compile_options(${T} PUBLIC /wd4100 /wd4201 /wd4324 /wd4458)
else()
  target_compile_options(${T} PUBLIC -Wno-unused-parameter)
  target_compile_options(${T} PUBLIC -Wno-unused-variable)
  target_compile_options(${T} PUBLIC -Wno-gnu-anonymous-struct)
  target_compile_options(${T} PUBLIC -Wno-nested-anon-types)
endif()

# Add include paths
target_include_directories(${T} PUBLIC
${CMAKE_SOURCE_DIR}/.
${CMAKE_SOURCE_DIR}/Moira
${CMAKE_SOURCE_DIR}/Runner
${CMAKE_SOURCE_DIR}/Runner/ThirdParty
${CMAKE_SOURCE_DIR}/Runner/ThirdParty/Binutils
${CMAKE_SOURCE_DIR}/Runner/ThirdParty/Musashi
)
endforeach()

# Add the FPU unit tests
add_executable(fpuTests Runner/FpuTests.cpp)
target_link_libraries(fpuTests moira)
target_include_directories(fpuTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

add_executable(fpuExecTests Runner/FpuExecTests.cpp)
target_link_libraries(fpuExecTests moira)
target_include_directories(fpuExecTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

# Add the reference tests against UAE's floating-point back-end (optional)
# UAE sources (GPLv3, used by the test runner only)
set(UAE_DIR ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/UAE CACHE PATH "UAE source tree")
if (UAE_DIR)
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
endif()

# Add UAE's CPU core as an alternative reference for the test runner (optional)
if (UAE_DIR)
  # UAE includes <SDL3/SDL.h> for a few string helpers. The CPU core doesn't need
  # SDL, so a minimal shim header is used instead (see UAEGlue/sdl-shim).
  if (TRUE)
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

    foreach(T testRunner runnerShared runnerUAE runnerMusashi)
      target_compile_definitions(${T} PRIVATE HAVE_UAE)
    endforeach()
    target_link_libraries(testRunner uaecpu)
    target_link_libraries(runnerUAE PUBLIC uaecpu)
    target_link_libraries(runnerMusashi PUBLIC uaecpu)
  endif()
endif()

# Add tests
add_test(UnitTest testRunner)
add_test(FpuExecTests fpuExecTests)
add_test(FpuTests fpuTests)
