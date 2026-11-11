# -----------------------------------------------------------------------------
# This file is part of Moira - A Motorola 68k emulator
#
# Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
# Published under the terms of the MIT License
# -----------------------------------------------------------------------------

# The randomized differential test runner. It is built once per tested Moira core
# (randomuae tests moiraUAE against UAE, randommusashi tests moiraMusashi against
# Musashi, randommame tests moiraMAMENoAE against MAME's microcoded 68000). The sources are shared with the disassembler tests (see ../Dasm).
set(RUNNER_SRC
${CMAKE_CURRENT_SOURCE_DIR}/RunnerMain.cpp
${CMAKE_CURRENT_SOURCE_DIR}/TestRunner.cpp
${CMAKE_CURRENT_SOURCE_DIR}/ExecRunner.cpp
${CMAKE_CURRENT_SOURCE_DIR}/ExecMMU.cpp
${CMAKE_CURRENT_SOURCE_DIR}/ExecCompare.cpp
${CMAKE_CURRENT_SOURCE_DIR}/DasmRunner.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Reference.cpp
${CMAKE_CURRENT_SOURCE_DIR}/MusashiRef.cpp
${CMAKE_CURRENT_SOURCE_DIR}/UAERef.cpp
${CMAKE_CURRENT_SOURCE_DIR}/MAMERef.cpp
${CMAKE_CURRENT_SOURCE_DIR}/TestCPU.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Shared.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Sandbox.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Randomizer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/MusashiGlue/musashi.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/BinutilsGlue/binutils.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/UAEGlue/uae.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/MAMEGlue/mame.cpp
)
set(RUNNER_SRC ${RUNNER_SRC} CACHE INTERNAL "Sources of the random and disassembler runners")

# Builds a runner executable. MAIN is the source file that selects the runner and its reference,
# BINUTILS is the Binutils library to link against.
function(add_test_runner NAME MOIRALIB MAIN BINUTILS)
  add_executable(${NAME} ${MAIN} ${RUNNER_SRC})
  target_link_libraries(${NAME} PRIVATE ${MOIRALIB} ${BINUTILS} musashi uaecpu mameref)
  if(MSVC)
    target_compile_options(${NAME} PRIVATE /wd4100 /wd4201 /wd4324 /wd4458)
  else()
    target_compile_options(${NAME} PRIVATE -Wno-unused-parameter -Wno-unused-variable
      -Wno-gnu-anonymous-struct -Wno-nested-anon-types)
  endif()
  target_include_directories(${NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/.
  ${CMAKE_SOURCE_DIR}/Moira
  ${CMAKE_SOURCE_DIR}/Tests/Random
  ${CMAKE_SOURCE_DIR}/Tests/ThirdParty
  ${CMAKE_SOURCE_DIR}/Tests/ThirdParty/Binutils
  ${CMAKE_SOURCE_DIR}/Tests/ThirdParty/Musashi
  )
endfunction()

add_test_runner(randommusashi moiraMusashi ${CMAKE_CURRENT_SOURCE_DIR}/RandomMusashi.cpp binutils)
add_test_runner(randomuae moiraUAE ${CMAKE_CURRENT_SOURCE_DIR}/RandomUAE.cpp binutils)
add_test_runner(randommame moiraMAMENoAE ${CMAKE_CURRENT_SOURCE_DIR}/RandomMAME.cpp binutils)

# A fixed seed keeps the automated runs reproducible. Without --seed, the runners
# derive the seed from the date.
add_test(NAME RandomMusashi COMMAND randommusashi --seed 1)
add_test(NAME RandomUAE COMMAND randomuae --seed 1)
add_test(NAME RandomMAME COMMAND randommame --seed 1)
set_tests_properties(RandomMusashi RandomUAE RandomMAME PROPERTIES LABELS "random")
