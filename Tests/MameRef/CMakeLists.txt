# Verifies the MAME reference CPU against the SingleStepTests 68000 suite
add_executable(mameCheck MameCheck.cpp ../SingleStep/SSTFile.cpp)
target_link_libraries(mameCheck PRIVATE mameref)
target_include_directories(mameCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../SingleStep)

# Runs Moira and the MAME reference side by side and lets bus accesses fail
add_executable(busErrorRunner BusErrorDiff.cpp ../SingleStep/SSTFile.cpp)
target_link_libraries(busErrorRunner PRIVATE mameref moiraMAME)
target_include_directories(busErrorRunner PRIVATE ${CMAKE_SOURCE_DIR}/Moira ${CMAKE_CURRENT_SOURCE_DIR}/../SingleStep)
