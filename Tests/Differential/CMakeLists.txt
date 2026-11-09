# The differential test runner. It is built once per tested Moira core
# (testRunnerUAE tests moiraUAE against UAE, testRunnerMusashi tests
# moiraMusashi against Musashi).
set(RUNNER_SRC
main.cpp
Testrunner.cpp
TestCPU.cpp
Shared.cpp
Sandbox.cpp
Randomizer.cpp
../ThirdParty/MusashiGlue/musashi.cpp
../ThirdParty/BinutilsGlue/binutils.cpp
../ThirdParty/UAEGlue/uae.cpp
)

function(add_test_runner NAME MOIRALIB)
  add_executable(${NAME} ${RUNNER_SRC})
  target_link_libraries(${NAME} PRIVATE ${MOIRALIB} binutils musashi uaecpu)
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

add_test_runner(testRunnerMusashi moiraMusashi)
add_test_runner(testRunnerUAE moiraUAE)

add_test(UnitTestMusashi testRunnerMusashi)
add_test(UnitTestUAE testRunnerUAE)
