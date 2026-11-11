# Tools based on the SingleStepTests suite. Each tool is a small main file that selects the runner
# and the reference core. The runners (SSTRunner.cpp, PlainRunner.cpp, ExceptionRunner.cpp) are shared.
#
#   sst<ref>        Runs the instructions on Moira and on a reference core and compares the
#                   results (the prerecorded results are ignored). One tool per compatibility mode.
#   exception<ref>  Differential exception test. Lets exceptions hit in the middle of instructions.

set(SST_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/SSTRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PlainRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ExceptionRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common/SSTFile.cpp)

# Builds a tool. MAIN selects runner and reference, MOIRALIB is the Moira variant under test,
# the remaining arguments are the libraries and sources of the reference.
function(add_sst_tool NAME MAIN MOIRALIB)
  add_executable(${NAME} ${MAIN} ${SST_SRC})
  target_link_libraries(${NAME} PRIVATE ${MOIRALIB} mameref ${ARGN})
  target_include_directories(${NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Moira
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Common)
endfunction()

add_sst_tool(sstmame ${CMAKE_CURRENT_SOURCE_DIR}/SSTMame.cpp moiraMAME)
add_sst_tool(sstuae ${CMAKE_CURRENT_SOURCE_DIR}/SSTUAE.cpp moiraUAEExc uaecpu)
add_sst_tool(sstmusashi ${CMAKE_CURRENT_SOURCE_DIR}/SSTMusashi.cpp moiraMusashi musashi)
target_sources(sstmusashi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/MusashiCore.cpp)
target_include_directories(sstmusashi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Musashi)

# Every compatibility mode reaches 100%.
# Its test demands the success rate of the last release, so regressions are caught.
add_test(NAME SSTMame COMMAND sstmame)
add_test(NAME SSTUAE COMMAND sstuae)
add_test(NAME SSTMusashi COMMAND sstmusashi)
set_tests_properties(SSTMame SSTUAE SSTMusashi PROPERTIES LABELS "sst")


#
# Exception tests
#

# Runs Moira and the MAME reference side by side and lets exceptions hit in the middle of instructions
add_sst_tool(exceptionmame ${CMAKE_CURRENT_SOURCE_DIR}/ExceptionMame.cpp moiraMAME)

# Same tool, but Moira runs in UAE compatibility mode and is compared against the UAE core
add_sst_tool(exceptionuae ${CMAKE_CURRENT_SOURCE_DIR}/ExceptionUAE.cpp moiraUAEExc uaecpu)

add_test(NAME ExceptionMame COMMAND exceptionmame --ignore ccr,cycles,nbit)
add_test(NAME ReferenceMame COMMAND exceptionmame --check)
add_test(NAME ExceptionUAE COMMAND exceptionuae --ignore ccr,cycles,nbit)
set_tests_properties(ExceptionMame ReferenceMame ExceptionUAE PROPERTIES LABELS "exception")
