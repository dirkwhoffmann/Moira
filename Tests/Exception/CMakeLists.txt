# Runs Moira and the MAME reference side by side and lets exceptions hit in the middle of instructions
add_executable(exceptionmame ExceptionDiff.cpp ../Common/SSTFile.cpp)
target_link_libraries(exceptionmame PRIVATE mameref uaecpu moiraMAME)
target_include_directories(exceptionmame PRIVATE ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR}/../Common)

# Same tool, but Moira runs in UAE compatibility mode and is compared against the UAE core
add_executable(exceptionuae ExceptionDiff.cpp ../Common/SSTFile.cpp)
target_link_libraries(exceptionuae PRIVATE mameref uaecpu moiraUAE)
target_include_directories(exceptionuae PRIVATE ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR}/../Common)
target_compile_definitions(exceptionuae PRIVATE EXC_DEFAULT_REFERENCE_UAE=1)

add_test(NAME ExceptionMame COMMAND exceptionmame --ignore ccr,cycles,nbit)
add_test(NAME ReferenceMame COMMAND exceptionmame --check)
add_test(NAME ExceptionUAE COMMAND exceptionuae --ignore ccr,cycles,nbit)
set_tests_properties(ExceptionMame ReferenceMame ExceptionUAE PROPERTIES LABELS "exception")
