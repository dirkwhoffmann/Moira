# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Set the project name
project(Moira)

# Enable testing
enable_testing()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

# Sanitizers (cmake -DMOIRA_SANITIZE=address,undefined). Applies to all targets. Use with
# an optimized build (e.g. RelWithDebInfo) and run the fast tests (ctest -L "unit|smoke|exception").
set(MOIRA_SANITIZE "" CACHE STRING "Comma-separated list of sanitizers (e.g. address,undefined)")
if(MOIRA_SANITIZE)
    if(MSVC)
        message(FATAL_ERROR "MOIRA_SANITIZE is not supported with MSVC")
    endif()
    add_compile_options(-fsanitize=${MOIRA_SANITIZE} -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=${MOIRA_SANITIZE})
    if(MOIRA_SANITIZE MATCHES "undefined")
        add_compile_options(-fno-sanitize-recover=undefined)
    endif()
endif()

# The CPU core
add_subdirectory(Moira)

# Example programs
add_subdirectory(Examples)

# Tests and test tools
add_subdirectory(Tests)
