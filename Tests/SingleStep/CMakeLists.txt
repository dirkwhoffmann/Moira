# Runs Moira against the SingleStepTests 68000 suite
add_executable(singleStepRunner SingleStep.cpp SSTFile.cpp)
target_link_libraries(singleStepRunner PRIVATE moiraMAME)
target_include_directories(singleStepRunner PRIVATE ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR})
