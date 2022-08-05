cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# Set the project name
project(testRunner)

# Set the build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-flto -O3")

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)
  
# Add sub directories
add_subdirectory(Moira)
add_subdirectory(Musashi)

# Add the executable
add_executable(testRunner main.cpp musashi.cpp Testrunner.cpp TestCPU.cpp Sandbox.cpp)
target_link_libraries(testRunner moira)
target_link_libraries(testRunner musashi)

# Add compile options
target_compile_options(testRunner PUBLIC -Wno-unused-parameter)
target_compile_options(testRunner PUBLIC -Wno-unused-variable)
target_compile_options(testRunner PUBLIC -Wno-gnu-anonymous-struct)
target_compile_options(testRunner PUBLIC -Wno-nested-anon-types)

# Add include paths
target_include_directories(testRunner PUBLIC

${CMAKE_CURRENT_SOURCE_DIR}/.
${CMAKE_CURRENT_SOURCE_DIR}/Musashi
${CMAKE_CURRENT_SOURCE_DIR}/Moira
)
