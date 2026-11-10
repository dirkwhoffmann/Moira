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
if(MSVC)
    target_compile_options(uaeref PRIVATE /w)
else()
    target_compile_options(uaeref PRIVATE -w -fwrapv -fno-strict-aliasing)
endif()


# Add UAE's CPU core (the reference of randomuae and the exception tests)
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
if(MSVC)
    target_compile_options(uaecpu PRIVATE /w)
else()
    # WinUAE/Amiberry rely on wrapping signed arithmetic and on type punning (UBSan reports
    # signed overflows in cpuemu_*.cpp, newcpu_common.cpp and the softfloat code)
    target_compile_options(uaecpu PRIVATE -w -O1 -fwrapv -fno-strict-aliasing)
endif()
target_include_directories(uaecpu PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue/sdl-shim
$<$<BOOL:${MSVC}>:${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue/msvc-shim>
${AMI} ${AMI}/include ${AMI}/osdep ${AMI}/archivers ${AMI}/threaddep)
target_include_directories(uaecpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue)



# MAME's microcoded 68000 core (BSD-3-Clause, used by the tests only)
#
# The core is compiled from MAME's unmodified sources on top of a minimal
# device framework (see MAMEGlue/shim). It is the reference for bus errors.
set(MAME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/MAME)
add_library(mameref STATIC
MAMEGlue/MameCpu.cpp
${MAME_DIR}/m68000.cpp
${MAME_DIR}/m68000-decode.cpp
${MAME_DIR}/m68000-sdf.cpp
${MAME_DIR}/m68000-sdp.cpp
)
set_target_properties(mameref PROPERTIES CXX_STANDARD 20)
target_include_directories(mameref PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/MAMEGlue/shim ${MAME_DIR})
target_include_directories(mameref PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/MAMEGlue)
if(MSVC)
    target_compile_options(mameref PRIVATE /w)
else()
    target_compile_options(mameref PRIVATE -w)
endif()
