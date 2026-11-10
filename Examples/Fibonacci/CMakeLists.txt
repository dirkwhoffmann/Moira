# Runs a small 68000 program that computes the Fibonacci numbers
add_executable(fibonacci main.cpp)
target_link_libraries(fibonacci PRIVATE moira)
target_include_directories(fibonacci PRIVATE ${CMAKE_SOURCE_DIR}/Moira)

# The program verifies its own result and returns 1 on failure
add_test(NAME Fibonacci COMMAND fibonacci)
set_tests_properties(Fibonacci PROPERTIES LABELS "example")
