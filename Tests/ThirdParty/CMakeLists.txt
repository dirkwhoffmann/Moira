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

# Motorola syntax
target_compile_definitions(binutils PUBLIC MOTOROLA)

# The MIT flavor of the disassembler test needs a binutils that is compiled without MOTOROLA
add_library(binutilsmit STATIC
    ${BINUTILS_DIR}/m68k-dis.c
    ${BINUTILS_DIR}/m68k-opc.c)

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

set(UAE_SRC ${UAE_DIR}/src)

add_library(uaecpu STATIC
    UAEGlue/UAECpu.cpp
    UAEGlue/UAERef.cpp
    UAEGlue/UAEStubs.cpp
    ${UAE_SRC}/newcpu_common.cpp
    ${UAE_SRC}/disasm.cpp
    ${UAE_SRC}/cpustbl.cpp
    ${UAE_SRC}/readcpu.cpp
    ${UAE_SRC}/cpudefs.cpp
    ${UAE_SRC}/cpummu.cpp
    ${UAE_SRC}/cpummu30.cpp
    ${UAE_SRC}/fpp.cpp
    ${UAE_SRC}/fpp_softfloat.cpp
    ${UAE_SRC}/softfloat/softfloat.cpp
    ${UAE_SRC}/softfloat/softfloat_decimal.cpp
    ${UAE_SRC}/softfloat/softfloat_fpsp.cpp
    ${UAE_SRC}/cpuemu_0.cpp ${UAE_SRC}/cpuemu_11.cpp ${UAE_SRC}/cpuemu_13.cpp
    ${UAE_SRC}/cpuemu_20.cpp ${UAE_SRC}/cpuemu_21.cpp ${UAE_SRC}/cpuemu_22.cpp
    ${UAE_SRC}/cpuemu_23.cpp ${UAE_SRC}/cpuemu_24.cpp ${UAE_SRC}/cpuemu_31.cpp
    ${UAE_SRC}/cpuemu_32.cpp ${UAE_SRC}/cpuemu_33.cpp ${UAE_SRC}/cpuemu_34.cpp
    ${UAE_SRC}/cpuemu_35.cpp ${UAE_SRC}/cpuemu_40.cpp ${UAE_SRC}/cpuemu_50.cpp)

set_target_properties(uaecpu PROPERTIES CXX_STANDARD 17)

# Switches that UAE's platform layer (osdep/sysconfig.h) understands
target_compile_definitions(uaecpu PRIVATE
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
    ${UAE_SRC} ${UAE_SRC}/include ${UAE_SRC}/osdep ${UAE_SRC}/archivers ${UAE_SRC}/threaddep)
    
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
