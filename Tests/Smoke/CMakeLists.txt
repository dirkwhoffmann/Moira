# Runs Moira against the SingleStepTests 68000 suite
add_executable(smoke Smoke.cpp ../Common/SSTFile.cpp)
target_link_libraries(smoke PRIVATE moiraMAME)
target_include_directories(smoke PRIVATE ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR}/../Common)

add_test(NAME Smoke COMMAND smoke)
set_tests_properties(Smoke PROPERTIES LABELS "smoke")
