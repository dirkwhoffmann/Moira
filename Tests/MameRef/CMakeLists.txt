# Verifies the MAME reference CPU against the SingleStepTests 68000 suite
add_executable(mameCheck MameCheck.cpp ../SingleStep/SSTFile.cpp)
target_link_libraries(mameCheck PRIVATE mameref)
target_include_directories(mameCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../SingleStep)

# Runs Moira and the MAME reference side by side and lets exceptions hit in the middle of instructions
add_executable(exceptionRunner ExceptionDiff.cpp ../SingleStep/SSTFile.cpp)
target_link_libraries(exceptionRunner PRIVATE mameref uaecpu moiraMAME)
target_include_directories(exceptionRunner PRIVATE ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR}/../SingleStep)

# Same tool, but Moira runs in UAE compatibility mode and is compared against the UAE core
add_executable(exceptionRunnerUAE ExceptionDiff.cpp ../SingleStep/SSTFile.cpp)
target_link_libraries(exceptionRunnerUAE PRIVATE mameref uaecpu moiraUAEExc)
target_include_directories(exceptionRunnerUAE PRIVATE ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR}/../SingleStep)
target_compile_definitions(exceptionRunnerUAE PRIVATE EXC_DEFAULT_REFERENCE_UAE=1)
