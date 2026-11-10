# Runs the instructions of the SingleStepTests on Moira and on a reference core and compares
# the results (the prerecorded results are ignored). One tool per compatibility mode.
# The tools are built from the sources of the exception tests (see ../Exception).
set(SST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../Exception/ExceptionDiff.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../Common/SSTFile.cpp)
set(SST_INC ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR}/../Common)

add_executable(sstmame ${SST_SRC})
target_link_libraries(sstmame PRIVATE mameref uaecpu moiraMAME)
target_include_directories(sstmame PRIVATE ${SST_INC})
target_compile_definitions(sstmame PRIVATE SST_TOOL SST_MAME)

add_executable(sstuae ${SST_SRC})
target_link_libraries(sstuae PRIVATE mameref uaecpu moiraUAEExc)
target_include_directories(sstuae PRIVATE ${SST_INC})
target_compile_definitions(sstuae PRIVATE SST_TOOL SST_UAE)

add_executable(sstmusashi ${SST_SRC})
target_link_libraries(sstmusashi PRIVATE mameref uaecpu musashi moiraMusashi)
target_include_directories(sstmusashi PRIVATE ${SST_INC} ${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Musashi)
target_compile_definitions(sstmusashi PRIVATE SST_TOOL SST_MUSASHI)

# Every compatibility mode reaches 100%.
# Its test demands the success rate of the last release, so regressions are caught.
add_test(NAME SSTMame COMMAND sstmame)
add_test(NAME SSTUAE COMMAND sstuae)
add_test(NAME SSTMusashi COMMAND sstmusashi)
set_tests_properties(SSTMame SSTUAE SSTMusashi PROPERTIES LABELS "sst")
