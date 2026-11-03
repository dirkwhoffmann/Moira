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

# Add the executable
add_executable(testRunner
Runner/main.cpp
Runner/Testrunner.cpp
Runner/TestCPU.cpp
Runner/ThirdParty/MusashiGlue/musashi.cpp
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
${CMAKE_SOURCE_DIR}/Runner/ThirdParty
${CMAKE_SOURCE_DIR}/Runner/ThirdParty/Binutils
${CMAKE_SOURCE_DIR}/Runner/ThirdParty/Musashi
)

# Add the FPU unit tests
add_executable(fpuTests Runner/FpuTests.cpp)
target_link_libraries(fpuTests moira)
target_include_directories(fpuTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

add_executable(fpuExecTests Runner/FpuExecTests.cpp)
target_link_libraries(fpuExecTests moira)
target_include_directories(fpuExecTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira)

# Add the reference tests against Amiberry's floating-point back-end (optional)
# Amiberry sources (GPLv3, used by the test runner only)
set(AMIBERRY_DIR ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/Amiberry CACHE PATH "Amiberry source tree")
if (AMIBERRY_DIR)
  add_library(amiberryref STATIC
  Runner/ThirdParty/AmiberryGlue/AmiberryRef.cpp
  ${AMIBERRY_DIR}/src/softfloat/softfloat.cpp
  ${AMIBERRY_DIR}/src/softfloat/softfloat_decimal.cpp
  ${AMIBERRY_DIR}/src/softfloat/softfloat_fpsp.cpp
  ${AMIBERRY_DIR}/src/fpp_softfloat.cpp
  )
  target_include_directories(amiberryref PRIVATE
  ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/AmiberryGlue/shim
  ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/AmiberryGlue
  ${AMIBERRY_DIR}/src/include
  ${AMIBERRY_DIR}/src
  )
  target_include_directories(amiberryref PUBLIC ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/AmiberryGlue)
  target_compile_options(amiberryref PRIVATE -w)

  add_executable(fpuRefTests Runner/FpuRefTests.cpp)
  target_link_libraries(fpuRefTests moira amiberryref)
  target_include_directories(fpuRefTests PUBLIC ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_SOURCE_DIR}/Runner ${CMAKE_SOURCE_DIR}/Runner/ThirdParty)
  add_test(FpuRefTests fpuRefTests)
endif()

# Add Amiberry's CPU core as an alternative reference for the test runner (optional)
if (AMIBERRY_DIR)
  find_library(SDL3_LIBRARY SDL3 HINTS /opt/homebrew/lib /usr/local/lib)
  find_path(SDL3_INCLUDE_DIR SDL3/SDL.h HINTS /opt/homebrew/include /usr/local/include)
  if (SDL3_LIBRARY AND SDL3_INCLUDE_DIR)
    set(AMI ${AMIBERRY_DIR}/src)
    add_library(amiberrycpu STATIC
    Runner/ThirdParty/AmiberryGlue/AmiberryCpu.cpp
    Runner/ThirdParty/AmiberryGlue/AmiberryStubs.cpp
    ${AMI}/newcpu_common.cpp
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
    set_target_properties(amiberrycpu PROPERTIES CXX_STANDARD 17)
    target_compile_definitions(amiberrycpu PRIVATE AMIBERRY AMIBERRY_NO_JIT)
    target_compile_options(amiberrycpu PRIVATE -w -O1)
    target_include_directories(amiberrycpu PRIVATE
    ${AMI} ${AMI}/include ${AMI}/osdep ${AMI}/archivers ${AMI}/threaddep
    ${SDL3_INCLUDE_DIR})
    target_include_directories(amiberrycpu PUBLIC ${CMAKE_SOURCE_DIR}/Runner/ThirdParty/AmiberryGlue)
    target_link_libraries(amiberrycpu ${SDL3_LIBRARY})

    target_compile_definitions(testRunner PRIVATE HAVE_AMIBERRY)
    target_link_libraries(testRunner amiberrycpu)
  else()
    message(STATUS "SDL3 not found: Amiberry's CPU core is not available as reference")
  endif()
endif()

# Add tests
add_test(UnitTest testRunner)
add_test(FpuExecTests fpuExecTests)
add_test(FpuTests fpuTests)
