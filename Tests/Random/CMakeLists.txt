# The randomized differential test runner. It is built once per tested Moira core
# (randomuae tests moiraUAE against UAE, randommusashi tests moiraMusashi against
# Musashi). The same sources are used by the disassembler tests (see ../Dasm).
set(RUNNER_SRC
${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Testrunner.cpp
${CMAKE_CURRENT_SOURCE_DIR}/TestCPU.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Shared.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Sandbox.cpp
${CMAKE_CURRENT_SOURCE_DIR}/Randomizer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/MusashiGlue/musashi.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/BinutilsGlue/binutils.cpp
${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/UAEGlue/uae.cpp
)
set(RUNNER_SRC ${RUNNER_SRC} CACHE INTERNAL "Sources of the random and disassembler runners")

# STAGES is RANDOM (execute instructions) or DASM (disassemble only)
function(add_test_runner NAME MOIRALIB STAGES)
  add_executable(${NAME} ${RUNNER_SRC})
  target_link_libraries(${NAME} PRIVATE ${MOIRALIB} binutils musashi uaecpu)
  target_compile_definitions(${NAME} PRIVATE RUNNER_STAGES_${STAGES})
  if(MSVC)
    target_compile_options(${NAME} PRIVATE /wd4100 /wd4201 /wd4324 /wd4458)
  else()
    target_compile_options(${NAME} PRIVATE -Wno-unused-parameter -Wno-unused-variable
      -Wno-gnu-anonymous-struct -Wno-nested-anon-types)
  endif()
  target_include_directories(${NAME} PRIVATE
  ${CMAKE_SOURCE_DIR}/.
  ${CMAKE_SOURCE_DIR}/Moira
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/Tests/ThirdParty
  ${CMAKE_SOURCE_DIR}/Tests/ThirdParty/Binutils
  ${CMAKE_SOURCE_DIR}/Tests/ThirdParty/Musashi
  )
endfunction()

add_test_runner(randommusashi moiraMusashi RANDOM)
add_test_runner(randomuae moiraUAE RANDOM)

# A fixed seed keeps the automated runs reproducible. Without --seed, the runners
# derive the seed from the date.
add_test(NAME RandomMusashi COMMAND randommusashi --seed 1)
add_test(NAME RandomUAE COMMAND randomuae --seed 1)
set_tests_properties(RandomMusashi RandomUAE PROPERTIES LABELS "random")
