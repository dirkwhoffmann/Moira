# Verifies the MAME reference CPU against the SingleStepTests 68000 suite
add_executable(mameCheck MameCheck.cpp ../SingleStep/SSTFile.cpp)
target_link_libraries(mameCheck PRIVATE mameref)
target_include_directories(mameCheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../SingleStep)
