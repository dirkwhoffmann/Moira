# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

# Third-party code that is used by the tests

#
# Binutils (disassembler reference)
#

set(BINUTILS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Binutils)

add_library(binutils STATIC
    ${BINUTILS_DIR}/m68k-dis.c
    ${BINUTILS_DIR}/m68k-opc.c)

target_include_directories(binutils PUBLIC ${CMAKE_SOURCE_DIR}/Tests/Random)

# The MIT flavor of the disassembler test needs a binutils that is compiled without MOTOROLA
# (see Tests/Random/config.h)
add_library(binutilsmit STATIC
    ${BINUTILS_DIR}/m68k-dis.c
    ${BINUTILS_DIR}/m68k-opc.c)

target_compile_definitions(binutilsmit PUBLIC RUNNER_STAGES_DASM_MIT)
target_include_directories(binutilsmit PUBLIC ${CMAKE_SOURCE_DIR}/Tests/Random)

foreach(T binutils binutilsmit)
  if(MSVC)
    target_compile_options(${T} PRIVATE /w)
  else()
    target_compile_options(${T} PRIVATE -w)
  endif()
endforeach()

#
# Musashi
#

set(MUSASHI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Musashi)

add_library(musashi STATIC
    ${MUSASHI_DIR}/m68kcpu.c
    ${MUSASHI_DIR}/m68kdasm.c
    ${MUSASHI_DIR}/m68kops.c
    ${MUSASHI_DIR}/musashi_softfloat/musashi_softfloat.c)

if(MSVC)
    target_compile_options(musashi PRIVATE /w)
else()
    target_compile_options(musashi PRIVATE -w)
endif()

#
# UAE (CPU core and FPU reference)
#

set(UAE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/UAE CACHE PATH "UAE source tree")

set(AMI ${UAE_DIR}/src)

add_library(uaecpu STATIC
    UAEGlue/UAECpu.cpp
    UAEGlue/UAERef.cpp
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
    ${AMI}/cpuemu_35.cpp ${AMI}/cpuemu_40.cpp ${AMI}/cpuemu_50.cpp)

set_target_properties(uaecpu PROPERTIES CXX_STANDARD 17)

target_compile_definitions(uaecpu PRIVATE
    AMIBERRY
    AMIBERRY_NO_JIT
    AMIBERRY_NO_THREADED_CPU)
    
if(MSVC)
    target_compile_options(uaecpu PRIVATE /w)
else()
    target_compile_options(uaecpu PRIVATE -w -O1 -fwrapv -fno-strict-aliasing)
endif()

target_include_directories(uaecpu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue/sdl-shim
    $<$<BOOL:${MSVC}>:${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue/msvc-shim>
    ${AMI} ${AMI}/include ${AMI}/osdep ${AMI}/archivers ${AMI}/threaddep)
    
target_include_directories(uaecpu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/UAEGlue)


#
# MAME's microcoded 68000 core
#

set(MAME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/MAME)

add_library(mameref STATIC
    MAMEGlue/MameCpu.cpp
    ${MAME_DIR}/m68000.cpp
    ${MAME_DIR}/m68000-decode.cpp
    ${MAME_DIR}/m68000-sdf.cpp
    ${MAME_DIR}/m68000-sdp.cpp)
    
set_target_properties(mameref PROPERTIES CXX_STANDARD 20)

target_include_directories(mameref PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/MAMEGlue/shim
    ${MAME_DIR})

target_include_directories(mameref PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/MAMEGlue)

if(MSVC)
    target_compile_options(mameref PRIVATE /w)
else()
    target_compile_options(mameref PRIVATE -w)
endif()
