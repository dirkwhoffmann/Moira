# Third-party code that is used by the tests

add_subdirectory(Binutils)
add_subdirectory(Musashi)


# UAE sources (GPLv3, used by the test runners only)
set(UAE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/UAE CACHE PATH "UAE source tree")
add_library(uaeref STATIC
UAEGlue/UAERef.cpp
${UAE_DIR}/src/softfloat/softfloat.cpp
${UAE_DIR}/src/softfloat/softfloat_decimal.cpp
${UAE_DIR}/src/softfloat/softfloat_fpsp.cpp
${UAE_DIR}/src/fpp_softfloat.cpp
)
target_include_directories(uaeref PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue/shim
${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue
${UAE_DIR}/src/include
${UAE_DIR}/src
)
target_include_directories(uaeref PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue)
target_compile_options(uaeref PRIVATE -w)


# Add UAE's CPU core (the reference of testRunnerUAE)
# UAE includes <SDL3/SDL.h> for a few string helpers. The CPU core doesn't need
# SDL, so a minimal shim header is used instead (see UAEGlue/sdl-shim).
set(AMI ${UAE_DIR}/src)
add_library(uaecpu STATIC
UAEGlue/UAECpu.cpp
UAEGlue/UAEStubs.cpp
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
${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue/sdl-shim
${AMI} ${AMI}/include ${AMI}/osdep ${AMI}/archivers ${AMI}/threaddep)
target_include_directories(uaecpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue)

