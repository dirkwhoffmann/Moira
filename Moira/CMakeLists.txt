# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(MOIRA_SRC

Moira.cpp
MoiraCore68000.cpp
MoiraCore68010.cpp
MoiraCore68020.cpp
MoiraDebugger.cpp
Cache/Cache.cpp
FPU/MFloat80.cpp
FPU/FpuEnv.cpp
FPU/FpuEnvMath.cpp
FPU/FpuFormats.cpp
FPU/FPU.cpp
MMU/MMU030.cpp
MMU/MMU040.cpp
)

add_library(moira STATIC ${MOIRA_SRC})

if (NOT MSVC)
    target_compile_options(moira PUBLIC -Wno-unused-parameter)
    target_compile_options(moira PUBLIC -Wno-unused-but-set-parameter)
    target_compile_options(moira PUBLIC -Wno-unused-but-set-variable)
    target_compile_options(moira PUBLIC -Wno-missing-field-initializers)
endif()

if(MSVC)
    target_compile_options(moira PUBLIC /W4)     # /bigobj /WX disabled for now
    target_compile_options(moira PUBLIC /wd4100) # unreferenced formal parameter
    target_compile_options(moira PUBLIC /wd4201) # nameless struct/union
    target_compile_options(moira PUBLIC /wd4324) # structure was padded due to alignment specifier
    target_compile_options(moira PUBLIC /wd4458) # declaration hides class member
    target_compile_options(moira PUBLIC /wd4127) # conditional expression is constant
    target_compile_options(moira PUBLIC /wd4702) # unreachable code
endif()

if(MINGW)
    # target_compile_options(moira PUBLIC -Wa,-mbig-obj)
endif()
