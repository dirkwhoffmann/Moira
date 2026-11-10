# Verifies the MAME reference CPU against the SingleStepTests 68000 suite
add_executable(mameCheck MameCheck.cpp ../SingleStep/SSTFile.cpp)
target_link_libraries(mameCheck PRIVATE mameref)
target_include_directories(mameCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../SingleStep)

# Runs Moira and the MAME reference side by side and lets exceptions hit in the middle of instructions
add_executable(exceptionRunner ExceptionDiff.cpp ../SingleStep/SSTFile.cpp)
target_link_libraries(exceptionRunner PRIVATE mameref uaecpu moiraMAME)
target_include_directories(exceptionRunner PRIVATE ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR}/../SingleStep)
